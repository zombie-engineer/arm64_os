#include <drivers/usb/usb_xfer_queue.h>
#include <list.h>
#include <spinlock.h>
#include "dwc2.h"
#include "dwc2_channel.h"
#include "dwc2_xfer_control.h"
#include <sched.h>
#include <bug.h>
#include <uart/pl011_uart.h>

int usb_xfer_log_level = 0;

DECL_STATIC_SLOT(struct usb_xfer_job, usb_xfer_jobs, 64);
DECL_STATIC_SLOT(struct usb_xfer_jobchain, usb_xfer_jobchains, 64);

struct jobchain_queue {
  struct list_head jobchains_pending;
  struct list_head jobs_running;
};

static struct jobchain_queue queue_state;
static DECL_COND_SPINLOCK(jobchains_pending_lock);

struct usb_xfer_job *usb_xfer_job_create(void)
{
  struct usb_xfer_job *j;
  j = usb_xfer_jobs_alloc();
  if (!IS_ERR(j)) {
    INIT_LIST_HEAD(&j->jobs);
  }
  USBQ_DEBUG2("usb_xfer_job_alloc:%p", j);
  return j;
}

void usb_xfer_job_destroy(struct usb_xfer_job* j)
{
  usb_xfer_jobs_release(j);
  USBQ_DEBUG2("usb_xfer_job_free:%p", j);
}

struct usb_xfer_jobchain *usb_xfer_jobchain_create(void)
{
  struct usb_xfer_jobchain *jc;
  jc = usb_xfer_jobchains_alloc();
  if (!IS_ERR(jc)) {
    INIT_LIST_HEAD(&jc->list);
    INIT_LIST_HEAD(&jc->jobs);
    INIT_LIST_HEAD(&jc->completed_jobs);
  }
  return jc;
}

void usb_xfer_jobchain_destroy(struct usb_xfer_jobchain *jc)
{
  struct usb_xfer_job *j, *tmp;
  list_for_each_entry_safe(j, tmp, &jc->jobs, jobs) {
    list_del_init(&j->jobs);
    usb_xfer_job_destroy(j);
  }
  list_for_each_entry_safe(j, tmp, &jc->completed_jobs, jobs) {
    list_del_init(&j->jobs);
    usb_xfer_job_destroy(j);
  }
  usb_xfer_jobchains_release(jc);
}

static uint64_t has_work = 0;

void usb_xfer_jobchain_enqueue(struct usb_xfer_jobchain *jc)
{
  int irqflags;
  cond_spinlock_lock_disable_irq(&jobchains_pending_lock, irqflags);
  list_add_tail(&jc->list, &queue_state.jobchains_pending);
  cond_spinlock_unlock_restore_irq(&jobchains_pending_lock, irqflags);
  wakeup_waitflag(&has_work);
}

static inline struct usb_xfer_jobchain *usb_xfer_jobchain_dequeue_locked(void)
{
  struct usb_xfer_jobchain *jc = NULL;
  struct list_head *h = &queue_state.jobchains_pending;
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
  cond_spinlock_lock_disable_irq(&jobchains_pending_lock, irqflags);
  jc = usb_xfer_jobchain_dequeue_locked();
  cond_spinlock_unlock_restore_irq(&jobchains_pending_lock, irqflags);
  return jc;
}

static void usb_xfer_job_cb(void *arg)
{
  struct usb_xfer_job *j = arg;
  USBQ_DEBUG("usb_xfer_job_cb completed");
  j->completed = true;
  wakeup_waitflag(&has_work);
}

static inline void usb_xfer_job_set_running(struct usb_xfer_job *j)
{
  int err;
  usb_xfer_job_print(DEBUG2, j, "usb_xfer_job_set_running");
  j->completion = usb_xfer_job_cb;
  j->completed = false;
  j->completion_arg = j;
  j->err = ERR_OK;
  err = dwc2_xfer_one_job(j);
  BUG(err != ERR_OK, "Failed to start job");
  // printf("j->jobs:%p,%p\n", j->jobs.prev, j->jobs.next);
  list_move_tail(&j->jobs, &queue_state.jobs_running);
}

