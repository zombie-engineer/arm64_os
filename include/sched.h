#pragma once
#include <list.h>
#include <types.h>

typedef int (*task_fn)(void);

typedef struct task {
  struct list_head schedlist;
  char name[64];
  char cpuctx[8 * 40];
  task_fn fn;
  uint64_t stack_base;
} task_t;

struct scheduler {
  struct list_head runlist;
  struct list_head waitlist;
};

void scheduler_init();

void scheduler_add_to_runlist(struct scheduler *s, struct task *t);

void scheduler_add_to_waitlist(struct scheduler *s, struct task *t);

void scheduler_rm_from_list(struct scheduler *s, struct task *t);
