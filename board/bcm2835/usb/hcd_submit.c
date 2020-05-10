#include <drivers/usb/hcd.h>
#include <drivers/usb/usb_dev_rq.h>
#include "root_hub.h"
#include "dwc2.h"
#include "dwc2_regs.h"

static inline void usb_hcd_submit_cm_prologue(struct usb_hcd_pipe *pipe, uint64_t rq)
{
  char rq_desc[256];
  if (usb_hcd_print_debug) {
    usb_rq_get_description(rq, rq_desc, sizeof(rq_desc));
    HCDDEBUG("SUBMIT: device_address:%d, max_packet:%d, ep:%d, req:%s", pipe->address, pipe->max_packet_size, pipe->endpoint, rq_desc);
  }
}

int usb_hcd_submit_cm(
  struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl,
  void *buf,
  int buf_sz,
  uint64_t rq,
  int timeout,
  int *out_num_bytes)
{
  int err;
  int num_bytes = 0;
  int num_bytes_last = 0;

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

  usb_hcd_submit_cm_prologue(pipe, rq);

  if (pipe->address == usb_root_hub_device_number) {
    err = usb_root_hub_process_req(rq, buf, buf_sz, &num_bytes);
    goto out_err;
  }

  /*
   * Send SETUP packet
   */
  err = dwc2_transfer(pipedesc, &rq, sizeof(rq), USB_HCTSIZ0_PID_SETUP, NULL);
  CHECK_ERR_SILENT();
  HCDDEBUG("%s:SETUP sent %d of %d bytes", usb_direction_to_string(pipedesc.u.ep_direction), sizeof(rq), sizeof(rq));

  /*
   * Transmit DATA packet
   */
  if (buf) {
    pipedesc.u.ep_direction = pctl->direction;
    err = dwc2_transfer(pipedesc, buf, buf_sz, USB_HCTSIZ0_PID_DATA1, &num_bytes);
    CHECK_ERR_SILENT();
    HCDDEBUG("%s:DATA transferred %d of %d bytes", usb_direction_to_string(pipedesc.u.ep_direction), num_bytes, buf_sz); 
  }

  /*
   * Transmit STATUS packet
   */
  if (!buf || pctl->direction == USB_DIRECTION_OUT)
    pipedesc.u.ep_direction = USB_DIRECTION_IN;
  else
    pipedesc.u.ep_direction = USB_DIRECTION_OUT;

  err = dwc2_transfer(pipedesc, 0, 0, USB_HCTSIZ0_PID_DATA1, &num_bytes_last);
  CHECK_ERR_SILENT();
  HCDDEBUG("%s:ACK", usb_direction_to_string(pctl->direction));

out_err:
  if (out_num_bytes)
    *out_num_bytes = num_bytes;

  HCDDEBUG("SUBMIT: completed with status: %d", err);
  return ERR_OK;
}

