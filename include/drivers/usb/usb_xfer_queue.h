#pragma once
#include <memory/static_slot.h>
#include <drivers/usb/hcd.h>
#include <list.h>

extern int usb_xfer_log_level;

#define USBQ_INFO(__fmt, ...)\
  printf("[USBQ INFO] " __fmt __endline, ## __VA_ARGS__)

#define USBQ_DEBUG(__fmt, ...)\
  if (usb_xfer_log_level)\
    printf("[USBQ DBG] " __fmt __endline, ## __VA_ARGS__)

#define USBQ_DEBUG2(__fmt, ...)\
  if (usb_xfer_log_level > 1)\
    printf("[USBQ DBG2] " __fmt __endline, ## __VA_ARGS__)

struct usb_xfer_jobchain;

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
  struct list_head jobs;
  struct usb_xfer_jobchain *jc;
};

static inline void usb_xfer_job_print(struct usb_xfer_job *j, const char *tag)
{
  USBQ_INFO("usb_xfer_job:[%s] %p, pid:%d, dir:%d, addr:%p, size:%d, prev:%p, next:%p", 
    tag, j, j->pid, j->direction, j->addr, j->transfer_size, 
    j->jobs.prev, j->jobs.next);
}

struct usb_xfer_job *usb_xfer_job_alloc(void);

void usb_xfer_job_free(struct usb_xfer_job *j);

struct hcd_pipe;

//typedef enum {
//  JOBCHAIN_STATUS_IDLE      = 0,
//  JOBCHAIN_STATUS_RUNNING   = 1,
//  JOBCHAIN_STATUS_COMPLETED = 2
//} jobchain_status_t;

struct usb_xfer_jobchain {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_xfer_jobchains);
  struct list_head list;
  // jobchain_status_t status;
  int err;
  struct usb_hcd_pipe *hcd_pipe;
  struct list_head jobs;
  struct list_head completed_jobs;
  void (*completed)(void*);
  void *completed_arg;
  void *channel;
};

static inline void uxb_xfer_jobchain_print(struct usb_xfer_jobchain *jc, const char *tag)
{
  struct usb_xfer_job *j;
  USBQ_INFO("jobchain [%s]: %p, pipe:%p, speed:%d, list.prev:%p,list.next:%p", tag, jc,
    jc, jc->hcd_pipe, jc->hcd_pipe->speed,
    jc->list.prev, jc->list.next);

  list_for_each_entry(j, &jc->jobs, jobs)
    usb_xfer_job_print(j, tag);
}

struct usb_xfer_jobchain *usb_xfer_jobchain_alloc(void);

void usb_xfer_jobchain_free(struct usb_xfer_jobchain *jc);

void usb_xfer_jobchain_destroy(struct usb_xfer_jobchain *jc);

void usb_xfer_jobchain_enqueue(struct usb_xfer_jobchain *jc);

struct usb_xfer_jobchain *usb_xfer_jobchain_dequeue(void);

void usb_xfer_queue_init(void);

