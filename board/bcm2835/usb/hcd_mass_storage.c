#include <drivers/usb/usb_mass_storage.h>
#include <common.h>
#include <drivers/usb/usb_dev_rq.h>
#include "usb_mass_cbw.h"
#include <scsi/scsi_opcode.h>
#include <mem_access.h>
#include <delays.h>

#define __MASS_PREFIX "[USB_MASS: %d] "
#define __MASS_ARGS 0

#define MASS_REQUEST_RESET       0xff
#define MASS_REQUEST_GET_MAX_LUN 0xfe

#define MASSLOG(__fmt, ...) printf(__MASS_PREFIX __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)

#define USB_HCTSIZ0_PID_DATA0 0
#define USB_HCTSIZ0_PID_DATA1 2
#define USB_HCTSIZ0_PID_DATA2 1
#define USB_HCTSIZ0_PID_SETUP 3

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
  op->allocation_length = allocation_length;
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

int usb_mass_reset(struct usb_hcd_device *dev)
{
  int err;
  int num_bytes;
	struct usb_hcd_pipe_control pctl;
  struct usb_hcd_interface *i;
  struct usb_device_request rq = { 
    .request_type = USB_RQ_TYPE_CLASS_SET_INTERFACE,
    .request      = MASS_REQUEST_RESET,
    .value        = 0,
    .index        = 0,
    .length       = 0,
  };

  MASSLOG("usb_mass_reset: %016x", rq.raw);

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_OUT;

  err = hcd_transfer_control(&dev->pipe0, &pctl, 
      NULL, 0, rq.raw, 1000, &num_bytes);
  if (err) {
    HCDERR("failed to send reset");
    goto out_err;
  }

  if (!dev->num_interfaces) {
    HCDERR("failed to clear HALT bit on device without intefaces");
    err = ERR_GENERIC;
    goto out_err;
  }

  i = &dev->interfaces[0];
  if (hcd_interface_get_num_endpoints(i) < 2) {
    HCDERR("failed to clear HALT bit on device without 2 endpoints");
    err = ERR_GENERIC;
    goto out_err;
  }

  err = hcd_endpoint_clear_feature(&i->endpoints[0], USB_ENDPOINT_FEATURE_HALT);
  CHECK_ERR("failed to clear HALT feature on endpoint");

  err = hcd_endpoint_clear_feature(&i->endpoints[1], USB_ENDPOINT_FEATURE_HALT);
  CHECK_ERR("failed to clear HALT feature on endpoint");

out_err:
  MASSLOG("usb_mass_reset complete err: %d", err);
  return err;
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

  char cmdbuf[31];

  usb_pid_t *next_pid = &ep_out->next_toggle_pid;

  memset(cmdbuf, 0, sizeof(cmdbuf));
  if (sizeof(cbw) + cmdsz > sizeof(cmdbuf))
    kernel_panic("no way");

  memcpy(cmdbuf, &cbw, sizeof(cbw));
  memcpy(cmdbuf + sizeof(cbw), cmd, cmdsz);

  err = hcd_transfer_bulk(&p, USB_DIRECTION_OUT, cmdbuf, sizeof(cmdbuf), next_pid, &num_bytes);
  CHECK_ERR("failed to send CBW");
  if (data && datasz) {
    if (data_direction == USB_DIRECTION_IN) {
      p.endpoint = hcd_endpoint_get_number(ep_in);
      next_pid = &ep_in->next_toggle_pid;
    }
    err = hcd_transfer_bulk(&p, data_direction, data, datasz, next_pid, &num_bytes);
    CHECK_ERR("failed to transfer data");
  }

  p.endpoint = hcd_endpoint_get_number(ep_in);
  next_pid = &ep_in->next_toggle_pid;
  err = hcd_transfer_bulk(&p, USB_DIRECTION_IN, &status, sizeof(status), next_pid, &num_bytes);
  CHECK_ERR("failed to get CSW");
  csw_print(&status);
out_err:
  return err;
}

