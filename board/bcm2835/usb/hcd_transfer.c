#include <drivers/usb/hcd.h>
#include <drivers/usb/usb_dev_rq.h>
#include "root_hub.h"
#include "dwc2.h"
#include "dwc2_regs.h"
#include "dwc2_channel.h"
#include <delays.h>
#include <drivers/usb/usb_xfer_queue.h>

static inline void hcd_transfer_control_prologue(struct usb_hcd_pipe *pipe, uint64_t rq)
{
  char rq_desc[256];
  if (usb_hcd_log_level) {
    usb_rq_get_description(rq, rq_desc, sizeof(rq_desc));
    HCDDEBUG("SUBMIT: device_address:%d, max_packet:%d, ep:%d, req:%s", pipe->address, pipe->max_packet_size, pipe->ep, rq_desc);
  }
}

static int hcd_transfer_packet_blocking(struct dwc2_channel *c, const char *debug_desc, int *out_num_bytes)
{
  int err = ERR_OK;
  int num_bytes;
  dwc2_transfer_status_t status;
  while(1) {
    status = dwc2_transfer_blocking(c, &num_bytes);
    // printf(">>> status :%d, %d\n", status, num_bytes);
    if (status == DWC2_STATUS_ACK)
      break;
    if (status == DWC2_STATUS_NAK)
      continue;
    if (status == DWC2_STATUS_NYET) {
      printf("NOT YET: addr = %p, transfer_size = %d\n", c->ctl->dma_addr_base, c->ctl->transfer_size);
      continue;
    }
    err = ERR_GENERIC;
    break;
  }
  CHECK_ERR("dwc2_transfer failed");

  HCDDEBUG("%s:%s transferred %d of %d bytes", usb_direction_to_string(c->ctl->direction), debug_desc, num_bytes, c->ctl->transfer_size);
out_err:
  if (out_num_bytes)
    *out_num_bytes = num_bytes;

  return err;
}

static inline struct usb_xfer_job *usb_xfer_job_prep(int pid, int dir, void *addr, int transfer_size)
{
  struct usb_xfer_job *j;
  j = usb_xfer_job_alloc();
  if (IS_ERR(j))
    return j;

  memset(j, 0, sizeof(*j));
  j->pid = pid;
  j->addr = addr;
  j->transfer_size = transfer_size;
  j->direction = dir;
  return j;
}

static inline struct usb_xfer_jobchain *usb_xfer_jobchain_prep_control(uint64_t *request, int direction, char *addr, int transfer_size)
{
  int err;
  int ack_direction;
  struct usb_xfer_jobchain *jc;
  struct usb_xfer_job *last_j, *j;

  jc = usb_xfer_jobchain_alloc();
  if (IS_ERR(jc))
    return jc;

  memset(jc, 0, sizeof(*jc));

  /* SETUP packet */
  j = usb_xfer_job_prep(USB_PID_SETUP, USB_DIRECTION_OUT, request, sizeof(*request));
  if (IS_ERR(j)) {
    err = PTR_ERR(j);
    goto out_err;
  }
  jc->first = last_j = j;
  usb_xfer_job_print(last_j, "usb_xfer_job_prep SETUP");

  /* DATA packet */
  if (addr) {
    j = usb_xfer_job_prep(USB_PID_DATA1, direction, addr, transfer_size);
    if (IS_ERR(j)) {
      err = PTR_ERR(j);
      goto out_err;
    }
    last_j->next = j;
    last_j = j;
  }
  usb_xfer_job_print(last_j, "usb_xfer_job_prep DATA");

  /* STATUS packet */
  if (addr && direction == USB_DIRECTION_IN)
    ack_direction = USB_DIRECTION_OUT;
  else
    ack_direction = USB_DIRECTION_IN;

  j = usb_xfer_job_prep(USB_PID_DATA1, ack_direction, NULL, 0);
  if (IS_ERR(j)) {
    err = PTR_ERR(j);
    goto out_err;
  }
  last_j->next = j;
  usb_xfer_job_print(last_j, "usb_xfer_job_prep ACK");
  return jc;

out_err:
  if (jc) {
    struct usb_xfer_job *next;
    next = j = jc->first;
    while (next) {
      next = j->next;
      usb_xfer_job_free(j);
      j = next;
    }
  }
  return ERR_PTR(err);
}

