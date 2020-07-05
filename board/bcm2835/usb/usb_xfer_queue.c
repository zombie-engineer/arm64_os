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
static DECL_COND_SPINLOCK(jc_list_lock);

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

void usb_xfer_jobchain_destroy(struct usb_xfer_jobchain *jc)
{
  struct usb_xfer_job *j, *tmp;
  list_for_each_entry_safe(j, tmp, &jc->jobs, jobs) {
    list_del_init(&j->jobs);
    usb_xfer_job_free(j);
  }
}

void usb_xfer_jobchain_enqueue(struct usb_xfer_jobchain *jc)
{
  int irqflags;
  cond_spinlock_lock_disable_irq(&jc_list_lock, irqflags);
  list_add_tail(&jc->list, &per_channel_jobchains.jc_list);
  cond_spinlock_unlock_restore_irq(&jc_list_lock, irqflags);
}

static inline struct usb_xfer_jobchain *usb_xfer_jobchain_dequeue_locked(void)
{
  struct usb_xfer_jobchain *jc = NULL;
  struct list_head *h = &per_channel_jobchains.jc_list;
  if (!list_empty(h)) {
    jc = list_first_entry(h, typeof(*jc), list);
    list_del_init(&jc->list);
    USBQ_INFO("usb_xfer_queue_run:head->prev:%p, head->next:%p", h->prev, h->next);
  }
  return jc;
}

struct usb_xfer_jobchain *usb_xfer_jobchain_dequeue(void)
{
  struct usb_xfer_jobchain *jc;
  int irqflags;
  cond_spinlock_lock_disable_irq(&jc_list_lock, irqflags);
  jc = usb_xfer_jobchain_dequeue_locked();
  cond_spinlock_unlock_restore_irq(&jc_list_lock, irqflags);
  return jc;
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
  struct usb_xfer_job *j;
  struct dwc2_channel *c = NULL;
  DECL_PIPE_DESC(dwc2_pipe, jc->hcd_pipe);

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
  list_for_each_entry(j, &jc->jobs, jobs) {
    printf("usb_xfer_one_jobchain: processing next job: %p\n", j);
    usb_xfer_one_job(j, c);
    if (j->err != ERR_OK) {
      jc->err = j->err;
      printf("usb_xfer_one_jobchain: job %p completed with err %d\n", j, j->err);
      break;
    }
    printf("usb_xfer_one_jobchain: job %p completed with success, next_job is %p\n", j, j->jobs.next);
  }
  dwc2_xfer_control_destroy(c->ctl);
  dwc2_channel_free(c);
  j->completion(j->completion_arg);
}

static int usb_xfer_queue_run(void)
{
  struct usb_xfer_jobchain *jc;
  USBQ_INFO("starting usb_runqueue");
  while(1) {
    asm volatile ("wfe");
    jc = usb_xfer_jobchain_dequeue();
    if (jc) {
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
