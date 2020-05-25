#include <drivers/usb/usb_mass_storage.h>
#include <common.h>
#include <drivers/usb/usb_dev_rq.h>
#include "usb_mass_cbw.h"
#include <scsi/scsi_opcode.h>
#include <mem_access.h>
#include <delays.h>

#define __MASS_PREFIX(__l) "[USB_MASS: %d "__l"] "
#define __MASS_ARGS 0

#define MASS_REQUEST_RESET       0xff
#define MASS_REQUEST_GET_MAX_LUN 0xfe

#define MASSLOG(__fmt, ...) printf(__MASS_PREFIX("") __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)
#define MASSDBG(__fmt, ...) if (usb_mass_log_level > 0)\
  printf(__MASS_PREFIX("DBG") __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)
#define MASSERR(__fmt, ...) printf(__MASS_PREFIX("ERR") __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)

#define USB_HCTSIZ0_PID_DATA0 0
#define USB_HCTSIZ0_PID_DATA1 2
#define USB_HCTSIZ0_PID_DATA2 1
#define USB_HCTSIZ0_PID_SETUP 3

extern int dwc2_set_log_level(int);

struct scsi_op_test_unit_ready {
  uint8_t opcode          ;
  uint8_t reserved[4]     ; // +1
  uint8_t control         ; // +5
} PACKED;

struct scsi_op_log_select {
  uint8_t opcode          ;
  uint8_t sp           : 1; // +1
  uint8_t pcr          : 1;
  uint8_t reserved0    : 6;
  uint8_t page_code    : 6; // +2
  uint8_t pc           : 2;
  uint8_t subpage_code    ; // +3
  uint8_t reserved[3]     ; // +4
  uint16_t param_list_len ; // +6
  uint8_t control         ; // +9
} PACKED;

struct scsi_op_read_10 {
  uint8_t opcode       ;
  uint8_t obsolete0 : 2; // +1
  uint8_t rarc      : 1;
  uint8_t fua       : 1;
  uint8_t dpo       : 1;
  uint8_t rdprotect : 3;

  uint32_t lba         ; // +2
  uint8_t group_num : 5; // +6
  uint8_t reserved  : 3;
  uint16_t transfer_len; // +7
  uint8_t control      ; // +9
} PACKED;

struct scsi_op_write_10 {
  uint8_t opcode       ;

  uint8_t obsolete0 : 2; // +1
  uint8_t reserved0 : 1; // +1
  uint8_t fua       : 1;
  uint8_t dpo       : 1;
  uint8_t wrprotect : 3;
  uint32_t lba         ; // +2
  uint8_t group_num : 5; // +6
  uint8_t reserved  : 3;
  uint16_t transfer_len; // +7
  uint8_t control      ; // +9
} PACKED;

struct scsi_op_read_capacity_10 {
  uint8_t opcode       ;
  uint8_t obsolete0 : 2; // +1
  uint8_t rarc      : 1;
  uint8_t fua       : 1;
  uint8_t dpo       : 1;
  uint8_t rdprotect : 3;

  uint32_t lba         ; // +2
  uint8_t group_num : 5; // +6
  uint8_t reserved  : 3;
  uint16_t transfer_len; // +7
  uint8_t control      ; // +9
} PACKED;

struct scsi_read_capacity_param {
  uint32_t logic_address;
  uint32_t block_length;
} PACKED;

struct scsi_op_inquiry {
  uint8_t opcode;
  uint8_t evpd : 1;
  uint8_t reserved1: 7;
  uint8_t page_code;
  uint16_t allocation_length;
  uint8_t control;
} PACKED;

struct scsi_response_inquiry {
  uint8_t peripheral_device_type : 5;
  uint8_t peripheral_qualifier   : 3;
  uint8_t reserved0              : 7; // +1
  uint8_t rmb                    : 1;
  uint8_t version                   ; // +2
  uint8_t response_data_format   : 4; // +3
  uint8_t hisup                  : 1;
  uint8_t normaca                : 1;
  uint8_t obsolete0              : 2;
  uint8_t additional_length         ; // +4 /* N-1 */
  uint8_t protect                : 1; // +5
  uint8_t obsolete1              : 2;
  uint8_t tpc                    : 1;
  uint8_t tpgs                   : 2;
  uint8_t acc                    : 1;
  uint8_t sccs                   : 1;

  uint8_t obsolete2              : 4; // +6
  uint8_t multip                 : 1;
  uint8_t vs                     : 1;
  uint8_t encserv                : 1;
  uint8_t obsolete3              : 1;

