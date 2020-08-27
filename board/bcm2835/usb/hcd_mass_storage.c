#include <drivers/usb/usb_mass_storage.h>
#include <common.h>
#include <drivers/usb/usb_dev_rq.h>
#include "usb_mass_cbw.h"
#include <scsi/scsi_opcode.h>
#include <mem_access.h>
#include <delays.h>
#include <log.h>
#include "dwc2.h"

#define __MASS_PREFIX(__l) "[USB_MASS: %d "__l"] "
#define __MASS_ARGS 0

#define MASS_REQUEST_RESET       0xff
#define MASS_REQUEST_GET_MAX_LUN 0xfe

#define MASSLOG(__fmt, ...) printf(__MASS_PREFIX("") __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)
#define MASSDBG(__fmt, ...) if (usb_mass_log_level > 0)\
  printf(__MASS_PREFIX("DBG") __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)
#define MASSERR(__fmt, ...) printf(__MASS_PREFIX("ERR") __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)

DECL_STATIC_SLOT(struct usb_hcd_device_class_mass, usb_hcd_device_class_mass, 12)

static int cbw_log_level = LOG_LEVEL_ERR;

int cbw_set_log_level(int level)
{
  int old_level = cbw_log_level;
  cbw_log_level = level;
  return old_level;
}

#define __CBWLOG_PREFIX "CBW"
#define __CBWLOG(__level, __fmt, ...) LOG(cbw_log_level, __level, __CBWLOG_PREFIX, __fmt, ##__VA_ARGS__)
#define __CBW_DUMP_PREFIX(__log_level) __LOG_PREFIX(__CBWLOG_PREFIX, __log_level)

#define CBW_INFO(__fmt, ...) __CBWLOG(INFO  , __fmt, ##__VA_ARGS__)
#define CBW_DBG(__fmt, ...)  __CBWLOG(DEBUG , __fmt, ##__VA_ARGS__)
#define CBW_DBG2(__fmt, ...) __CBWLOG(DEBUG2, __fmt, ##__VA_ARGS__)
#define CBW_WARN(__fmt, ...) __CBWLOG(WARN  , __fmt, ##__VA_ARGS__)
#define CBW_CRIT(__fmt, ...) __CBWLOG(CRIT  , __fmt, ##__VA_ARGS__)
#define CBW_ERR(__fmt, ...)  __CBWLOG(ERR   , __fmt, ##__VA_ARGS__)

#define CBW_CHECK_ERR(__fmt, ...) if (err != ERR_OK) {\
  CBW_ERR("err: %d " __fmt, err, ##__VA_ARGS__);\
  goto out_err;\
}

#define CBW_DBG2_DUMP(__addr, __size)\
  hexdump_memory_ex(__CBW_DUMP_PREFIX(DEBUG2), 64, data, datasz);

struct usb_hcd_device_class_mass *usb_hcd_allocate_mass()
{
  struct usb_hcd_device_class_mass *mass;
  mass = usb_hcd_device_class_mass_alloc();
  mass->base.device_class = USB_HCD_DEVICE_CLASS_MASS;
  return mass;
}

void usb_hcd_deallocate_mass(struct usb_hcd_device_class_mass *m)
{
  usb_hcd_device_class_mass_release(m);
}

void usb_hcd_mass_init()
{
  STATIC_SLOT_INIT_FREE(usb_hcd_device_class_mass);
}

static inline void scsi_inquiry_data_to_string(char *buf, int bufsz, struct scsi_response_inquiry *data)
{
  snprintf(buf, bufsz, "version: %d, data_format: %d, add_length: %d, vendor: %s, product: %s\n",
    data->version,
    data->response_data_format,
    data->additional_length,
    data->vendor_id,
    data->product_id);
}

static inline void scsi_fill_cmd_inquiry(struct scsi_op_inquiry *op, int evpd,
  int page_code, int allocation_length)
{
  memset(op, 0, sizeof(*op));
  op->opcode = SCSI_OPCODE_INQUIRY;
  op->evpd = evpd;
  op->page_code = page_code;
  set_unaligned_16_le(&op->allocation_length, allocation_length);
  op->control = 0;
}

