#pragma once
#include <list.h>
#include <types.h>

typedef int (*task_fn)(void);

typedef struct task {
  list_t run_queue;
  char name[64];
  char cpuctx[8 * 40];
  task_fn fn;
  uint64_t stack_base;
} task_t;

void scheduler_init();