  uint8_t vs2                    : 1; // +7
  uint8_t cmdque                 : 1;
  uint8_t obsolete4              : 6;

  char    vendor_id[8]           ;   // +8
  char    product_id[16]         ;   // +16
  char    product_revision_lvl[4];   // +32
  char    drive_serial_number[8] ;   // +36
  char    vendor_unique[12]      ;   // +44
  char    reserved1              ;   // +56
  char    reserved2              ;   // +57
  uint16_t version_descriptor[8] ;   // +58
  char    reserved[22]           ;   // +74
  char    copyright_notice[0]    ;   // +96
} PACKED;

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
  printf("CSW: %08x:%08x:data_residue:%d,status:%d" __endline,
    s->csw_signature,
    s->csw_tag,
    s->csw_data_residue,
    s->csw_status);
}

static int usb_mass_log_level = 1;

int usb_mass_reset_recovery(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in)
{
  int err;
  MASSDBG("starting reset recovery");

  err = hcd_endpoint_clear_feature(ep_in, USB_ENDPOINT_FEATURE_HALT);
  CHECK_ERR("failed to clear feature HALT on Bulk-IN endpoint");

  err = hcd_endpoint_clear_feature(ep_out, USB_ENDPOINT_FEATURE_HALT);
  CHECK_ERR("failed to clear feature HALT on Bulk-OUT endpoint");
  MASSDBG("reset recovery done");
out_err:
  return err;
}

int usb_mass_reset(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in)
{
  int err;
  int num_bytes;
	struct usb_hcd_pipe_control pctl;
  struct usb_device_request rq = { 
    .request_type = USB_RQ_TYPE_CLASS_SET_INTERFACE,
    .request      = MASS_REQUEST_RESET,
    .value        = 0,
    .index        = 0,
    .length       = 0,
  };

  MASSLOG("usb_mass_reset: request=t:%02x,r:%02x,v:%02x,i:%02x,l:%02x", rq.request_type, rq.request, rq.value, rq.index, rq.length);

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_OUT;

  err = hcd_transfer_control(&ep_out->device->pipe0, &pctl, 
      NULL, 0, rq.raw, 1000, &num_bytes);
  MASSLOG("usb_mass_reset complete err: %d", err);
  return err;
}

static inline void usb_cbw_transfer_debug_send_cbw(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in,
  int lun, const void *cmd, int cmdsz, int datasz, int data_direction)
{
  char cmd_string[256];
  char data_string[256];
  snprintf(cmd_string, sizeof(cmd_string), "opcode=%02xh,size=%d",
    *(char *)cmd, cmdsz);
  snprintf(data_string, sizeof(data_string), "size=%d,%s",
    datasz, data_direction == USB_DIRECTION_OUT ? "out" : "in");
  MASSDBG("sending CBW: lun:%d,command:'%s',data:'%s'", lun, cmd_string, data_string);
}

