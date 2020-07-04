#include <drivers/usb/usb_xfer_queue.h>
#include <list.h>
#include <spinlock.h>
#include "dwc2.h"
#include "dwc2_channel.h"
#include "dwc2_xfer_control.h"
#include <sched.h>

DECL_STATIC_SLOT(struct usb_xfer_job, usb_xfer_jobs, 64);
DECL_STATIC_SLOT(struct usb_xfer_jobchain, usb_xfer_jobchains, 64);

struct jobchain_queue {
  struct list_head jc_list;
};

static struct jobchain_queue per_channel_jobchains;
static DECL_SPINLOCK(jc_list_lock);

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
  int flags;

  disable_irq_save_flags(flags);
  spinlock_lock(&jc_list_lock);

  list_add_tail(&jc->list, &per_channel_jobchains.jc_list);

  spinlock_unlock(&jc_list_lock);
  restore_irq_flags(flags);
}

void usb_xfer_jobchain_dequeue(struct usb_xfer_jobchain *jc)
{
  int flags;
  disable_irq_save_flags(flags);
  spinlock_lock(&jc_list_lock);

  list_del_init(&jc->list);

  spinlock_unlock(&jc_list_lock);
  restore_irq_flags(flags);
}

static void usb_xfer_job_cb(void *arg)
{
  struct usb_xfer_job *j = arg;
  printf("usb_xfer_job_cb completed\n");
  j->completed = true;
}

static inline void usb_xfer_one_job(struct usb_xfer_job *j, struct dwc2_channel *c)
{
  usb_xfer_job_print(j, "usb_xfer_one_job");
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
  DECL_PIPE_DESC(dwc2_pipe, jc->hcd_pipe);
  printf("usb_xfer_one_jobchain: %p, hcd_pipe:%p, hcd_pipe_speed:%d\n", jc, jc->hcd_pipe, jc->hcd_pipe->speed);
  uxb_xfer_jobchain_print(jc, "running");

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

static int usb_xfer_queue_run(void)
{
  struct usb_xfer_jobchain *jc;
  struct list_head *h = &per_channel_jobchains.jc_list;
  int flags;
  USBQ_INFO("starting usb_runqueue");
  while(1) {
    asm volatile ("wfe");
    jc = NULL;
    disable_irq_save_flags(flags);
    spinlock_lock(&jc_list_lock);
    if (!list_empty(h)) {
      jc = list_first_entry(h, typeof(*jc), list);
      list_del_init(&jc->list);
    }
    spinlock_unlock(&jc_list_lock);
    restore_irq_flags(flags);

    if (jc) {
      USBQ_INFO("usb_xfer_queue_run:head->prev:%p, head->next:%p", h->prev, h->next);
      USBQ_INFO("jc->list.prev:%p, next:%p", jc->list.prev, jc->list.next);
      usb_xfer_one_jobchain(jc);
      usb_xfer_jobchain_free(jc);
    }
  }
  return 0;
}

void usb_xfer_queue_init(void)
{
  task_t *usb_runqueue_task;
  STATIC_SLOT_INIT_FREE(usb_xfer_jobs);
  STATIC_SLOT_INIT_FREE(usb_xfer_jobchains);
  INIT_LIST_HEAD(&per_channel_jobchains.jc_list);
  spinlock_init(&jc_list_lock);

  usb_runqueue_task = task_create(usb_xfer_queue_run, "usb_runqueue");
  BUG(IS_ERR(usb_runqueue_task), "Failed to initiazlie usb_runqueue_task");
  sched_queue_runnable_task(get_scheduler(), usb_runqueue_task);
}
