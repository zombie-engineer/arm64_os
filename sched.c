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
#include <debug.h>
#include <board/bcm2835/bcm2835_irq.h>
#include <irq.h>

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

extern void __armv8_prep_context(uint64_t *, uint64_t, uint64_t, void *);
extern void __armv8_pickup_context(void*);

void prep_task_ctx(uint64_t *sp, uint64_t fn, uint64_t flags, void *cpuctx)
{
  __armv8_prep_context(sp, fn, flags, cpuctx);
}

void start_task_from_ctx(void *cpuctx)
{
  __armv8_pickup_context(cpuctx);
}

static char stacks[STACK_SIZE * NUM_STACKS];

static char stack_idx = 0;

void *alloc_stack()
{
  stack_idx++;
  return stacks + stack_idx * STACK_SIZE;
}

task_t *task_create(task_fn fn, const char *task_name)
{
  task_t *t;

  uint64_t flags;
  uint64_t *stack; 
  //cpuctx_init_opts_t opt;

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

  INIT_LIST_HEAD(&t->run_queue);

  asm volatile("mrs %0, daif\n" : "=r"(flags) );
  prep_task_ctx(stack, (uint64_t)fn, flags, t->cpuctx);

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

int run_cmdrunner_thread()
{
  task_t *t;
  t = task_create(cmdrunner_process, "cmdrunner_process");
  if (t);
  return ERR_OK;
}

int run_uart_thread()
{
  task_t *t;
  t = task_create(pl011_io_thread, "pl011_io_thread");
  if (t);
  return ERR_OK;
}

static void sched_timer_cb(void *arg)
{
  puts("sched_timer_cb");
  systimer_set_oneshot(200000, sched_timer_cb, 0);
}

void post_irq_schedule()
{
  puts("post_irq_schedule");
  while(1);
  // schedule();
}

int init_task_fn(void)
{
  intr_ctl_gpu_irq_enable(INTR_CTL_IRQ_GPU_SYSTIMER_1);
  // __irq_set_post_hook(post_irq_schedule);
  systimer_set_oneshot(CONFIG_SCHED_INTERVAL_US, sched_timer_cb, 0);
  enable_irq();
  //run_uart_thread();
  //run_cmdrunner_thread();

  while(1) {
    // disable_irq();
    blink_led(3, 1000);
    // enable_irq();
    asm volatile("wfe");
  }
  uart_puts("123456789");
}



task_t *scheduler_pick_next_task(task_t *ct)
{
  return container_of(ct->run_queue.next, task_t, run_queue);
}

static int sched_num_switches = 0;

void schedule_debug()
{
  sched_num_switches++;
  if (sched_num_switches % 1000 == 0) {
    // blink_led(1, 10);
    // pl011_uart_send('+');
  }
}

void* schedule()
{
  task_t *current_task;

  schedule_debug();

  current_task = container_of(__current_cpuctx, task_t, cpuctx);
  current_task = scheduler_pick_next_task(current_task);

  if (!current_task)
    kernel_panic("scheduler logic failed.\n");

  __current_cpuctx = current_task->cpuctx;
  return NULL;
}

void yield()
{
  asm volatile ("b __armv8_yield\n");
}


#define ARM8_CPSR_M_USER       0
#define ARM8_CPSR_M_FIQ        1
#define ARM8_CPSR_M_IRQ        2
#define ARM8_CPSR_M_SUPERVISOR 3
#define ARM8_CPSR_M_ABORT      7
#define ARM8_CPSR_M_UNDEFINED  11
#define ARM8_CPSR_M_SYSTEM     15

#define SPSEL_NORMAL    0
#define SPSEL_EXCEPTION 1

#define __stringify(__mode) #__mode

#define armv8_set_mode(__mode)\
  asm volatile (\
      "mov x0, #"__stringify(__mode)"\n"\
      "msr spsr_el1, x0\n" : : : )

#define armv8_set_spsel(__sp_mode)\
  asm volatile (\
      "mov x0, #"__stringify(__sp_mode)"\n"\
      "msr SPSel, x0\n" : : : )

void jump_to_schedule()
{
  asm volatile ("bl __armv8_cpuctx_store\n");
  armv8_set_spsel(SPSEL_EXCEPTION);
  armv8_set_mode(ARM8_CPSR_M_IRQ);
  asm volatile ( "b __armv8_cpuctx_eret\n");
}

void scheduler_init()
{
  task_t *initial_task;
  puts("Starting task scheduler\n");
  task_idx = 0;
  stack_idx = 0;
  memset(&tasks, 0, sizeof(tasks));

  initial_task = task_create(init_task_fn, "init_task_fn");
  start_task_from_ctx(initial_task->cpuctx);
}
