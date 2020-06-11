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
#include <board/bcm2835/bcm2835_arm_timer.h>
#include <irq.h>
#include <delays.h>
#include <syscall.h>
#include <percpu.h>
#include <mem_access.h>

static struct timer *sched_timer = NULL;

/*
 * Description of current scheduling algorithm:
 *
 * Scheduler maintains a list of tasks to run, a runlist.
 * If the task needs to wait for something it's removed from a runlist
 * and put into another list called the waitlist.
 *
 * Scheduler is preemptive and manages time quantum for each task by
 * means of system timer irqs.
 * The task has 2 ways of stopping exectution:
 * 1. The task decides to wait for some event by calling one of waiting
 *    functions, passing cpu to the next task.
 * 2. By preemption. Any IRQ results in checking rescheduling conditions.
 *    If current task has run out of it's cpu time it will be removed from
 *    the head of the runlist and put into tail.
 *
 * Context switching:
 * Each task's state is maintained in architecture-specific cpu context.
 * Running task has all the state in it's registers, which includes
 * all general purpose registers.
 * Note:FPU/NEON registers are not stored yet, but should be.
 *
 * Each task has a buffer big enough to store the whole context.
 * When new task needs to run, previously running task will store all of it's
 * context into the buffer. After that new task reads state from it's buffer
 * into cpu registers. After all context is read, cpu has enough state to
 * start running the task.
 *
 * Low level code like IRQ handler should not know much about the task. It
 * only cares about saving and restoring the context, that's why there is
 * a system-global variable '__current_cpuctx' to which most of the low-level
 * code has access to.
 *
 * When interrupt happens, the handler stores all cpu state to where '__current_cpuctx'
 * points to and at exit from interrupt the handler fully recovers state from
 * where '__current_cpuctx' points to.
 *
 * Thus it is possible to switch one cpu context to another by rewriting the pointer
 * to another location during interrupt execution, resulting in entering to interrupt
 * from one task and exiting interrupt into another task.
 *
 * One other way of switching context is by putting the task to wait for some event.
 * In that case the waiting task does not need CPU resource until the requested event
 * happens. That's why it can give up the rest of it's CPU time to some other task.
 * The waiting task will call 'yield', which will store the this task's state into
 * buffer and request rescheduling. The scheduler chooses next task and loads
 * this task's context into cpu registers. After cpu context is restored, the link
 * register of the task contains the address of the instruction from where it should
 * start execution, so the value of the program counter does not matter.
 * As soon as 'yield' reaches 'ret' instruction program counter will be set to
 * address in the link register and return to task execution will take place as if
 * the recovered task itself has called yield.
 */

static inline void yield()
{
  asm volatile ("svc %0"::"i"(SVC_YIELD));
}

extern void __armv8_cpuctx_eret();

#define STACK_SIZE (1024 * 1024 * 1)
#define NUM_STACKS 10

struct pcpu_scheduler_holder {
  struct scheduler s;
  char   padding[64 - sizeof(struct scheduler)];
};
static struct pcpu_scheduler_holder pcpu_schedulers[NUM_CORES] ALIGNED(64);

#define get_scheduler_n(cpu) (&pcpu_schedulers[cpu].s)
#define get_scheduler() (get_scheduler_n(get_cpu_num()))

static void pcpu_scheduler_init(struct scheduler *s)
{
  BUG(!is_aligned(s, 64), "scheduler struct not aligned to cache line width");
  INIT_LIST_HEAD(&s->running);
  INIT_LIST_HEAD(&s->timer_waiting);
  INIT_LIST_HEAD(&s->io_waiting);
}

void sched_queue_runnable_task_noirq(struct scheduler *s, struct task *t)
{
  list_del_init(&t->schedlist);
  list_add_tail(&t->schedlist, &s->running);
  t->task_state = TASK_STATE_SCHEDULED;
}

void sched_queue_timewait_task_noirq(struct scheduler *s, struct task *t)
{
  list_del_init(&t->schedlist);
  list_add_tail(&t->schedlist, &s->timer_waiting);
  t->task_state = TASK_STATE_TIMER_WAIT;
}

void sched_queue_runnable_task(struct scheduler *s, struct task *t)
{
  disable_irq();
  sched_queue_runnable_task_noirq(s, t);
  enable_irq();
}

