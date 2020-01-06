#pragma once
#include <list.h>
#include <types.h>

typedef struct task {
  list_t sched_list;
  char cpuctx[8 * 40];
  int (*work)(int, char **);
  uint64_t stack_base;
} task_t;

void scheduler_init();
