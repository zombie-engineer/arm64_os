#include <drivers/usb/hcd.h>
#include <drivers/usb/usb_dev_rq.h>
#include "root_hub.h"
#include "dwc2.h"
#include "dwc2_regs.h"

static inline void hcd_transfer_control_prologue(struct usb_hcd_pipe *pipe, uint64_t rq)
{
  char rq_desc[256];
  if (usb_hcd_log_level) {
    usb_rq_get_description(rq, rq_desc, sizeof(rq_desc));
    HCDDEBUG("SUBMIT: device_address:%d, max_packet:%d, ep:%d, req:%s", pipe->address, pipe->max_packet_size, pipe->endpoint, rq_desc);
  }
}

static int __hcd_dwc2_transfer(dwc2_pipe_desc_t pipedesc, void *buf, int bufsz, usb_pid_t *pid, const char *debug_desc, int *out_num_bytes)
{
  int err = ERR_OK;
  int num_bytes;
  dwc2_transfer_status_t status;
  while(1) {
    status = dwc2_transfer(pipedesc, buf, bufsz, pid, &num_bytes);
    if (status == DWC2_STATUS_ACK)
      break;
    if (status == DWC2_STATUS_NAK)
      continue;
    if (status == DWC2_STATUS_NYET)
      continue;
    err = ERR_GENERIC;
    break;
  }
  CHECK_ERR("dwc2_transfer failed");

  HCDDEBUG("%s:%s sent %d of %d bytes", usb_direction_to_string(pipedesc.u.ep_direction), debug_desc, num_bytes, bufsz);
out_err:
  if (out_num_bytes)
    *out_num_bytes = bufsz;
  return err;
}

int hcd_transfer_control(
  struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl,
  void *buf,
  int bufsz,
  uint64_t rq,
  int timeout,
  int *out_num_bytes)
{
  int err;
  int channel;
  int num_bytes = 0;
  uint64_t rqbuf ALIGNED(8) = rq;
  usb_pid_t pid;

  dwc2_pipe_desc_t pipedesc = {
    .u = {
      .device_address  = pipe->address,
      .ep_address      = pipe->endpoint,
      .ep_type         = USB_ENDPOINT_TYPE_CONTROL,
      .ep_direction    = USB_DIRECTION_OUT,
      .speed           = pipe->speed,
      .max_packet_size = pipe->max_packet_size,
      .dwc_channel     = pctl->channel,
      .hub_address     = pipe->ls_hub_address,
      .hub_port        = pipe->ls_hub_port
    }
  };

  hcd_transfer_control_prologue(pipe, rq);

  if (pipe->address == usb_root_hub_device_number) {
    err = usb_root_hub_process_req(rq, buf, bufsz, &num_bytes);
    goto out_err;
  }

  /*
   * Send SETUP packet
   */
  pid = USB_PID_SETUP;
  err = __hcd_dwc2_transfer(pipedesc, &rqbuf, sizeof(rqbuf), &pid, "SETUP", NULL);
  CHECK_ERR_SILENT();

  /*
   * Transmit DATA packet
   */
  if (buf) {
    pid = USB_PID_DATA1;
    pipedesc.u.ep_direction = pctl->direction;
    err = __hcd_dwc2_transfer(pipedesc, buf, bufsz, &pid, "DATA", &num_bytes);
    CHECK_ERR_SILENT();
  }

  /*
   * Transmit STATUS packet
   */
  if (!buf || pctl->direction == USB_DIRECTION_OUT)
    pipedesc.u.ep_direction = USB_DIRECTION_IN;
  else
    pipedesc.u.ep_direction = USB_DIRECTION_OUT;

  pid = USB_PID_DATA1;
  err = __hcd_dwc2_transfer(pipedesc, 0, 0, &pid, "ACK", NULL);
  CHECK_ERR_SILENT();

out_err:
  if (out_num_bytes)
    *out_num_bytes = num_bytes;

  HCDDEBUG("SUBMIT: completed with status: %d", err);
  return err;
}

int hcd_transfer_interrupt(
  struct usb_hcd_pipe *pipe,
  void *buf,
  int buf_sz,
  int timeout,
  int *out_num_bytes)
{
  dwc2_transfer_status_t status;
  int err;

  dwc2_pipe_desc_t pipedesc = {
    .u = {
      .device_address  = pipe->address,
      .ep_address      = pipe->endpoint,
      .ep_type         = USB_ENDPOINT_TYPE_INTERRUPT,
      .ep_direction    = USB_DIRECTION_IN,
      .speed           = pipe->speed,
      .max_packet_size = pipe->max_packet_size,
      .dwc_channel     = 0,
      .hub_address     = pipe->ls_hub_address,
      .hub_port        = pipe->ls_hub_port
    }
  };
  status = dwc2_transfer(pipedesc, buf, buf_sz, USB_PID_DATA0, out_num_bytes);
  switch(status) {
    case DWC2_STATUS_ACK: err = ERR_OK; break;
    case DWC2_STATUS_NAK: err = ERR_RETRY; break;
    default: err = ERR_GENERIC; break;
  }
  CHECK_ERR("a");
out_err:
  return err;
}

int hcd_transfer_bulk(
  struct usb_hcd_pipe *pipe,
  int direction,
  void *buf,
  int buf_sz,
  usb_pid_t *pid,
  int *out_num_bytes)
{
  int err = ERR_OK;
  dwc2_transfer_status_t status;

  dwc2_pipe_desc_t pipedesc = {
    .u = {
      .device_address  = pipe->address,
      .ep_address      = pipe->endpoint,
      .ep_type         = USB_ENDPOINT_TYPE_BULK,
      .ep_direction    = direction,
      .speed           = pipe->speed,
      .max_packet_size = pipe->max_packet_size,
      .dwc_channel     = 0,
      .hub_address     = pipe->ls_hub_address,
      .hub_port        = pipe->ls_hub_port
    }
  };
  status = dwc2_transfer(pipedesc, buf, buf_sz, pid, out_num_bytes);
  if (status != DWC2_STATUS_ACK) {
    if (status == DWC2_STATUS_NAK) {
      err = ERR_RETRY;
    } else
      err = ERR_GENERIC;
  }
  CHECK_ERR("a");
out_err:
  return err;
}
