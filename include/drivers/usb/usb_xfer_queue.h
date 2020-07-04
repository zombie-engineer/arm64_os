#pragma once
#include <memory/static_slot.h>
#include <drivers/usb/hcd.h>
#include <list.h>

#define USBQ_INFO(__fmt, ...)\
  printf("[USBQ INFO] " __fmt __endline, ## __VA_ARGS__)

struct usb_xfer_job {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_xfer_jobs);

  int direction;
  void *addr;
  int transfer_size;
  int pid;

  int err;
  void (*completion)(void*);
  void *completion_arg;
  bool completed;
  struct usb_hcd_pipe *pipe;
  struct usb_xfer_job *next;
};

static inline void usb_xfer_job_print(struct usb_xfer_job *j, const char *tag)
{
  USBQ_INFO("usb_xfer_job:[%s] %p, pid:%d, dir:%d, addr:%p, size:%d, next:%p", tag, j, j->pid, j->direction, j->addr, j->transfer_size, j->next);
}

struct usb_xfer_job *usb_xfer_job_alloc(void);

void usb_xfer_job_free(struct usb_xfer_job *j);

void usb_xfer_link_next(struct usb_xfer_job *j, struct usb_xfer_job *next);

struct hcd_pipe;

struct usb_xfer_jobchain {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_xfer_jobchains);
  struct list_head list;
  int err;
  struct usb_hcd_pipe *hcd_pipe;
  struct usb_xfer_job *first;
  void (*completion)(void*);
  void *completion_arg;
};

static inline void uxb_xfer_jobchain_print(struct usb_xfer_jobchain *jc, const char *tag)
{
  struct usb_xfer_job *j;
  USBQ_INFO("jobchain [%s]: %p", tag, jc);
  for (j = jc->first; j; j = j->next) {
    usb_xfer_job_print(j, tag);
  }
}

struct usb_xfer_jobchain *usb_xfer_jobchain_alloc(void);

void usb_xfer_jobchain_free(struct usb_xfer_jobchain *jc);

void usb_xfer_jobchain_enqueue(struct usb_xfer_jobchain *jc);

void usb_xfer_jobchain_dequeue(struct usb_xfer_jobchain *jc);

void usb_xfer_queue_init(void);

