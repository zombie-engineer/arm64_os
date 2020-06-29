#pragma once
#include <memory/static_slot.h>
#include <drivers/usb/hcd.h>
#include <list.h>

struct usb_xfer_job {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_xfer_jobs);
  int direction;
  void *addr;
  int transfer_size;
  struct usb_hcd_pipe *pipe;
  struct usb_xfer_job *next;
};

struct usb_xfer_job *usb_xfer_job_alloc(void);

void usb_xfer_job_free(struct usb_xfer_job *j);

void usb_xfer_link_next(struct usb_xfer_job *j, struct usb_xfer_job *next);

struct usb_xfer_jobchain {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_xfer_jobchains);
  struct list_head list;
  struct usb_xfer_job *fist_job;
  void (*completion)(void*);
  void *completion_arg;
};

struct usb_xfer_jobchain *usb_xfer_jobchain_alloc(void);

void usb_xfer_jobchain_free(struct usb_xfer_jobchain *j);

void usb_xfer_queue_init(void);

