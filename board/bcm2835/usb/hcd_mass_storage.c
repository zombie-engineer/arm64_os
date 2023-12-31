#include <drivers/usb/usb_mass_storage.h>
#include <drivers/usb/hcd_hub.h>
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

static int usb_mass_log_level = 0;

static inline void csw_print(struct csw *s)
{
  MASSDBG("CSW: %08x:%08x:data_residue:%d,status:%d\r\n",
    s->csw_signature,
    s->csw_tag,
    s->csw_data_residue,
    s->csw_status);
}

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

static int tag = 0;

int usb_cbw_transfer(hcd_mass_t *m,
  int lun, void *cmd, int cmdsz, int data_direction, void *data, int datasz, int *csw_status)
{
  int err;
  int num_bytes;
  int last_tag = tag++;

  struct cbw cbw ALIGNED(512) = {
    .cbw_signature   = CBW_SIGNATURE,
    .cbw_tag         = last_tag,
    .cbw_data_length = datasz,
    .cbw_flags       = cbw_make_flags(data_direction),
    .cbw_lun         = lun,
    .cbw_length      = cmdsz
  };

  struct csw status ALIGNED(4);

  int retries = 5;

  if (cbw_log_level >= LOG_LEVEL_DEBUG)
    usb_cbw_transfer_debug_send_cbw(m, lun, cmd, cmdsz, datasz, data_direction);

  BUG(sizeof(cbw) != 31, "CBW packet must be exactly 31 bytes");
  BUG(cmdsz > sizeof(cbw.cbw_block), "command block size more than 16 bytes");
  memcpy(cbw.cbw_block, cmd, cmdsz);

retry:
  err = hcd_transfer_bulk(m->ep_out, &cbw, sizeof(cbw), &num_bytes);
  CHECK_RETRYABLE_ERROR("OUT:CBW");

  if (data && datasz) {
    err = hcd_transfer_bulk(
      data_direction == USB_DIRECTION_OUT ? m->ep_out : m->ep_in,
      data, datasz, &num_bytes);
    putc('.');
    CHECK_RETRYABLE_ERROR("INOUT:DATA");
  }
  if (cbw_log_level >= LOG_LEVEL_DEBUG2) {
    CBW_DBG2("transfer complete: ");
    CBW_DBG2_DUMP(data, datasz);
  }

  memset(&status, 0, sizeof(status));
  err = hcd_transfer_bulk(m->ep_in, &status, sizeof(status), &num_bytes);
  CHECK_RETRYABLE_ERROR("IN:CSW");

  if (!is_csw_valid(&status, num_bytes, last_tag)) {
    err = ERR_GENERIC;
    csw_print(&status);
    CBW_ERR("recieved CSW not valid");
    goto out_reset_recovery;
  }
  if (!is_csw_meaningful(&status, datasz)) {
    err = ERR_GENERIC;
    csw_print(&status);
    CBW_ERR("recieved CSW not meaningful");
    goto out_reset_recovery;
  }
  if (status.csw_status == CSW_STATUS_PHASE_ERR) {
    CBW_ERR("device responded with PHASE_ERROR");
    goto out_reset_recovery;
  }

 // if (cbw_log_level >= LOG_LEVEL_DEBUG)
  csw_print(&status);
  *csw_status = status.csw_status;

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
  int csw_status = 0;
  struct scsi_op_read_capacity_10 cmd ALIGNED(4) = { 0 };
  struct scsi_read_capacity_param param ALIGNED(4);
  cmd.opcode = SCSI_OPCODE_READ_CAPACITY_10;

  MASSLOG("READ CAPACITY(10)");
  err = usb_cbw_transfer(m, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, &param, sizeof(param), &csw_status);
  hexdump_memory_ex("capacity", 16, &param, sizeof(param));
  CHECK_ERR("READ CAPACITY(10) failed");
out_err:
  return err;
}

static int usb_mass_read10(hcd_mass_t *m,
  int lun, void *dst, uint32_t offset, int size)
{
  int err;
  int csw_status ALIGNED(512) = 0;
  struct scsi_op_read_10 cmd ALIGNED(512) = { 0 };
  cmd.opcode = SCSI_OPCODE_READ_10;
  set_unaligned_32_le(&cmd.lba, offset / 512);
  set_unaligned_16_le(&cmd.transfer_len, size / 512);
  // MASSLOG("READ(10) offset:%08x, size:%08x", offset, data_sz);
  // usb_mass_set_log_level(10);
  // usb_hcd_set_log_level(10);
  // dwc2_set_log_level(10);

  // memset(data_dst, 0, data_sz);
  err = usb_cbw_transfer(m, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, dst, size, &csw_status);
  CHECK_ERR("READ(10) command failed");
out_err:
  return err;
}

int usb_mass_write10(hcd_mass_t *m,
  int lun, uint32_t offset, void *data_dst, int data_sz)
{
  int err;
  struct scsi_op_write_10 cmd ALIGNED(4) = { 0 };
  int csw_status = 0;
  cmd.opcode = SCSI_OPCODE_WRITE_10;
  set_unaligned_32_le(&cmd.lba, offset);
  set_unaligned_16_le(&cmd.transfer_len, data_sz / 512);
  MASSLOG("WRITE(10)");
  err = usb_cbw_transfer(m, lun, &cmd, sizeof(cmd), USB_DIRECTION_OUT, data_dst, data_sz, &csw_status);
  CHECK_ERR("WRITE(10) command failed");
out_err:
  return err;
}

