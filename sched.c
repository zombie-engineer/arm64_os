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

static struct scheduler __scheduler;

void scheduler_add_to_runlist(struct scheduler *s, struct task *t)
{
  disable_irq();
  list_del_init(&t->schedlist);
  list_add_tail(&t->schedlist, &s->runlist);
  enable_irq();
}

void scheduler_add_to_waitlist(struct scheduler *s, struct task *t)
{
  disable_irq();
  list_add(&t->schedlist, &s->waitlist);
  enable_irq();
}

void scheduler_rm_from_list(struct scheduler *s, struct task *t)
{
  disable_irq();
  list_del_init(&t->schedlist);
  enable_irq();
}

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

extern void __armv8_restore_ctx_from(void*);

void prep_task_ctx(uint64_t *sp, uint64_t fn, uint64_t flags, void *cpuctx)
{
  __armv8_prep_context(sp, fn, flags, cpuctx);
}

void start_task_from_ctx(void *cpuctx)
{
  __armv8_restore_ctx_from(cpuctx);
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

  INIT_LIST_HEAD(&t->schedlist);

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
  return ERR_PTR(ERR_GENERIC);
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

void wait_on_timer_ms(uint64_t msec)
{
  yield();
}

int test_thread()
{
  while(1) {
    blink_led_2(2, 1000);
    wait_on_timer_ms(1000);
  }
}

int run_test_thread()
{
  task_t *t;
  t = task_create(test_thread, "test_thread");
  if (IS_ERR(t))
    return PTR_ERR(t);
  scheduler_add_to_runlist(&__scheduler, t);
  return ERR_OK;
}

static void sched_timer_cb(void *arg)
{
  systimer_set_oneshot(100000, sched_timer_cb, 0);
  blink_led(1, 10);
}

void yield()
{
  asm volatile ("b __armv8_yield\n");
}

int init_task_fn(void)
{
  // run_uart_thread();
  // run_cmdrunner_thread();
  BUG(run_test_thread() != ERR_OK, "Failed to run test thread");
  intr_ctl_gpu_irq_enable(INTR_CTL_IRQ_GPU_SYSTIMER_1);
  // systimer_set_oneshot(CONFIG_SCHED_INTERVAL_US, sched_timer_cb, 0);
  // enable_irq();

  while(1) {
    blink_led(8, 200);
    yield();
  }
  uart_puts("123456789");
}



task_t *scheduler_pick_next_task(task_t *t)
{
  list_del_init(&t->schedlist);
  list_add_tail(&t->schedlist, &__scheduler.runlist);

  /* 
   * Put currently executing task to end of list
   */
  t = list_first_entry(&__scheduler.runlist, task_t, schedlist);
  // list_add_tail(&t->schedlist, &__scheduler.runlist);

  /*
   * Return new next
   */
 //  t = list_first_entry(&__scheduler.runlist, task_t, schedlist);
  return t;
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

void schedule()
{
  task_t *current_task;

  schedule_debug();

  current_task = container_of(__current_cpuctx, task_t, cpuctx);
  current_task = scheduler_pick_next_task(current_task);

  BUG(!current_task, "scheduler logic failed.");

  __current_cpuctx = current_task->cpuctx;
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
  INIT_LIST_HEAD(&__scheduler.runlist);
  INIT_LIST_HEAD(&__scheduler.waitlist);

  task_t *initial_task;
  puts("Starting task scheduler\n");
  task_idx = 0;
  stack_idx = 0;
  memset(&tasks, 0, sizeof(tasks));

  initial_task = task_create(init_task_fn, "init_task_fn");
  scheduler_add_to_runlist(&__scheduler, initial_task);
  start_task_from_ctx(initial_task->cpuctx);
}