int usb_cbw_transfer(
  struct usb_hcd_endpoint *ep_out,
  struct usb_hcd_endpoint *ep_in,
  int tag, int lun, void *cmd, int cmdsz, int data_direction, void *data, int datasz)
{
  int err;
  int num_bytes;
  struct cbw cbw ALIGNED(4) = {
    .cbw_signature   = CBW_SIGNATURE,
    .cbw_tag         = tag,
    .cbw_data_length = datasz,
    .cbw_flags       = cbw_make_flags(data_direction),
    .cbw_lun         = lun,
    .cbw_length      = cmdsz
  };

  struct csw status ALIGNED(4);

  struct usb_hcd_pipe p ALIGNED(4) = {
    .address         = hcd_endpoint_get_address(ep_out),
    .endpoint        = hcd_endpoint_get_number(ep_out),
    .speed           = ep_out->device->pipe0.speed,
    .max_packet_size = hcd_endpoint_get_max_packet_size(ep_out),
    .ls_hub_port     = ep_out->device->pipe0.ls_hub_port,
    .ls_hub_address  = ep_out->device->pipe0.ls_hub_address
  };

  char cmdbuf[31] ALIGNED(4);

  usb_pid_t *next_pid = &ep_out->next_toggle_pid;

  if (usb_mass_log_level)
    usb_cbw_transfer_debug_send_cbw(ep_out, ep_in, lun, cmd, cmdsz, datasz, data_direction);

  memset(cmdbuf, 0, sizeof(cmdbuf));
  BUG(sizeof(cbw) + cmdsz > sizeof(cmdbuf), "CBW packet must be exactly 31 bytes");

  memcpy(cmdbuf, &cbw, sizeof(cbw));
  memcpy(cmdbuf + sizeof(cbw), cmd, cmdsz);
  memcpy(&status, 0, sizeof(status));

  err = hcd_transfer_bulk(&p, USB_DIRECTION_OUT, cmdbuf, sizeof(cmdbuf), next_pid, &num_bytes);
  CHECK_ERR("transfer failed CBW send");
  if (data && datasz) {
    if (data_direction == USB_DIRECTION_IN) {
      p.endpoint = hcd_endpoint_get_number(ep_in);
      next_pid = &ep_in->next_toggle_pid;
    }
    int retries = 0;
    for (retries = 0; retries < 5; retries++) {
      wait_usec(500);
      err = hcd_transfer_bulk(&p, data_direction, data, datasz, next_pid, &num_bytes);
      if (err == ERR_OK)
        break;
      printf("ERR: %d\n", err);

      if (err == ERR_RETRY) {
        MASSERR("RETRYING");
        wait_usec(900);
        continue;
      }

      MASSERR("transfer failed at data stage");
      goto out_err;
    }
    if (usb_mass_log_level) {
      MASSDBG("transfer complete: ");
      hexdump_memory(data, datasz);
    }
  }

  p.endpoint = hcd_endpoint_get_number(ep_in);
  next_pid = &ep_in->next_toggle_pid;
  err = hcd_transfer_bulk(&p, USB_DIRECTION_IN, &status, sizeof(status), next_pid, &num_bytes);
  CHECK_ERR("transfer failed to CSW recieve");

  if (!is_csw_valid(&status, num_bytes, tag)) {
    MASSERR("recieved CSW not valid");
    goto out_reset_recovery;
  }
  if (!is_csw_meaningful(&status, datasz)) {
    MASSERR("recieved CSW not meaningful");
    goto out_reset_recovery;
  }
  if (status.csw_status == CSW_STATUS_PHASE_ERR) {
    MASSERR("device responded with PHASE_ERROR");
    goto out_reset_recovery;
  }

  if (usb_mass_log_level)
    csw_print(&status);

out_err:
  return err;

out_reset_recovery:
  err = usb_mass_reset_recovery(ep_out, ep_in);
  CHECK_ERR("Failed to perform reset recovery");
  err = ERR_RETRY;
  return err;
}

int usb_mass_read_capacity10(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in, int lun)
{
  int err;
  struct scsi_op_read_capacity_10 cmd ALIGNED(4) = { 0 };
  struct scsi_read_capacity_param param ALIGNED(4);
  cmd.opcode = SCSI_OPCODE_READ_CAPACITY_10;

  MASSLOG("READ CAPACITY(10)");
  err = usb_cbw_transfer(ep_out, ep_in, 3, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, &param, sizeof(param));
  CHECK_ERR("READ CAPACITY(10) failed");
out_err:
  return err;
}

int usb_mass_read10(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in,
  int lun, uint32_t offset, void *data_dst, int data_sz)
{
  int err;
  struct scsi_op_read_10 cmd ALIGNED(4) = { 0 };
  cmd.opcode = SCSI_OPCODE_READ_10;
  set_unaligned_32_le(&cmd.lba, offset);
  set_unaligned_16_le(&cmd.transfer_len, data_sz / 512);
  MASSLOG("READ(10)");

  memset(data_dst, 0, data_sz);
  err = usb_cbw_transfer(ep_out, ep_in, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, data_dst, data_sz);
  CHECK_ERR("READ(10) command failed");
out_err:
  return err;
}

int usb_mass_write10(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in,
  int lun, uint32_t offset, void *data_dst, int data_sz)
{
  int err;
  struct scsi_op_write_10 cmd ALIGNED(4) = { 0 };
  cmd.opcode = SCSI_OPCODE_WRITE_10;
  set_unaligned_32_le(&cmd.lba, offset);
  set_unaligned_16_le(&cmd.transfer_len, data_sz / 512);
  MASSLOG("WRITE(10)");
  err = usb_cbw_transfer(ep_out, ep_in, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_OUT, data_dst, data_sz);
  CHECK_ERR("WRITE(10) command failed");
out_err:
  return err;
}