int usb_mass_test_unit_ready(hcd_mass_t *m, int lun, int *csw_status)
{
  int err;
  struct scsi_op_test_unit_ready cmd ALIGNED(512) = { 0 };
  cmd.opcode = SCSI_OPCODE_TEST_UNIT_READY;
  MASSLOG("TEST UNIT READY: cmd size: %d", sizeof(cmd));
  err = usb_cbw_transfer(m, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, NULL, 0, csw_status);
  CHECK_ERR("TEST UNIT READY failed");
out_err:
  return err;
}

int usb_mass_request_sense(hcd_mass_t *m, int lun, int *csw_status)
{
  char buf[18] ALIGNED(64);
  int err;
  struct scsi_op_request_sense cmd ALIGNED(512) = { 0 };
  cmd.opcode = SCSI_OPCODE_REQUEST_SENSE;
  cmd.alloc_length = sizeof(buf);
  MASSLOG("REQUEST_SENSE: cmd size: %d", sizeof(cmd));
  err = usb_cbw_transfer(m, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, sizeof(buf), csw_status);
  CHECK_ERR("REQUEST_SENSE failed");
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
  int inquiry_data_size = 36;
  int csw_status = 0;

  memset(&cmd, 0, sizeof(cmd));
  memset(buf , 0, sizeof(buf));

  scsi_fill_cmd_inquiry(&cmd, 0, 0, inquiry_data_size);
  MASSLOG("SCSI_OPCODE_INQUIRY: cmd size: %d", sizeof(*info));
  err = usb_cbw_transfer(m, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, inquiry_data_size, &csw_status);
  CHECK_ERR("Failed to get INQUIRY data length");

//  inquiry_data_size = info->additional_length;
//  scsi_fill_cmd_inquiry(&cmd, 0, 0, inquiry_data_size);
//  MASSLOG("SCSI_OPCODE_INQUIRY 2: cmd size: %d", sizeof(cmd));
//  err = usb_cbw_transfer(m, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, inquiry_data_size, &csw_status);
//  CHECK_ERR("Failed to get full INQUIRY data");
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
  d->class = &m->base;

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

int usb_mass_read(hcd_mass_t *m, void *dst, uint32_t offset, int size)
{
  int err;
  // int port = m->d->location.hub_port;
  // char buf[512];
  // struct usb_hub_port_status port_status ALIGNED(4);
  // err = usb_hub_port_get_status(usb_hcd_device_to_hub(m->d->location.hub), port, &port_status);
  // usb_status_to_string(&port_status, buf, sizeof(buf));
  // printf("get_port_status(%s): port:%d,err:%d,%s", "before_usb_read", port, err, buf);
  err = usb_mass_read10(m, 0, dst, offset, size);
  printf("MASS_READ: completed: offset: %d, size: %d, %d\r\n", offset, size, err);
  return err;
}

static int usb_mass_read_debug(hcd_mass_t *m)
{
  int err;
  char buf[512];
  memset(buf, 0x66, sizeof(buf));
  err = usb_mass_read10(m, 0, buf, 0, sizeof(buf));
  CHECK_ERR("usb_mass_read10 failed");
out_err:
  return err;
}

#if 0

static inline void usb_mass_init_debug()
{
  wait_msec(500);
  usb_mass_read_debug(m);
  err = usb_mass_test_unit_ready(m, lun, &csw_status);
  CHECK_ERR("failed to test unit ready");
  err = usb_mass_test_unit_ready(m, lun, &csw_status);
  CHECK_ERR("failed to test unit ready");
  // dwc2_set_log_level(10);
}
#endif

int usb_mass_init(struct usb_hcd_device* d)
{
  int err;
  int max_lun = 0;
  int lun = 0;
  int csw_status = 0;
  hcd_mass_t *m = NULL;
//  char buf[512];

  m = usb_mass_init_class(d);
  if (IS_ERR(m)) {
    err = PTR_ERR(m);
    HCDERR("mass device not created");
    goto out_err;
  }

  err = usb_mass_reset(m);
  // CHECK_ERR("failed to reset mass storage device");
  // usb_mass_set_log_level(20);
  // dwc2_set_log_level(20);
  err = usb_mass_get_max_lun(d, &max_lun);
  CHECK_ERR("failed to get max lun");
  printf("max_lun : %d\r\n", max_lun);
  // wait_msec(100);
  err = usb_mass_inquiry(m, lun);
  CHECK_ERR("failed to send INQUIRY request");

  err = usb_mass_test_unit_ready(m, lun, &csw_status);
  CHECK_ERR("failed to test unit ready");
  if (csw_status == CSW_STATUS_CHECK_CONDITION) {
    err = usb_mass_request_sense(m, lun, &csw_status);
  }

  err = usb_mass_read_capacity10(m, lun);
  CHECK_ERR("failed to get capacity");
out_err:
  if (err && m) {
    while(1)
      puts("%^^^^\r\n");
    usb_mass_deinit_class(m);
  }
  dcache_flush(d, sizeof(*d));
  dcache_flush(m, sizeof(*m));
  return err;
}

int usb_mass_set_log_level(int level)
{
  int old_level = usb_mass_log_level;
  usb_mass_log_level = level;
  return old_level;
}
