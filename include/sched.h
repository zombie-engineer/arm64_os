#pragma once
#include <list.h>
#include <types.h>

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

#define TASK_STATE_STOPPED    0
#define TASK_STATE_SCHEDULED  1
#define TASK_STATE_RUNNING    2
#define TASK_STATE_TIMER_WAIT 3
#define TASK_STATE_IO_WAIT    4

  int task_state;

  uint64_t timer_wait_until;
} task_t;

#define get_current() (container_of(__current_cpuctx, task_t, cpuctx))

struct scheduler {
  struct list_head running;
  struct list_head timer_waiting;
  struct list_head io_waiting;
};

void scheduler_init();

void schedule();

void sched_queue_runnable_task(struct scheduler *s, struct task *t);

void sched_queue_timewait_task(struct scheduler *s, struct task *t);
