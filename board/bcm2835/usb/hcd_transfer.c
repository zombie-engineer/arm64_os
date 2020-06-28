#include <drivers/usb/hcd.h>
#include <drivers/usb/usb_dev_rq.h>
#include "root_hub.h"
#include "dwc2.h"
#include "dwc2_regs.h"
#include <delays.h>

static inline void hcd_transfer_control_prologue(struct usb_hcd_pipe *pipe, uint64_t rq)
{
  char rq_desc[256];
  if (usb_hcd_log_level) {
    usb_rq_get_description(rq, rq_desc, sizeof(rq_desc));
    HCDDEBUG("SUBMIT: device_address:%d, max_packet:%d, ep:%d, req:%s", pipe->address, pipe->max_packet_size, pipe->endpoint, rq_desc);
  }
}

static int hcd_transfer_packet_blocking(dwc2_pipe_desc_t pipedesc, void *addr, int transfer_size, usb_pid_t *pid, const char *debug_desc, int *out_num_bytes)
{
  int err = ERR_OK;
  int num_bytes;
  dwc2_transfer_status_t status;
  while(1) {
    status = dwc2_transfer_blocking(pipedesc, addr, transfer_size, pid, &num_bytes);
    // printf(">>> status :%d, %d\n", status, num_bytes);
    if (status == DWC2_STATUS_ACK)
      break;
    if (status == DWC2_STATUS_NAK)
      continue;
    if (status == DWC2_STATUS_NYET) {
      printf("NOT YET: addr = %p, transfer_size = %d\n", addr, transfer_size);
      continue;
    }
    err = ERR_GENERIC;
    break;
  }
  CHECK_ERR("dwc2_transfer failed");

  HCDDEBUG("%s:%s transferred %d of %d bytes", usb_direction_to_string(pipedesc.u.ep_direction), debug_desc, num_bytes, transfer_size);
out_err:
  if (out_num_bytes)
    *out_num_bytes = num_bytes;

  return err;
}

void hcd_transfer_fsm(void *arg)
{
  struct hcd_transfer_status *s = arg;
  struct dwc2_channel *c = s->priv;
  printf("hcd_transfer_fsm: %p, stage: %d\n", c, s->stage);

  /*
   * Read current stage, decide on what the next stage is.
   */
  switch(s->stage) {
    case HCD_TRANSFER_STAGE_SETUP_PACKET:
      if (s->addr)
        s->stage = HCD_TRANSFER_STAGE_DATA_PACKET;
      else
        s->stage = HCD_TRANSFER_STAGE_ACK_PACKET;
      break;
    case HCD_TRANSFER_STAGE_DATA_PACKET:
      s->stage = HCD_TRANSFER_STAGE_ACK_PACKET;
      break;
    case HCD_TRANSFER_STAGE_ACK_PACKET:
      s->err = ERR_OK;
      s->stage = HCD_TRANSFER_STAGE_COMPLETED;
      dwc2_transfer_ctl_deallocate(c->tc);
      dwc2_channel_disable(c->pipe.u.dwc_channel);
      dwc2_channel_free(c->pipe.u.dwc_channel);
      return;
      break;
    default:
      BUG(1, "Logic hcd transfer logic error");
      break;
  }
  printf("hcd_transfer_fsm: %p, stage now: %d\n", c, s->stage);

  /*
   * Next stage figured out, now set it up
   */
  switch(s->stage) {
    case HCD_TRANSFER_STAGE_SETUP_PACKET:
      /* can not be */
      break;
    case HCD_TRANSFER_STAGE_DATA_PACKET:
      c->pipe.u.ep_direction = s->direction;
      dwc2_transfer_prepare(c->pipe, s->addr, s->transfer_size, USB_PID_DATA1);
      dwc2_transfer_start(c->pipe);
      break;
    case HCD_TRANSFER_STAGE_ACK_PACKET:
      if (!c->tc->dma_addr_base || s->direction == USB_DIRECTION_OUT)
        c->pipe.u.ep_direction = USB_DIRECTION_IN;
      else
        c->pipe.u.ep_direction = USB_DIRECTION_OUT;
      dwc2_transfer_prepare(c->pipe, NULL, 0, USB_PID_DATA1);
      dwc2_transfer_start(c->pipe);
      break;
    default:
      BUG(1, "hcd_transfer logic at stage action");
  }
}