static inline void usb_xfer_jobchain_start(struct usb_xfer_jobchain *jc)
{
  struct usb_xfer_job *j;
  struct dwc2_channel *c = NULL;
  DECL_PIPE_DESC(dwc2_pipe, jc->hcd_pipe);

  uxb_xfer_jobchain_print(DEBUG2, jc, "running");

  c = dwc2_channel_create();
  BUG(!c, "Failed to create dwc2_channel");
  c->ctl = dwc2_xfer_control_create();
  BUG(!c->ctl, "Failed to create dwc2_xfer_control");
  c->pipe.u.raw = dwc2_pipe.u.raw;
  jc->channel = c;
  jc->err = ERR_OK;
  BUG(list_empty(&jc->jobs), "Trying to process jobchain with 0 jobs");
  j = list_first_entry(&jc->jobs, typeof(*j), jobs);
  usb_xfer_job_set_running(j);
}


/*
 * Check if there are any pending jobchains.
 * If yes start executing first job in chain.
 */
static void usb_xfer_process_pending(void)
{
  // printf("usb_xfer_process_pending\n");
  struct usb_xfer_jobchain *jc;
  jc = usb_xfer_jobchain_dequeue();
  // printf("usb_xfer_process_pending: jc=%p\n", jc);
  if (jc)
    usb_xfer_jobchain_start(jc);
}

/*
 * check if any of pending jobs has been flagged
 * as completed.
 */
static void usb_xfer_process_running(void)
{
  struct usb_xfer_jobchain *jc;
  struct usb_xfer_job *j, *tmp;
  // printf("usb_xfer_process_running\n");
  list_for_each_entry_safe(j, tmp, &queue_state.jobs_running, jobs) {
    bool jobchain_completed = false;
    // printf("usb_xfer_process_running: j:%p, %d\n", j, j->completed);
    if (j->completed) {
      jc = j->jc;
      struct dwc2_channel *c = jc->channel;
      if (c->ctl->status == DWC2_STATUS_NYET) {
        usb_xfer_job_set_running(j);
        continue;
      }

      list_move_tail(&j->jobs, &jc->completed_jobs);
      if (j->err != ERR_OK) {
        jc->err = j->err;
        jobchain_completed = true;
      }
      if (list_empty(&jc->jobs))
        jobchain_completed = true;
      else {
        /* more jobs left in jobchain */
        j = list_first_entry(&jc->jobs, typeof(*j), jobs);
        usb_xfer_job_set_running(j);
      }
      if (jobchain_completed) {
        struct dwc2_channel *c = jc->channel;
        USBQ_DEBUG2("usb_xfer_process_running: jc completed:%p, err:%d", jc, jc->err);
        jc->channel = NULL;
        jc->completed(jc->completed_arg);
        dwc2_xfer_control_destroy(c->ctl);
        c->ctl = NULL;
        dwc2_channel_destroy(c);
        usb_xfer_jobchain_destroy(jc);
      }
    }
  }
}

static int usb_xfer_queue_run(void)
{
  USBQ_INFO("starting usb_runqueue");
  while(1) {
    wait_on_waitflag(&has_work);
    // pl011_putc_blocking('$');
    usb_xfer_process_pending();
    usb_xfer_process_running();
  }
  return 0;
}

void usb_xfer_queue_init(void)
{
  task_t *usb_runqueue_task;
  STATIC_SLOT_INIT_FREE(usb_xfer_jobs);
  STATIC_SLOT_INIT_FREE(usb_xfer_jobchains);
  INIT_LIST_HEAD(&queue_state.jobchains_pending);
  INIT_LIST_HEAD(&queue_state.jobs_running);
  cond_spinlock_init(&jobchains_pending_lock);

  usb_runqueue_task = task_create(usb_xfer_queue_run, "usb_runqueue");
  BUG(IS_ERR(usb_runqueue_task), "Failed to initiazlie usb_runqueue_task");
  sched_queue_runnable_task(get_scheduler(), usb_runqueue_task);
}
