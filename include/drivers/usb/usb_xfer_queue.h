#pragma once
#include <memory/static_slot.h>
#include <drivers/usb/hcd.h>
#include <list.h>
#include <log.h>

extern int usb_xfer_log_level;

#define USBQ_LOG(__log_level, __fmt, ...)\
  do {\
    if (LOG_LEVEL_ ## __log_level <= usb_xfer_log_level)\
      printf("[USBQ " #__log_level " ] " __fmt __endline, ## __VA_ARGS__);\
  } while(0)

#define USBQ_INFO(__fmt, ...) USBQ_LOG(INFO, __fmt, ## __VA_ARGS__)
#define USBQ_DEBUG(__fmt, ...) USBQ_LOG(DEBUG, __fmt, ## __VA_ARGS__)
#define USBQ_DEBUG2(__fmt, ...) USBQ_LOG(DEBUG2, __fmt, ## __VA_ARGS__)

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

#define usb_xfer_job_print(__log_level, __j, __tag)\
  do {\
    USBQ_INFO("usb_xfer_job:[%s] %p, pid:%d, dir:%d, addr:%p, size:%d, prev:%p, next:%p",\
      __tag, __j, __j->pid, __j->direction, __j->addr, __j->transfer_size,\
      __j->jobs.prev, __j->jobs.next);\
  } while(0)

struct usb_xfer_job *usb_xfer_job_create(void);

void usb_xfer_job_destroy(struct usb_xfer_job *j);

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

#define uxb_xfer_jobchain_print(__log_level, __jc, __tag)\
  do {\
    struct usb_xfer_job *j;\
    USBQ_LOG(__log_level, "jobchain [%s]: %p, pipe:%p, speed:%d, list.prev:%p,list.next:%p",\
      __tag, __jc, __jc->hcd_pipe, __jc->hcd_pipe->speed,\
      __jc->list.prev, __jc->list.next);\
    list_for_each_entry(j, &__jc->jobs, jobs)\
      usb_xfer_job_print(__log_level, j, __tag);\
    list_for_each_entry(j, &__jc->completed_jobs, jobs)\
      usb_xfer_job_print(__log_level, j, __tag);\
} while(0)

struct usb_xfer_jobchain *usb_xfer_jobchain_create(void);

void usb_xfer_jobchain_destroy(struct usb_xfer_jobchain *jc);

void usb_xfer_jobchain_enqueue(struct usb_xfer_jobchain *jc);

struct usb_xfer_jobchain *usb_xfer_jobchain_dequeue(void);

void usb_xfer_queue_init(void);

