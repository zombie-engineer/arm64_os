#include <drivers/usb/usb_mass_storage.h>
#include <common.h>
#include <drivers/usb/usb_dev_rq.h>
#include "usb_mass_cbw.h"
#include <scsi/scsi_opcode.h>

#define __MASS_PREFIX "[USB_MASS: %d] "
#define __MASS_ARGS 0

#define MASS_REQUEST_RESET       0xff
#define MASS_REQUEST_GET_MAX_LUN 0xfe

#define MASSLOG(__fmt, ...) printf(__MASS_PREFIX __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)

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

  char    t10_vendor_id[8]       ;   // +8
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

int usb_mass_inquiry(struct usb_hcd_device *dev, int lun)
{
  int err;
  int num_bytes;
  const char SCSI_CMD_LENGTH = 6;
  char cbwbuf[31] ALIGNED(4);
  char cswbuf[36] ALIGNED(4);
  char buf[128] ALIGNED(4);
  struct scsi_response_inquiry response ALIGNED(4);
  struct csw status ALIGNED(4);
  struct cbw *c = (struct cbw*) cbwbuf;
  struct scsi_op_inquiry *op = (struct scsi_op_inquiry *)(cbwbuf + sizeof(*c));
  struct usb_hcd_pipe p ALIGNED(4) = {
    .address = dev->pipe0.address,
    .endpoint = 2,
    .speed = dev->pipe0.speed,
    .max_packet_size = 512,
    .ls_hub_port = dev->pipe0.ls_hub_port,
    .ls_hub_address = dev->pipe0.ls_hub_address
  };

  if (sizeof(cbwbuf) != 31)
    kernel_panic("should be 31 bytes");

  memset(cbwbuf   , 0, sizeof(cbwbuf));
  memset(&response, 0, sizeof(response));
  memset(&status  , 0, sizeof(status));

  c->cbw_signature   = CBW_SIGNATURE;
  c->cbw_tag         = 1;
  c->cbw_data_length = sizeof(response);
  c->cbw_flags       = 0x80;// USB_DIRECTION_IN;
  c->cbw_lun         = lun;
  c->cbw_length      = SCSI_CMD_LENGTH;
  hexdump_memory(c, sizeof(cbwbuf));
#define USB_HCTSIZ0_PID_DATA0 0
#define USB_HCTSIZ0_PID_DATA1 2
#define USB_HCTSIZ0_PID_DATA2 1
#define USB_HCTSIZ0_PID_SETUP 3

  scsi_fill_cmd_inquiry(op, 0, 0, sizeof(response));
  err = hcd_transfer_bulk(&p, USB_DIRECTION_OUT, cbwbuf, sizeof(cbwbuf), USB_HCTSIZ0_PID_DATA0, &num_bytes);
  CHECK_ERR("failed to send CBW");
  p.endpoint = 1;
  memset(buf, 0x66, sizeof(buf));
  err = hcd_transfer_bulk(&p, USB_DIRECTION_IN, buf, sizeof(buf), USB_HCTSIZ0_PID_DATA0, &num_bytes);
  hexdump_memory(buf, sizeof(buf));
  hexdump_memory(&status, sizeof(status));
  CHECK_ERR("failed to send CBW");
out_err:
  while(1);
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

int usb_mass_reset(struct usb_hcd_device *dev)
{
  int err;
  int num_bytes;
	struct usb_hcd_pipe_control pctl;
  struct usb_device_request rq = { 
    .request_type = USB_DEVICE_REQUEST_TYPE(HOST2DEV, CLASS, INTERFACE),
    .request      = MASS_REQUEST_RESET,
    .value        = 0,
    .index        = 0,
    .length       = 0,
  };
  rq.request_type = 0b00100001;
  rq.request = 0xff;
  rq.value = 0;
  rq.index = 0;
  rq.length = 0;

  MASSLOG("usb_mass_reset: %016x", rq.raw);

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  err = hcd_transfer_control(&dev->pipe0, &pctl, 
      NULL, 0, rq.raw, 1000, &num_bytes);
  if (err) {
    HCDERR("failed to read descriptor header");
    goto out_err;
  }
out_err:
  return err;
}

void usb_mass_storage_init(struct usb_hcd_device* dev)
{
  int err;
  int max_lun = 0;
extern int dwc2_print_debug;
  dwc2_print_debug = 2;
  MASSLOG("usb_mass_storage_init");
  err = usb_mass_reset(dev);
  CHECK_ERR("failed to reset mass storage device");

  err = usb_mass_get_max_lun(dev, &max_lun);
  CHECK_ERR("failed to get max lun");
  err = usb_mass_inquiry(dev, 0);
  CHECK_ERR("failed to send INQUIRY request");
out_err:
  while(1);
  return err;
}