static inline void scsi_fill_cmd_test_unit_ready(struct scsi_op_inquiry *op)
{
  memset(op, 0, sizeof(*op));
  op->opcode = SCSI_OPCODE_TEST_UNIT_READY;
  op->control = 0;
}

static inline void csw_print(struct csw *s)
{
  CBW_DBG("CSW: %08x:%08x:data_residue:%d,status:%d",
    s->csw_signature,
    s->csw_tag,
    s->csw_data_residue,
    s->csw_status);
}

static int usb_mass_log_level = 1;

int usb_mass_reset_recovery(hcd_mass_t *m)
{
  int err;
  MASSDBG("starting reset recovery");

  err = hcd_endpoint_clear_feature(m->ep_in, USB_ENDPOINT_FEATURE_HALT);
  CHECK_ERR("failed to clear feature HALT on Bulk-IN endpoint");

  err = hcd_endpoint_clear_feature(m->ep_out, USB_ENDPOINT_FEATURE_HALT);
  CHECK_ERR("failed to clear feature HALT on Bulk-OUT endpoint");
  MASSDBG("reset recovery done");
out_err:
  return err;
}

int usb_mass_reset(hcd_mass_t *m)
{
  int err;
  int num_bytes;
  struct usb_device_request rq = {
    .request_type = USB_RQ_TYPE_CLASS_SET_INTERFACE,
    .request      = MASS_REQUEST_RESET,
    .value        = 0,
    .index        = 0,
    .length       = 0,
  };

  if (usb_mass_log_level > 0) {
    char buf[512];
    usb_rq_get_description(rq.raw, buf, sizeof(buf));
    MASSLOG("usb_mass_reset: rq:%s", buf);
  }

  err = HCD_TRANSFER_CONTROL(&m->d->pipe0, USB_DIRECTION_OUT, NULL, 0, rq.raw, &num_bytes);
  MASSLOG("usb_mass_reset complete err: %d", err);
  return err;
}

static inline void usb_cbw_transfer_debug_send_cbw(hcd_mass_t *m,
  int lun, const void *cmd, int cmdsz, int datasz, int data_direction)
{
  char cmd_string[256];
  char data_string[256];
  snprintf(cmd_string, sizeof(cmd_string), "opcode=%02xh,size=%d",
    *(char *)cmd, cmdsz);
  snprintf(data_string, sizeof(data_string), "size=%d,%s",
    datasz, data_direction == USB_DIRECTION_OUT ? "out" : "in");
  CBW_DBG("sending CBW: lun:%d,command:'%s',data:'%s'", lun, cmd_string, data_string);
}

#define CHECK_RETRYABLE_ERROR(__stage) \
  if (err != ERR_OK) {\
    if (err == ERR_RETRY) {\
      if (retries) {\
        --retries;\
        CBW_INFO("error %d at stage: %s. retrying", err, __stage);\
        wait_usec(900);\
        goto retry;\
      }\
      CBW_INFO("error %d at stage: %s. out of retries", err, __stage);\
      goto out_err;\
    }\
    CBW_ERR("failed at stage" __stage);\
    goto out_err;\
  }

static usb_pid_t next_pids[2];

static void next_pid_toggle(usb_pid_t *pid)
{
  if (*pid == USB_PID_DATA0)
    *pid = USB_PID_DATA1;
  else if (*pid == USB_PID_DATA1)
    *pid = USB_PID_DATA0;
}

