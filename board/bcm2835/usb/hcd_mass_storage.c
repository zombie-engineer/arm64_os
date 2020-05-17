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
  const char SCSI_CMD_LENGTH = 6;
  char cswbuf[sizeof(struct cbw) + SCSI_CMD_LENGTH] ALIGNED(4);
  struct scsi_response_inquiry response ALIGNED(4);
  struct cbw *c = (struct cbw*) cswbuf;
  struct scsi_op_inquiry *op = (struct scsi_op_inquiry *)(cswbuf + sizeof(*c));

  memset(cswbuf, 0, sizeof(cswbuf));
  memset(&response, 0, sizeof(response));

  c->cbw_signature   = CBW_SIGNATURE;
  c->cbw_tag         = 1;
  c->cbw_data_length = sizeof(response);
  c->cbw_flags       = USB_DIRECTION_IN;
  c->cbw_lun         = lun;
  c->cbw_length      = SCSI_CMD_LENGTH;

  scsi_fill_cmd_inquiry(op, 0, 0, sizeof(response));
  // err = hcd_transfer_bulk();
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
    .index        = 0,
    .length       = 1,
  };
  char max_lun ALIGNED(4);

  MASSLOG("usb_mass_get_max_lun: %016x", rq.raw);

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  err = usb_hcd_submit_cm(&dev->pipe0, &pctl, 
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
    .index        = 0,
    .length       = 0,
  };

  MASSLOG("usb_mass_reset: %016x", rq.raw);

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  err = usb_hcd_submit_cm(&dev->pipe0, &pctl, 
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
  MASSLOG("usb_mass_storage_init");
  err = usb_mass_reset(dev);
  CHECK_ERR("failed to reset mass storage device");

  err = usb_mass_get_max_lun(dev, &max_lun);
  //err = usb_mass_
out_err:
  return err;
}