int usb_mass_read_capacity10(struct usb_hcd_endpoint *ep_out, struct usb_hcd_endpoint *ep_in, int lun, void *buf, int bufsz)
{
  int err;
  struct scsi_op_read_capacity_10 cmd ALIGNED(4) = { 0 };
  cmd.opcode = SCSI_OPCODE_READ_10;

  MASSLOG("READ CAPACITY");
  err = usb_cbw_transfer(ep_out, ep_in, 2, lun, &cmd, sizeof(cmd), USB_DIRECTION_IN, buf, bufsz);
  CHECK_ERR("READ CAPACITY failed");
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
  int num_bytes;
  const char SCSI_CMD_LENGTH = 6;
  char cbwbuf[31] ALIGNED(4);
  struct cbw *c = (struct cbw*) cbwbuf;
  char buf[128] ALIGNED(4);

  struct scsi_response_inquiry response ALIGNED(4);
  struct scsi_op_inquiry *op = (struct scsi_op_inquiry *)(cbwbuf + sizeof(*c));

  struct csw status ALIGNED(4);

  struct usb_hcd_pipe p ALIGNED(4) = {
    .address         = hcd_endpoint_get_address(ep_out),
    .endpoint        = hcd_endpoint_get_number(ep_out),
    .speed           = ep_out->device->pipe0.speed,
    .max_packet_size = hcd_endpoint_get_max_packet_size(ep_out),
    .ls_hub_port     = ep_out->device->pipe0.ls_hub_port,
    .ls_hub_address  = ep_out->device->pipe0.ls_hub_address
  };

  usb_pid_t *next_pid = &ep_out->next_toggle_pid;

  if (sizeof(cbwbuf) != 31)
    kernel_panic("should be 31 bytes");

  memset(cbwbuf   , 0, sizeof(cbwbuf));
  memset(&response, 0, sizeof(response));
  memset(&status  , 0, sizeof(status));
  memset(buf      , 0, sizeof(buf));

  c->cbw_signature   = CBW_SIGNATURE;
  c->cbw_tag         = 1;
  c->cbw_data_length = 95;
  c->cbw_flags       = cbw_make_flags(USB_DIRECTION_IN);
  c->cbw_lun         = lun;
  c->cbw_length      = SCSI_CMD_LENGTH;

  scsi_fill_cmd_inquiry(op, 0, 0, 95);
  MASSLOG("INQUIRY:SEND CBW");
  err = hcd_transfer_bulk(&p, USB_DIRECTION_OUT, cbwbuf, sizeof(cbwbuf), next_pid, &num_bytes);
  CHECK_ERR("failed to send CBW");
  p.endpoint = 1;
  MASSLOG("INQUIRY:RECV DATA");
  next_pid = &ep_in->next_toggle_pid;
  err = hcd_transfer_bulk(&p, USB_DIRECTION_IN, buf, 95/*sizeof(response)*/, next_pid, &num_bytes);
  CHECK_ERR("failed to recieve CBW");
  {
    char description[256];
    scsi_inquiry_data_to_string(
      description, sizeof(description), 
      (struct scsi_response_inquiry *)buf);
    printf("%s\r\n", description);
  }
  MASSLOG("INQUIRY:RECV CSW size:%d", sizeof(status));
  err = hcd_transfer_bulk(&p, USB_DIRECTION_IN, &status, sizeof(status), next_pid, &num_bytes);
  CHECK_ERR("failed to recieve CSW");
  csw_print(&status);
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

int cbw_transfer(struct usb_hcd_pipe *d, int dir, void *buf, int bufsz)
{
  return 0;
}

int usb_mass_storage_init(struct usb_hcd_device* dev)
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

  err = usb_mass_reset(dev);
  CHECK_ERR("failed to reset mass storage device");

  err = usb_mass_get_max_lun(dev, &max_lun);
  CHECK_ERR("failed to get max lun");
  dwc2_set_log_level(2);
  err = usb_mass_test_unit_ready(ep_out, ep_in, lun);
  err = usb_mass_inquiry(ep_out, ep_in, lun);
  CHECK_ERR("failed to send INQUIRY request");
  err = usb_mass_inquiry(ep_out, ep_in, lun);
  CHECK_ERR("failed to send INQUIRY request");
  while(1);
  CHECK_ERR("failed to test unit ready");
  //err = usb_mass_read_capacity10(dev, 0, readbuf, 512);
  // hexdump_memory(readbuf, 512);
  while(1) {
    memset(readbuf, 0, 2048);
    err = usb_mass_read10(ep_out, ep_in, lun, 0, readbuf, 512);
    hexdump_memory(readbuf, 512);
  }
  memset(readbuf, 0, 2048);
  err = usb_mass_read10(ep_out, ep_in, lun, 1, readbuf, 512);
  hexdump_memory(readbuf, 512);
out_err:
  while(1);
  return err;
}