static void control_chain_signal_completed(void *arg)
{
  int *completed = arg;
  *completed = 1;
}

int hcd_transfer_control(
  struct usb_hcd_pipe *pipe,
  int direction,
  void *addr,
  int transfer_size,
  uint64_t rq,
  int *out_num_bytes)
{
  int err;
  int completed ALIGNED(8) = 0;
  struct usb_xfer_jobchain *jc;
  uint64_t rqbuf ALIGNED(64) = rq;

  if (pipe->address == usb_root_hub_device_number)
    return usb_root_hub_process_req(rq, addr, transfer_size, out_num_bytes);

  printf("hcd_transfer_control, pipe:%p, hub_port:%d, speed:%d\n", pipe, pipe->ls_hub_port, pipe->speed);

  jc = usb_xfer_jobchain_prep_control(&rqbuf, direction, addr, transfer_size);
  if (IS_ERR(jc)) {
    err = PTR_ERR(jc);
    goto out_err;
  }
  uxb_xfer_jobchain_print(jc, "control job chain");
  jc->completion = control_chain_signal_completed;
  jc->completion_arg = &completed;
  jc->hcd_pipe = pipe;

  usb_xfer_jobchain_enqueue(jc);

  while(!completed)
    asm volatile("wfe");
  err = jc->err;
  *out_num_bytes = transfer_size;
out_err:
  HCDDEBUG("control transfer completed with, err = %d", err);
  return err;
}

int hcd_transfer_control_blocking(
  struct usb_hcd_pipe *pipe,
  int direction,
  void *addr,
  int transfer_size,
  uint64_t rq,
  int *out_num_bytes)
{
  struct dwc2_channel *c;
  int err;
  uint64_t rqbuf ALIGNED(64) = rq;
  DECL_PIPE_DESC(pipedesc, pipe);
  hcd_transfer_control_prologue(pipe, rq);
  if (pipe->address == usb_root_hub_device_number)
    return usb_root_hub_process_req(rq, addr, transfer_size, out_num_bytes);

  c = dwc2_channel_alloc();
  if (!c) {
    err = ERR_RETRY;
    HCDERR("channel not allocated. Retry");
    goto out_err;
  }

  c->ctl = dwc2_xfer_control_create();
  if (!c->ctl) {
    err = ERR_MEMALLOC;
    HCDERR("tranfer control structure not allocated.");
    goto out_err;
  }

  c->pipe = pipedesc;

  if (out_num_bytes)
    *out_num_bytes = 0;

  /*
   * Send SETUP packet
   */
  c->next_pid = USB_PID_SETUP;
  c->ctl->dma_addr_base = (uint64_t)&rqbuf;
  c->ctl->transfer_size = sizeof(rqbuf);
  c->ctl->direction = USB_DIRECTION_OUT;
  err = hcd_transfer_packet_blocking(c, "SETUP", NULL);
  CHECK_ERR_SILENT();

  /*
   * Transmit DATA packet
   */
  if (addr) {
    c->next_pid = USB_PID_DATA1;
    c->ctl->dma_addr_base = (uint64_t)addr;
    c->ctl->transfer_size = transfer_size;
    c->ctl->direction = direction;
    err = hcd_transfer_packet_blocking(c, "DATA", out_num_bytes);
    CHECK_ERR_SILENT();
  }

  /*
   * Transmit STATUS packet
   */
  if (!addr || direction == USB_DIRECTION_OUT)
    c->ctl->direction = USB_DIRECTION_IN;
  else
    c->ctl->direction = USB_DIRECTION_OUT;

  c->next_pid = USB_PID_DATA1;
  c->ctl->dma_addr_base = 0;
  c->ctl->transfer_size = 0;
  err = hcd_transfer_packet_blocking(c, "ACK", NULL);
  CHECK_ERR_SILENT();

out_err:
  if (c) {
    dwc2_channel_disable(c->id);
    if (c->ctl)
      dwc2_xfer_control_destroy(c->ctl);
    dwc2_channel_free(c);
  }

  HCDDEBUG("SUBMIT: completed with status: %d", err);
  return err;
}

