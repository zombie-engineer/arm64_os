#include <drivers/usb/usb_xfer_queue.h>
#include <list.h>
#include <spinlock.h>
#include "dwc2.h"
#include "dwc2_channel.h"
#include "dwc2_xfer_control.h"

DECL_STATIC_SLOT(struct usb_xfer_job, usb_xfer_jobs, 64);
DECL_STATIC_SLOT(struct usb_xfer_jobchain, usb_xfer_jobchains, 64);

struct jobchain_queue {
  struct list_head jc_list;
};

static struct jobchain_queue per_channel_jobchains;

struct usb_xfer_job *usb_xfer_job_alloc(void)
{
  struct usb_xfer_job *job;
  job = usb_xfer_jobs_alloc();
  return job;
}

void usb_xfer_job_free(struct usb_xfer_job* j)
{
  usb_xfer_jobs_release(j);
}

void usb_xfer_link_next(struct usb_xfer_job *j, struct usb_xfer_job *next)
{
  j->next = next;
}

struct usb_xfer_jobchain *usb_xfer_jobchain_alloc(void)
{
  struct usb_xfer_jobchain *jobchain;
  jobchain = usb_xfer_jobchains_alloc();
  return jobchain;
}

void usb_xfer_jobchain_free(struct usb_xfer_jobchain *jc)
{
  usb_xfer_jobchains_release(jc);
}

void usb_xfer_jobchain_enqueue(struct usb_xfer_jobchain *jc)
{
  list_add_tail(&jc->list, &per_channel_jobchains.jc_list);
}

void usb_xfer_jobchain_dequeue(struct usb_xfer_jobchain *jc)
{
  list_del_init(&jc->list);
}

static void usb_xfer_job_cb(void *arg)
{
  struct usb_xfer_job *j = arg;
  printf("usb_xfer_job_cb completed\n");
  j->completed = true;
}

static inline void usb_xfer_one_job(struct usb_xfer_job *j, struct dwc2_channel *c)
{
  uxb_xfer_job_print(j, "usb_xfer_one_job");
  j->completion = usb_xfer_job_cb;
  j->completed = false;
  j->completion_arg = j;
  while(1) {
    j->err = ERR_OK;
    dwc2_xfer_one_job(j, c);
    while(!j->completed && !j->err)
      asm volatile("wfe");

    dwc2_channel_disable(c->id);

    if (j->err != ERR_RETRY)
      break;
    if (j->completed)
      break;
  }
}

static inline void usb_xfer_one_jobchain(struct usb_xfer_jobchain *jc)
{
  struct usb_xfer_job *j, *next_j;
  struct dwc2_channel *c = NULL;
  printf("usb_xfer_one_jobchain: %p\n", jc);
  uxb_xfer_jobchain_print(jc, "running");
  DECL_PIPE_DESC(dwc2_pipe, jc->hcd_pipe);

  while(!c) {
    printf(">");
    c = dwc2_channel_alloc();
  }
  while(!c->ctl) {
    printf("++");
    c->ctl = dwc2_xfer_control_create();
  }
  c->pipe.u.raw = dwc2_pipe.u.raw;

  jc->err = ERR_OK;
  j = jc->first;
  while(j) {
    printf("usb_xfer_one_jobchain: processing next job: %p\n", j);
    usb_xfer_one_job(j, c);
    if (j->err != ERR_OK) {
      jc->err = j->err;
      printf("usb_xfer_one_jobchain: job %p completed with err %d\n", j, j->err);
      break;
    }
    printf("usb_xfer_one_jobchain: job %p completed with success, next_job is %p\n", j, j->next);
    next_j = j->next;
    usb_xfer_job_free(j);
    j = next_j;
  }
  dwc2_xfer_control_destroy(c->ctl);
  dwc2_channel_free(c);
}

void usb_xfer_queue_run()
{
  struct usb_xfer_jobchain *jc, *tmp;
  struct list_head *h = &per_channel_jobchains.jc_list;
  list_for_each_entry_safe(jc, tmp, h, list) {
    printf("usb_xfer_queue_run:head->prev:%p, head->next:%p\n", h->prev, h->next);
    printf("jc->list.prev:%p, next:%p\n", jc->list.prev, jc->list.next);
    usb_xfer_one_jobchain(jc);
    list_del(&jc->list);
    usb_xfer_jobchain_free(jc);
  }
}

void usb_xfer_queue_init(void)
{
  STATIC_SLOT_INIT_FREE(usb_xfer_jobs);
  STATIC_SLOT_INIT_FREE(usb_xfer_jobchains);
  INIT_LIST_HEAD(&per_channel_jobchains.jc_list);
}