int usb_mass_test_unit_ready(
  struct usb_hcd_endpoint *ep_out,
  struct usb_hcd_endpoint *ep_in,
  int lun)
{
  int err;
  struct scsi_op_test_unit_ready cmd ALIGNED(4) = { 0 };
  cmd.opcode = SCSI_OPCODE_TEST_UNIT_READY;
  MASSLOG("TEST UNIT READY: cmd size: %d", sizeof(cmd));
  err = usb_cbw_transfer(ep_out, ep_in, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, NULL, 0);
  CHECK_ERR("TEST UNIT READY failed");
out_err:
  return err;
}

int usb_mass_inquiry(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in, int lun)
{
  int err;
  char buf[128] ALIGNED(4);
  char description[256];
  struct scsi_op_inquiry cmd;
  struct scsi_response_inquiry *info = (struct scsi_response_inquiry *)buf;
  const int min_cmd_len = 5;

  memset(&cmd, 0, sizeof(cmd));
  memset(buf , 0, sizeof(buf));

  scsi_fill_cmd_inquiry(&cmd, 0, 0, min_cmd_len);
  err = usb_cbw_transfer(ep_out, ep_in, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, min_cmd_len);
  CHECK_ERR("Failed to get INQUIRY data length");
  scsi_fill_cmd_inquiry(&cmd, 0, 0, info->additional_length);
  err = usb_cbw_transfer(ep_out, ep_in, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, info->additional_length);
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
	struct usb_hcd_pipe_control pctl;
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

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  err = hcd_transfer_control(&dev->pipe0, &pctl, 
      &max_lun, 1, rq.raw, 1000, &num_bytes);
  if (err) {
    HCDERR("failed to read descriptor header");
    goto out_err;
  }
out_err:
  HCDLOG("max_lun: %d", max_lun);
  *out_max_lun = max_lun;
  return err;
}

int usb_mass_init(struct usb_hcd_device* dev)
{
  int err;
  int max_lun = 0;
  int lun = 0;
  char readbuf[2048];
  struct usb_hcd_interface *i;
  struct usb_hcd_endpoint *ep_in = NULL;
  struct usb_hcd_endpoint *ep_out = NULL;
  struct usb_hcd_endpoint *ep_tmp = NULL;

  i = hcd_device_get_interface(dev, 0);
  if (IS_ERR(i)) {
    MASSLOG("Failed to get inteface");
    goto out_err;
  }

  ep_tmp = hcd_interface_get_endpoint(i, 0);
  if (hcd_endpoint_get_direction(ep_tmp) == USB_DIRECTION_OUT)
    ep_out = ep_tmp;
  else
    ep_in = ep_tmp;

  ep_tmp = hcd_interface_get_endpoint(i, 1);
  if (hcd_endpoint_get_direction(ep_tmp) == USB_DIRECTION_OUT)
    ep_out = ep_tmp;
  else
    ep_in = ep_tmp;

  MASSLOG("usb_mass_storage_init: OUT:ep%d, IN:ep%d", 
    hcd_endpoint_get_number(ep_out),
    hcd_endpoint_get_number(ep_in));

  // usb_hcd_set_log_level(1);
  // dwc2_set_log_level(2);
  err = usb_mass_reset(ep_out, ep_in);
  CHECK_ERR("failed to reset mass storage device");

  err = usb_mass_get_max_lun(dev, &max_lun);
  CHECK_ERR("failed to get max lun");
  usb_mass_set_log_level(1);
  err = usb_mass_test_unit_ready(ep_out, ep_in, lun);
  CHECK_ERR("failed to test unit ready");
  err = usb_mass_inquiry(ep_out, ep_in, lun);
  CHECK_ERR("failed to send INQUIRY request");
  err = usb_mass_inquiry(ep_out, ep_in, lun);
  CHECK_ERR("failed to send INQUIRY request");
  err = usb_mass_read_capacity10(ep_out, ep_in, lun);
  memset(readbuf, 0, 2048);

  err = usb_mass_read10(ep_out, ep_in, lun, 0, readbuf, 512);
  hexdump_memory(readbuf, 512);
  memset(readbuf, 0xfc, 512);
  memset(readbuf + 128, 0xee, 128);
  memset(readbuf + 512 - 4, 0x55, 4);
  err = usb_mass_write10(ep_out, ep_in, lun, 0, readbuf, 512);
  memset(readbuf, 0, 512);
  err = usb_mass_read10(ep_out, ep_in, lun, 0, readbuf, 512);
  hexdump_memory(readbuf, 512);
out_err:
  while(1);
  return err;
}

int usb_mass_set_log_level(int level)
{
  int old_level = usb_mass_log_level;
  usb_mass_log_level = level;
  return old_level;
}
