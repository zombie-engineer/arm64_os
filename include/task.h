#pragma once
#include <types.h>

typedef struct task {
  uint64_t sp;
  uint64_t pc;
} task_t;


void run_task(task_t *t);