void sched_queue_timewait_task(struct scheduler *s, struct task *t)
{
  disable_irq();
  sched_queue_timewait_task_noirq(s, t);
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

char stacks[STACK_SIZE * NUM_STACKS] ALIGNED(1024);

static char stack_idx = 0;

void *alloc_stack()
{
  stack_idx++;
  return stacks + stack_idx * STACK_SIZE;
}

task_t *task_create(task_fn fn, const char *task_name)
{
  uint64_t flags;
  task_t *t = NULL;
  uint64_t *stack = NULL;

  t = alloc_task();

  if (!t)
    goto out;

  if (strlen(task_name) > sizeof(t->name))
    goto out;

  strcpy(t->name, task_name);

  stack = alloc_stack();
  printf("stack at %p"__endline, stack);
  if (!stack)
    goto out;

  INIT_LIST_HEAD(&t->schedlist);

  flags = 0;
  prep_task_ctx(stack, (uint64_t)fn, flags, t->cpuctx);

  t->stack_base = (uint64_t)stack;
  t->task_state = TASK_STATE_STOPPED;
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

struct event_waiter {

};

void wait_on_timer_ms(uint64_t msec)
{
  uint64_t now = read_cpu_counter_64();
  uint64_t cnt_per_sec = get_cpu_counter_64_freq();
  uint64_t cnt_per_msec = cnt_per_sec / 1000;
  uint64_t until = now + cnt_per_msec * msec;
  get_current()->timer_wait_until = until;
  // printf("wait_on_timer_ms: %llu, frq: %llu, until: %llu\n", now, cnt_per_sec, until);
  sched_queue_timewait_task(get_scheduler(), get_current());
  yield();
}

int test_thread()
{
  while(1) {
    blink_led_2(12, 100);
    puts("task_b_loop_start" __endline);
    print_cpu_flags();
    puts("task_b_wait_blocking_start" __endline);
    wait_msec(10000);
    puts("************************" __endline);
    puts("task_b_wait_blocking_end" __endline);
    puts("************************" __endline);
    puts("************************" __endline);
    puts("task_b_wait_async_start" __endline);
    wait_on_timer_ms(5000);
    puts("task_b_wait_async_end" __endline);
    puts("task_b_loop_end" __endline);
  }
}

int run_test_thread()
{
  task_t *t;
  t = task_create(test_thread, "test_thread");
  if (IS_ERR(t))
    return PTR_ERR(t);
  sched_queue_runnable_task(get_scheduler(), t);
  return ERR_OK;
}

task_t *scheduler_pick_next_task(struct scheduler *s, task_t *t)
{
  const int ticks_until_preempt = 10;
  /*
   * Put currently executing task to end of list
   */
  if (t->task_state == TASK_STATE_TIMER_WAIT) {

  } else {
    t->ticks_total += ticks_until_preempt;
    sched_queue_runnable_task_noirq(s, t);
  }

  /*
   * Return new next
   */
  t = list_first_entry(&s->running, task_t, schedlist);
  t->ticks_left = ticks_until_preempt;
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
  struct scheduler *s = get_scheduler();
  task_t *current_task;
  // task_t *old_task;

  schedule_debug();

  current_task = get_current();
  // old_task = current_task;
  current_task = scheduler_pick_next_task(s, current_task);
  current_task->task_state = TASK_STATE_RUNNING;
  // printf("schedule:'%s'->'%s'" __endline, old_task->name, current_task->name);

  BUG(!current_task, "scheduler logic failed.");

  __percpu_data[0].context_addr = current_task->cpuctx;
}

static inline bool needs_resched(task_t *t)
{
  if (t->ticks_left)
    return false;
  return true;
}

static inline void schedule_handle_timer_waiting(struct scheduler *s)
{
  task_t *t, *tmp;
  uint64_t now = read_cpu_counter_64();
  list_for_each_entry_safe(t, tmp, &s->timer_waiting, schedlist) {
    if (t->timer_wait_until <= now) {
      // printf("timeout: now: %llu, until: %llu\n", now, t->timer_wait_until);
      sched_queue_runnable_task_noirq(s, t);
    }
  }
}

static inline void schedule_debug_info(struct scheduler *s)
{
  task_t *t;
  char buf[256];
  int n = 0;
  // struct list_head *l;
  n = snprintf(buf + n, sizeof(buf) - n, "runlist: ");
  list_for_each_entry(t, &s->running, schedlist) {
    n += snprintf(buf + n, sizeof(buf) - n, "%s(%d)->", t->name, t->ticks_left);
  }
  n += snprintf(buf + n, sizeof(buf) - n, ", on_timer: ");
  list_for_each_entry(t, &s->timer_waiting, schedlist) {
    n += snprintf(buf + n, sizeof(buf) - n, "%s(%llu)->", t->name, t->timer_wait_until);
  }

  puts(buf);
  puts(__endline);
  print_cpu_flags();
}

/*
 * schedule_from_irq - is called from irq context with irq off
 */
static void schedule_from_irq()
{
  /*
   * print debug info
   */
  struct scheduler *s = get_scheduler();
  // schedule_debug_info();

  /*
   * handle tasks waiting on timers.
   */
  schedule_handle_timer_waiting(s);

  /*
   * decrease current task's cpu time
   */
  get_current()->ticks_left--;

  /*
   * decide do we need to preempt current task
   */
  if (needs_resched(get_current())) {
    /*
     * preempt current task
     */
    schedule();
  }
}

#define SCHED_REARM_TIMER \
  sched_timer->set_oneshot(3000, sched_timer_cb, 0)
  //systimer_set_oneshot(CONFIG_SCHED_INTERVAL_US * 30, sched_timer_cb, 0)

static void sched_timer_cb(void *arg)
{
  // puts("oneshot end"__endline);
  SCHED_REARM_TIMER;
  // blink_led_3(1, 2);
  schedule_from_irq();
}

void other_cpu_timer_handler()
{
  puts("++"__endline);
  asm volatile ("msr cntp_tval_el0, %0\n" : : "r"(0x400000));

  // blink_led_3(10, 100);
  //&__percpu_data[cpu_num].jmp_addr
}

void cpu_test(void)
{
  uint64_t cntp_ctl_el0;
  uint64_t cntp_cval_el0;
  uint64_t cntp_tval_el0;
  uint64_t cntpct_el0;
  asm volatile ("msr cntp_ctl_el0, %0\n" : : "r"(1));
  asm volatile ("msr cntp_cval_el0, %0\n" : : "r"(0x6666666));
  *(uint32_t *)0x40000044 = 0x0f;
  enable_irq();
  while(1) {
    irq_set(1, ARM_IRQ_TIMER, other_cpu_timer_handler);
    wait_msec(1000);

    asm volatile (
        "mrs %0, cntp_ctl_el0\n"
        "mrs %1, cntp_cval_el0\n"
        "mrs %2, cntp_tval_el0\n"
        "mrs %3, cntpct_el0\n"
        :
        "=r"(cntp_ctl_el0),
        "=r"(cntp_cval_el0),
        "=r"(cntp_tval_el0),
        "=r"(cntpct_el0)
        );
    printf("cpu_test: cntp_ctl_el: %llx,cval:%llx,tval:%llx,ct:%llx" __endline, cntp_ctl_el0, cntp_cval_el0, cntp_tval_el0, cntpct_el0);
    printf("src: %08x" __endline, *(uint32_t*)0x40000064);
  }
}

void cpu_run(int cpu_num, void (*fn)(void))
{
  void **write_to = &__percpu_data[cpu_num].jmp_addr;
  printf("setting %p to %p\n", write_to, fn);
  *write_to = (void *)fn;
  dcache_clean_and_invalidate_rng(write_to, (char *)write_to + 64);
  asm volatile("sev");
}

int init_func(void)
{
  int i = 0;
  // run_uart_thread();
  // run_cmdrunner_thread();
  BUG(run_test_thread() != ERR_OK, "Failed to run test thread");
  intr_ctl_gpu_irq_enable(INTR_CTL_IRQ_GPU_SYSTIMER_1);
  intr_ctl_arm_irq_enable(INTR_CTL_IRQ_ARM_TIMER);
  SCHED_REARM_TIMER;
  enable_irq();
  cpu_run(1, cpu_test);
//  for (i = 1; i < NUM_CORES; ++i) {
//    cpu_run(i, cpu_test);
//  }

  while(1) {
    blink_led(1, 100);
    yield();
  }
}

void scheduler_init()
{
  const int ticks_until_preempt = 10;
  int i;
  task_t *init_task;

  for (i = 0; i < ARRAY_SIZE(pcpu_schedulers); ++i)
    pcpu_scheduler_init(get_scheduler_n(i));

  puts("Starting task scheduler\n");
  task_idx = 0;
  stack_idx = 0;
  memset(&tasks, 0, sizeof(tasks));
  sched_timer = get_timer(TIMER_ID_ARM_TIMER);

  init_task = task_create(init_func, "init_func");
  init_task->ticks_left = ticks_until_preempt;
  sched_queue_runnable_task(get_scheduler(), init_task);
  start_task_from_ctx(init_task->cpuctx);
}