int usb_cbw_transfer(hcd_mass_t *m,
  int tag, int lun, void *cmd, int cmdsz, int data_direction, void *data, int datasz)
{
  int err;
  int num_bytes;

  struct cbw cbw ALIGNED(512) = {
    .cbw_signature   = CBW_SIGNATURE,
    .cbw_tag         = tag,
    .cbw_data_length = datasz,
    .cbw_flags       = cbw_make_flags(data_direction),
    .cbw_lun         = lun,
    .cbw_length      = cmdsz
  };

  struct csw status ALIGNED(4);

  struct usb_hcd_pipe p ALIGNED(4) = {
    .address         = hcd_endpoint_get_address(m->ep_out),
    .ep              = hcd_endpoint_get_number(m->ep_out),
    .speed           = m->d->pipe0.speed,
    .max_packet_size = hcd_endpoint_get_max_packet_size(m->ep_out),
    .ls_hub_port     = m->d->pipe0.ls_hub_port,
    .ls_hub_address  = m->d->pipe0.ls_hub_address,
    .ep_type         = USB_ENDPOINT_TYPE_BULK,
  };

  char cmdbuf[31] ALIGNED(512);
  int retries = 5;

  usb_pid_t *next_pid = &m->ep_out->next_toggle_pid;

  if (cbw_log_level >= LOG_LEVEL_DEBUG)
    usb_cbw_transfer_debug_send_cbw(m, lun, cmd, cmdsz, datasz, data_direction);

  memset(cmdbuf, 0, sizeof(cmdbuf));
  BUG(sizeof(cbw) + cmdsz > sizeof(cmdbuf), "CBW packet must be exactly 31 bytes");

  memcpy(cmdbuf, &cbw, sizeof(cbw));
  memcpy(cmdbuf + sizeof(cbw), cmd, cmdsz);
  memcpy(&status, 0, sizeof(status));
  dcache_flush(cmdbuf, sizeof(cmdbuf));
  hexdump_memory(cmdbuf, sizeof(cmdbuf));

retry:
  next_pid = &next_pids[USB_DIRECTION_OUT];
  err = hcd_transfer_bulk(&p, USB_DIRECTION_OUT, cmdbuf, sizeof(cmdbuf), next_pid, &num_bytes);
  CHECK_RETRYABLE_ERROR("OUT:CBW");
  next_pid_toggle(next_pid);

  if (data && datasz) {
    if (data_direction == USB_DIRECTION_IN) {
      p.ep = hcd_endpoint_get_number(m->ep_in);
      next_pid = &m->ep_in->next_toggle_pid;
      next_pid = &next_pids[USB_DIRECTION_IN];
    }
    err = hcd_transfer_bulk(&p, data_direction, data, datasz, next_pid, &num_bytes);
    CHECK_RETRYABLE_ERROR("INOUT:DATA");
    next_pid_toggle(next_pid);
  }
  if (cbw_log_level >= LOG_LEVEL_DEBUG2) {
    CBW_DBG2("transfer complete: ");
    CBW_DBG2_DUMP(data, datasz);
  }

  p.ep = hcd_endpoint_get_number(m->ep_in);
  next_pid = &m->ep_in->next_toggle_pid;
  next_pid = &next_pids[USB_DIRECTION_IN];
  err = hcd_transfer_bulk(&p, USB_DIRECTION_IN, &status, sizeof(status), next_pid, &num_bytes);
  CHECK_RETRYABLE_ERROR("IN:CSW");
  next_pid_toggle(next_pid);

  if (!is_csw_valid(&status, num_bytes, tag)) {
    err = ERR_GENERIC;
    CBW_ERR("recieved CSW not valid");
    goto out_reset_recovery;
  }
  if (!is_csw_meaningful(&status, datasz)) {
    err = ERR_GENERIC;
    CBW_ERR("recieved CSW not meaningful");
    goto out_reset_recovery;
  }
  if (status.csw_status == CSW_STATUS_PHASE_ERR) {
    CBW_ERR("device responded with PHASE_ERROR");
    goto out_reset_recovery;
  }

  if (cbw_log_level >= LOG_LEVEL_DEBUG)
    csw_print(&status);

out_err:
  return err;

out_reset_recovery:
  err = usb_mass_reset_recovery(m);
  if (err != ERR_OK) {
    CBW_ERR("failed to perform reset recovery");
  }
  else {
    err = ERR_RETRY;
  }
  return err;
}

int usb_mass_read_capacity10(hcd_mass_t *m, int lun)
{
  int err;
  struct scsi_op_read_capacity_10 cmd ALIGNED(4) = { 0 };
  struct scsi_read_capacity_param param ALIGNED(4);
  cmd.opcode = SCSI_OPCODE_READ_CAPACITY_10;

  MASSLOG("READ CAPACITY(10)");
  err = usb_cbw_transfer(m, 3, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, &param, sizeof(param));
  CHECK_ERR("READ CAPACITY(10) failed");
out_err:
  return err;
}