#define HCD_TRANSFER_COMMON \
  struct dwc2_channel *c;\
  int err; \
  uint64_t rqbuf ALIGNED(64) = rq;\
  usb_pid_t pid ALIGNED(64);\
  int channel_id = DWC2_INVALID_CHANNEL;\
  DECL_PIPE_DESC(pipedesc, pipe, USB_ENDPOINT_TYPE_CONTROL, USB_DIRECTION_OUT);\
  hcd_transfer_control_prologue(pipe, rq);\
  if (pipe->address == usb_root_hub_device_number) { printf("--"); \
    return usb_root_hub_process_req(rq, addr, transfer_size, out_num_bytes); }\
  channel_id = dwc2_channel_alloc();\
  if (channel_id == DWC2_INVALID_CHANNEL) {\
    err = ERR_RETRY;\
    HCDERR("channel not allocated. Retry");\
    goto out_err;\
  }\
  c = dwc2_channel_get(channel_id);\
  if (!c) {\
    err =  ERR_GENERIC;\
    HCDERR("channel not retrieved.");\
    goto out_err;\
  }\
  c->tc = dwc2_transfer_ctl_allocate();\
  if (!c->tc) {\
    err = ERR_MEMALLOC;\
    HCDERR("tranfer control structure not allocated.");\
    goto out_err;\
  }\


int hcd_transfer_control(
  struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl,
  void *addr,
  int transfer_size,
  uint64_t rq,
  int timeout,
  int *out_num_bytes)
{
  struct hcd_transfer_status status ALIGNED(8);
  HCD_TRANSFER_COMMON;

  /*
   * Transfer status setup
   */
  memset(&status, 0, sizeof(status));
  status.priv = c;
  status.stage = HCD_TRANSFER_STAGE_SETUP_PACKET;
  status.direction = pctl->direction;
  status.addr = addr;
  status.transfer_size = transfer_size;

  /*
   * Channel setup
   */
  memset(c->tc, 0, sizeof(*c->tc));
  c->tc->completion = hcd_transfer_fsm;
  c->tc->completion_data = &status;

  dwc2_channel_enable(channel_id);

  pipedesc.u.dwc_channel = channel_id;
  HCDDEBUG("control transfer prepare, c:%p, tc:%p, completion:%p, addr:%p, size:%d", c, c->tc, c->tc->completion, addr, transfer_size);
  pid = USB_PID_SETUP;
  dwc2_transfer_prepare(pipedesc, &rqbuf, sizeof(rqbuf), pid);
  dwc2_transfer_start(pipedesc);
  while(status.stage != HCD_TRANSFER_STAGE_COMPLETED)
    asm volatile("wfe");
  err = status.err;
  *out_num_bytes = transfer_size;
out_err:
  HCDDEBUG("control transfer completed with, err = %d", err);
  return err;
}

int hcd_transfer_control_blocking(
  struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl,
  void *addr,
  int transfer_size,
  uint64_t rq,
  int timeout,
  int *out_num_bytes)
{
  HCD_TRANSFER_COMMON
  if (out_num_bytes)
    *out_num_bytes = 0;

  /*
   * Send SETUP packet
   */
  pid = USB_PID_SETUP;
  err = hcd_transfer_packet_blocking(pipedesc, &rqbuf, sizeof(rqbuf), &pid, "SETUP", NULL);
  CHECK_ERR_SILENT();

  /*
   * Transmit DATA packet
   */
  if (addr) {
    pid = USB_PID_DATA1;
    pipedesc.u.ep_direction = pctl->direction;
    err = hcd_transfer_packet_blocking(pipedesc, addr, transfer_size, &pid, "DATA", out_num_bytes);
    CHECK_ERR_SILENT();
  }

  /*
   * Transmit STATUS packet
   */
  if (!addr || pctl->direction == USB_DIRECTION_OUT)
    pipedesc.u.ep_direction = USB_DIRECTION_IN;
  else
    pipedesc.u.ep_direction = USB_DIRECTION_OUT;

  pid = USB_PID_DATA1;
  err = hcd_transfer_packet_blocking(pipedesc, 0, 0, &pid, "ACK", NULL);
  CHECK_ERR_SILENT();

out_err:
  if (channel_id != DWC2_INVALID_CHANNEL) {
    if (c) {
      if (c->tc) {
        dwc2_transfer_ctl_deallocate(c->tc);
      }
    }
    dwc2_channel_disable(pipedesc.u.dwc_channel);
    dwc2_channel_free(pipedesc.u.dwc_channel);
  }

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
  status = dwc2_transfer_blocking(pipedesc, buf, buf_sz, USB_PID_DATA0, out_num_bytes);
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
  status = dwc2_transfer_blocking(pipedesc, buf, buf_sz, pid, out_num_bytes);
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