int hcd_transfer_interrupt(
  struct usb_hcd_pipe *pipe,
  void *addr,
  int transfer_size,
  int *out_num_bytes)
{
  struct dwc2_channel *c;
  dwc2_transfer_status_t status;
  int err;

  dwc2_pipe_desc_t pipedesc = {
    .u = {
      .device_address  = pipe->address,
      .ep_address      = pipe->ep,
      .ep_type         = USB_ENDPOINT_TYPE_INTERRUPT,
      .speed           = pipe->speed,
      .max_packet_size = pipe->max_packet_size,
      .dwc_channel     = 0,
      .hub_address     = pipe->ls_hub_address,
      .hub_port        = pipe->ls_hub_port
    }
  };

  c = dwc2_channel_alloc();
  if (!c) {
    err = ERR_RETRY;
    HCDERR("channel not allocated. Retry");
    goto out_err;
  }

  c->ctl = dwc2_xfer_control_create();
  if (!c->ctl) {
    err = ERR_MEMALLOC;
    HCDERR("tranfer control structure not allocated.");
    goto out_err;
  }

  c->pipe = pipedesc;
  c->next_pid = USB_PID_DATA0;
  c->ctl->dma_addr_base = (uint64_t)addr;
  c->ctl->transfer_size = transfer_size;
  c->ctl->direction = USB_DIRECTION_OUT;
  status = dwc2_transfer_blocking(c, out_num_bytes);
  switch(status) {
    case DWC2_STATUS_ACK: err = ERR_OK; break;
    case DWC2_STATUS_NAK: err = ERR_RETRY; break;
    default: err = ERR_GENERIC; break;
  }
  CHECK_ERR("a");
out_err:
  if (c) {
    dwc2_channel_disable(c->id);
    if (c->ctl)
      dwc2_xfer_control_destroy(c->ctl);
    dwc2_channel_free(c);
  }
  return err;
}

int hcd_transfer_bulk(
  struct usb_hcd_pipe *pipe,
  int direction,
  void *addr,
  int transfer_size,
  usb_pid_t *pid,
  int *out_num_bytes)
{
  struct dwc2_channel *c;
  int err = ERR_OK;
  dwc2_transfer_status_t status;

  dwc2_pipe_desc_t pipedesc = {
    .u = {
      .device_address  = pipe->address,
      .ep_address      = pipe->ep,
      .ep_type         = USB_ENDPOINT_TYPE_BULK,
      .speed           = pipe->speed,
      .max_packet_size = pipe->max_packet_size,
      .dwc_channel     = 0,
      .hub_address     = pipe->ls_hub_address,
      .hub_port        = pipe->ls_hub_port
    }
  };

  c = dwc2_channel_alloc();
  if (!c) {
    err = ERR_RETRY;
    HCDERR("channel not allocated. Retry");
    goto out_err;
  }

  c->ctl = dwc2_xfer_control_create();
  if (!c->ctl) {
    err = ERR_MEMALLOC;
    HCDERR("tranfer control structure not allocated.");
    goto out_err;
  }

  c->pipe = pipedesc;
  c->next_pid = *pid;
  c->ctl->dma_addr_base = (uint64_t)addr;
  c->ctl->transfer_size = transfer_size;
  c->ctl->direction = direction;
  status = dwc2_transfer_blocking(c, out_num_bytes);
  if (status != DWC2_STATUS_ACK) {
    if (status == DWC2_STATUS_NAK) {
      err = ERR_RETRY;
    } else
      err = ERR_GENERIC;
  }
  *pid = c->next_pid;
  CHECK_ERR("a");
out_err:
  return err;
}