int usb_mass_read10(hcd_mass_t *m,
  int lun, uint32_t offset, void *data_dst, int data_sz)
{
  int err;
  struct scsi_op_read_10 cmd ALIGNED(4) = { 0 };
  cmd.opcode = SCSI_OPCODE_READ_10;
  set_unaligned_32_le(&cmd.lba, offset);
  set_unaligned_16_le(&cmd.transfer_len, data_sz / 512);
  MASSLOG("READ(10)");

  memset(data_dst, 0, data_sz);
  err = usb_cbw_transfer(m, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, data_dst, data_sz);
  CHECK_ERR("READ(10) command failed");
out_err:
  return err;
}

int usb_mass_write10(hcd_mass_t *m,
  int lun, uint32_t offset, void *data_dst, int data_sz)
{
  int err;
  struct scsi_op_write_10 cmd ALIGNED(4) = { 0 };
  cmd.opcode = SCSI_OPCODE_WRITE_10;
  set_unaligned_32_le(&cmd.lba, offset);
  set_unaligned_16_le(&cmd.transfer_len, data_sz / 512);
  MASSLOG("WRITE(10)");
  err = usb_cbw_transfer(m, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_OUT, data_dst, data_sz);
  CHECK_ERR("WRITE(10) command failed");
out_err:
  return err;
}

int usb_mass_test_unit_ready(hcd_mass_t *m, int lun)
{
  int err;
  struct scsi_op_test_unit_ready cmd ALIGNED(512) = { 0 };
  cmd.opcode = SCSI_OPCODE_TEST_UNIT_READY;
  MASSLOG("TEST UNIT READY: cmd size: %d", sizeof(cmd));
  err = usb_cbw_transfer(m, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, NULL, 0);
  CHECK_ERR("TEST UNIT READY failed");
out_err:
  return err;
}

int usb_mass_inquiry(hcd_mass_t *m, int lun)
{
  int err;
  char buf[128] ALIGNED(512);
  char description[256];
  struct scsi_op_inquiry cmd;
  struct scsi_response_inquiry *info = (struct scsi_response_inquiry *)buf;
  const int min_cmd_len = 5;

  memset(&cmd, 0, sizeof(cmd));
  memset(buf , 0, sizeof(buf));

  scsi_fill_cmd_inquiry(&cmd, 0, 0, min_cmd_len);
  MASSLOG("SCSI_OPCODE_INQUIRY: cmd size: %d", sizeof(cmd));
  err = usb_cbw_transfer(m, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, min_cmd_len);
  CHECK_ERR("Failed to get INQUIRY data length");
  scsi_fill_cmd_inquiry(&cmd, 0, 0, info->additional_length);
  MASSLOG("SCSI_OPCODE_INQUIRY 2: cmd size: %d", sizeof(cmd));
  err = usb_cbw_transfer(m, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, info->additional_length);
  CHECK_ERR("Failed to get full INQUIRY data");
  scsi_inquiry_data_to_string(description, sizeof(description), info);
  MASSLOG("%s", description);
out_err:
  return err;
}

int usb_mass_get_max_lun(struct usb_hcd_device *dev, int *out_max_lun)
{
  int err;
  int num_bytes;
  struct usb_device_request rq = {
    .request_type = USB_DEVICE_REQUEST_TYPE(DEV2HOST, CLASS, INTERFACE),
    .request      = MASS_REQUEST_GET_MAX_LUN,
    .value        = 0,
    .index        = 0,
    .length       = 1,
  };

  rq.request_type = 0xa1;
  rq.request = 0xfe;
  rq.value = 0;
  rq.index = 0;
  rq.length = 1;
  char max_lun ALIGNED(4);

  MASSLOG("usb_mass_get_max_lun: %016x", rq.raw);

  err = HCD_TRANSFER_CONTROL(&dev->pipe0, USB_DIRECTION_IN,
      &max_lun, 1, rq.raw, &num_bytes);
  if (err) {
    HCDERR("failed to read descriptor header");
    goto out_err;
  }
out_err:
  HCDLOG("max_lun: %d", max_lun);
  *out_max_lun = max_lun;
  return err;
}

