#include <config.h>
#include <common.h>
#include <sched.h>
#include <uart/uart.h>
#include <cpu.h>
#include <stringlib.h>
#include <timer.h>
#include <interrupts.h>

extern void *__current_cpuctx;

extern void __armv8_cpuctx_eret();

#define STACK_SIZE (1024 * 1024 * 1)
#define NUM_STACKS 10

static task_t tasks[40];

static int task_idx;

task_t *alloc_task()
{
  return &tasks[task_idx++];
}

void dealloc_task(task_t *t)
{
}

void dealloc_stack(void *s)
{
}

static char stacks[STACK_SIZE * NUM_STACKS];

static char stack_idx = 0;

void *alloc_stack()
{
  stack_idx++;
  return stacks + stack_idx * STACK_SIZE;
}


int scheduler_test_job(int argc, char *argv[])
{
  while(1)
    uart_puts("123456789");
}

int scheduler_test_job2(int argc, char *argv[])
{
  while(1) {
    uart_puts("abcdefjhi");
  }
}

task_t *task_create(int(*work)(int, char *[]), int argc, char *argv[])
{
  task_t *tsk;
  uint64_t *stack; 
  cpuctx_init_opts_t opt;

  tsk = 0;
  stack = 0;

  tsk = alloc_task();

  if (!tsk) {
    goto out;
  }

  stack = alloc_stack(4096);
  if (!stack) {
    goto out;
  }

  opt.cpuctx = tsk->cpuctx;
  opt.cpuctx_sz = sizeof(tsk->cpuctx);
  opt.sp = (uint64_t)stack;
  opt.pc = (uint64_t)work;
  opt.args.argc = argc;
  opt.args.argv = argv;

  cpuctx_init(&opt);

  tsk->work = work;
  tsk->stack_base = (uint64_t)stack;

  return tsk;
out:
  if (stack)
    dealloc_stack(stack);
  if (tsk)
    dealloc_task(tsk);
  return 0;
}


task_t *scheduler_pick_next_task(task_t *ct)
{
  return ct->sched_list.next;
}

void* scheduler_job(void* arg)
{
  task_t *current_task;
  current_task = container_of(__current_cpuctx, task_t, cpuctx);
  current_task = scheduler_pick_next_task(current_task);

  if (!current_task)
    kernel_panic("scheduler logic failed.\n");

  __current_cpuctx = current_task->cpuctx;
  systimer_set_oneshot(CONFIG_SCHED_INTERVAL_US, scheduler_job, 0);
  return 0;
}

void scheduler_init()
{
  task_t *initial_task, *next_task;

  puts("Starting task scheduler\n");
  task_idx = 0;
  stack_idx = 0;
  memset(&tasks, 0, sizeof(tasks));

  char *argv1[] = {
    "initial_task"
  };

  initial_task = task_create(scheduler_test_job, ARRAY_SIZE(argv1), argv1);

  char *argv2[] = {
    "test_job2"
  };

  next_task = task_create(scheduler_test_job2, ARRAY_SIZE(argv2), argv2);

  initial_task->sched_list.next = next_task;
  next_task->sched_list.next = initial_task;
  __current_cpuctx = initial_task->cpuctx;
  scheduler_job(0);

  interrupt_ctrl_enable_systimer_1();

  asm volatile (
      "mov x0, #1\n"
      "msr SPSel, x0\n"
      "mov x0, #(1<<2)\n"
      "msr spsr_el1, x0\n"
      "b __armv8_cpuctx_eret\n"
      );

  while(1) {
    asm volatile ("wfi");
  }
}
