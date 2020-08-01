#pragma once
#include <list.h>
#include <types.h>
#include <syscall.h>
#include <config.h>
#include <common.h>
#include <atomic.h>

extern int sched_log_level;

#define SCHED_INFO(__fmt, ...) printf("[SCHED INFO] " __fmt __endline, ## __VA_ARGS__)
#define __SCHED_DEBUG_N(__n, __fmt, ...) \
  do {\
    if (sched_log_level > __n) \
      printf("[SCHED DEBUG] " __fmt __endline, ## __VA_ARGS__);\
  } while(0)

#define SCHED_DEBUG(__fmt, ...) __SCHED_DEBUG_N(0, __fmt, ## __VA_ARGS__)
#define SCHED_DEBUG2(__fmt, ...) __SCHED_DEBUG_N(1, __fmt, ## __VA_ARGS__)
#define SCHED_DEBUG3(__fmt, ...) __SCHED_DEBUG_N(2, __fmt, ## __VA_ARGS__)
#define SCHED_ERR(__fmt, ...) printf("[SCHED ERR] " __fmt __endline, ## __VA_ARGS__)

typedef int (*task_fn)(void);

typedef struct task {
  /*
   * the task is attached to scheduler lists by
   * this node
   */
  struct list_head schedlist;

  /*
   * task's name
   */
  char name[64];

  /*
   * generic context holding task's registes and
   * other state when not running.
   */
  char cpuctx[8 * 40];

  uint64_t stack_base;

  /*
   * ticks until preempt. Value is initiated with
   * some value and at timer ticks it's decremented
   * At value == 0, the task is rescheduled.
   */
  int ticks_left;

  /*
   * total number of ticks this task lived in
   * running state so far.
   */
  int ticks_total;

#define TASK_STATE_STOPPED     0
#define TASK_STATE_SCHEDULED   1
#define TASK_STATE_RUNNING     2
#define TASK_STATE_TIMEWAITING 3
#define TASK_STATE_FLAGWAITING 4

  int task_state;

  uint64_t timer_wait_until;
  atomic_t *waitflag;
} task_t;

extern void *get_current_ctx();

#define get_current() (container_of(get_current_ctx(), task_t, cpuctx))

struct scheduler {
  atomic_t flag_is_set;

  struct spinlock lock;
  struct list_head running;
  struct list_head timer_waiting;
  struct list_head flag_waiting;
  struct timer *sched_timer;
  int timer_interval_ms;
};

void scheduler_init(int log_level, task_fn init_func);

void schedule();

void sched_queue_runnable_task(struct scheduler *s, struct task *t);

void sched_queue_timewait_task(struct scheduler *s, struct task *t);

void sched_queue_flagwait_task(struct scheduler *s, struct task *t);

void wait_on_timer_ms(uint64_t msec);

void waitflag_init(atomic_t *waitflag);

void wait_on_waitflag(atomic_t *waitflag);

void wakeup_waitflag(atomic_t *waitflag);

void sched_timer_cb(void *arg);

#define SCHED_REARM_TIMER(__scheduler) \
  __scheduler->sched_timer->set_oneshot(__scheduler->timer_interval_ms, sched_timer_cb, 0)

static inline void yield()
{
  // SCHED_INFO("yield enter: %s\n", get_current()->name);
  asm volatile ("svc %0"::"i"(SVC_YIELD));
  // SCHED_INFO("yield exit: %s\n", get_current()->name);
}

task_t *task_create(task_fn fn, const char *task_name);

struct pcpu_scheduler_holder {
  struct scheduler s;
 // char   padding[64 - sizeof(struct scheduler)];
};

extern struct pcpu_scheduler_holder pcpu_schedulers[NUM_CORES];

#define get_scheduler_n(cpu) (&pcpu_schedulers[cpu].s)
#define get_scheduler() (get_scheduler_n(get_cpu_num()))

void start_task_from_ctx(void *cpuctx);

void run_on_cpu(int cpu_n, void (*fn)(void));