hcd_mass_t *usb_mass_init_class(struct usb_hcd_device* d)
{
  int err = ERR_OK;
  struct usb_hcd_interface *i = NULL;
  struct usb_hcd_endpoint *ep_tmp = NULL;
  hcd_mass_t *m = NULL;

  m = usb_hcd_allocate_mass();
  if (IS_ERR(m)) {
    HCDERR("failed to allocate mass device");
    err = PTR_ERR(m);
    m = NULL;
    goto out_err;
  }

  i = hcd_device_get_interface(d, 0);
  if (IS_ERR(i)) {
    HCDERR("failed to get inteface");
    err = ERR_GENERIC;
    goto out_err;
  }

  ep_tmp = hcd_interface_get_endpoint(i, 0);
  if (!ep_tmp) {
    HCDERR("failed to get interface %d, endpoint %d", 0, 0);
    err = ERR_GENERIC;
    goto out_err;
  }

  if (hcd_endpoint_get_direction(ep_tmp) == USB_DIRECTION_OUT)
    m->ep_out = ep_tmp;
  else
    m->ep_in = ep_tmp;

  ep_tmp = hcd_interface_get_endpoint(i, 1);
  if (!ep_tmp) {
    HCDERR("failed to get interface %d, endpoint %d", 0, 1);
    err = ERR_GENERIC;
    goto out_err;
  }

  if (hcd_endpoint_get_direction(ep_tmp) == USB_DIRECTION_OUT)
    m->ep_out = ep_tmp;
  else
    m->ep_in = ep_tmp;

  MASSLOG("usb_mass_storage_init: OUT:ep%d, IN:ep%d",
    hcd_endpoint_get_number(m->ep_out),
    hcd_endpoint_get_number(m->ep_in));
  m->d = d;

out_err:
  if (err) {
    if (m)
      usb_hcd_deallocate_mass(m);
    return ERR_PTR(err);
  }

  return m;
}

void usb_mass_deinit_class(hcd_mass_t* m)
{
  if (m)
    usb_hcd_deallocate_mass(m);
}

int usb_mass_read(hcd_mass_t *m)
{
  int err;
  char buf[512];
  memset(buf, 0x66, sizeof(buf));
  err = usb_mass_read10(m, 0, 0, buf, sizeof(buf));
  CHECK_ERR("usb_mass_read10 failed");
  hexdump_memory(buf, sizeof(buf));
out_err:
  return err;
}

int usb_mass_init(struct usb_hcd_device* d)
{
  int err;
  int max_lun = 0;
  int lun = 0;
  hcd_mass_t *m = NULL;

  m = usb_mass_init_class(d);
  if (IS_ERR(m)) {
    err = PTR_ERR(m);
    HCDERR("mass device not created");
    goto out_err;
  }

  next_pids[USB_DIRECTION_OUT] = USB_PID_DATA0;
  next_pids[USB_DIRECTION_IN] = USB_PID_DATA0;
  err = usb_mass_reset(m);
  CHECK_ERR("failed to reset mass storage device");

  err = usb_mass_get_max_lun(d, &max_lun);
  CHECK_ERR("failed to get max lun");
  usb_mass_set_log_level(1);
  err = usb_mass_test_unit_ready(m, lun);
  CHECK_ERR("failed to test unit ready");
  err = usb_mass_test_unit_ready(m, lun);
  CHECK_ERR("failed to test unit ready");
  err = usb_mass_test_unit_ready(m, lun);
  CHECK_ERR("failed to test unit ready");
  // dwc2_set_log_level(10);
  err = usb_mass_inquiry(m, lun);
  CHECK_ERR("failed to send INQUIRY request");
  err = usb_mass_read_capacity10(m, lun);
  CHECK_ERR("failed to get capacity");
  usb_mass_read(m);
out_err:
  if (err && m)
    usb_mass_deinit_class(m);
  return err;
}

int usb_mass_set_log_level(int level)
{
  int old_level = usb_mass_log_level;
  usb_mass_log_level = level;
  return old_level;
}
