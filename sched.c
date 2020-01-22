#include <config.h>
#include <common.h>
#include <sched.h>
#include <uart/uart.h>
#include <cpu.h>
#include <stringlib.h>
#include <timer.h>
#include <intr_ctl.h>
#include <uart/pl011_uart.h>
#include <cmdrunner.h>

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

int scheduler_test_job(void)
{
  while(1) {
  }
  uart_puts("123456789");
}

task_t *task_create(task_fn fn, const char *task_name)
{
  task_t *t;

  uint64_t *stack; 
  cpuctx_init_opts_t opt;

  t = 0;
  stack = 0;

  t = alloc_task();

  if (!t)
    goto out;

  if (strlen(task_name) > sizeof(t->name))
    goto out;

  strcpy(t->name, task_name);

  stack = alloc_stack(4096);
  if (!stack)
    goto out;

  opt.cpuctx = t->cpuctx;
  opt.cpuctx_sz = sizeof(t->cpuctx);
  opt.sp = (uint64_t)stack;
  opt.pc = (uint64_t)fn;
  // opt.args.argc = argc;
  // opt.args.argv = argv;

  cpuctx_init(&opt);

  t->fn = fn;
  t->stack_base = (uint64_t)stack;

  return t;
out:
  if (stack)
    dealloc_stack(stack);
  if (t)
    dealloc_task(t);
  return 0;
}


task_t *scheduler_pick_next_task(task_t *ct)
{
  return ct->run_queue.next;
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
  task_t *initial_task, *next_task, *cmdrunner_task;

  puts("Starting task scheduler\n");
  task_idx = 0;
  stack_idx = 0;
  memset(&tasks, 0, sizeof(tasks));

  initial_task = task_create(scheduler_test_job, "scheduler_test_job");
  next_task = task_create(pl011_io_thread, "pl011_io_thread");
  cmdrunner_task = task_create(cmdrunner_process, "cmdrunner_process");

  initial_task->run_queue.next = &next_task->run_queue;
  next_task->run_queue.next = &cmdrunner_task->run_queue;
  cmdrunner_task->run_queue.next = &initial_task->run_queue;
  __current_cpuctx = initial_task->cpuctx;
  scheduler_job(0);

  intr_ctl_gpu_irq_enable(INTR_CTL_IRQ_GPU_SYSTIMER_1);

  asm volatile (
      "mov x0, #1\n"
      "msr SPSel, x0\n"
      "mov x0, #(1<<2)\n"
      "msr spsr_el1, x0\n"
      "b __armv8_cpuctx_eret\n"
      );
}
