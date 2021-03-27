#include <config.h>
#include <common.h>
#include <sched.h>
#include <uart/uart.h>
#include <cpu.h>
#include <stringlib.h>
#include <timer.h>
#include <intr_ctl.h>
#include <debug.h>
#include <board/bcm2835/bcm2835_irq.h>
#include <board/bcm2835/bcm2835_arm_timer.h>
#include <uart/pl011_uart.h>
#include <irq.h>
#include <delays.h>
#include <syscall.h>
#include <percpu.h>
#include <mem_access.h>

int sched_log_level = 0;

static uint64_t pid_counter = 0;
static uint64_t idle_pid = 0;
static bool preemption_by_timer = false;
static struct task *idle_task = NULL;

#define PREEMPT_TICKS 10

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

struct pcpu_scheduler_holder pcpu_schedulers[NUM_CORES] ALIGNED(64);

#define STACK_SIZE (1024 * 1024 * 1)
#define NUM_STACKS 10

static void pcpu_scheduler_init(struct scheduler *s)
{
  BUG(!is_aligned(s, 64), "scheduler struct not aligned to cache line width");
  INIT_LIST_HEAD(&s->running);
  INIT_LIST_HEAD(&s->timer_waiting);
  INIT_LIST_HEAD(&s->flag_waiting);
  spinlock_init(&s->lock);
  s->waitflag_is_set = 0;
  s->timer_interval_ms = 1000;
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
  t->task_state = TASK_STATE_TIMEWAITING;
}

void sched_queue_flagwait_task_noirq(struct scheduler *s, struct task *t)
{
  SCHED_DEBUG2("enqueue task %p: wait for flag %p", t, t->waitflag);
  list_del_init(&t->schedlist);
  list_add_tail(&t->schedlist, &s->flag_waiting);
  t->task_state = TASK_STATE_FLAGWAITING;
}

void sched_queue_runnable_task(struct scheduler *s, struct task *t)
{
  int flags;
  disable_irq_save_flags(flags);
  sched_queue_runnable_task_noirq(s, t);
  restore_irq_flags(flags);
}

void sched_queue_timewait_task(struct scheduler *s, struct task *t)
{
  int flags;
  disable_irq_save_flags(flags);
  sched_queue_timewait_task_noirq(s, t);
  restore_irq_flags(flags);
}

void sched_queue_flagwait_task(struct scheduler *s, struct task *t)
{
  int flags;
  disable_irq_save_flags(flags);
  sched_queue_flagwait_task_noirq(s, t);
  restore_irq_flags(flags);
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

void run_on_cpu(int cpu_n, void (*fn)(void))
{
  void **write_to = &__percpu_data[cpu_n].jmp_addr;
  SCHED_DEBUG("setting %p to %p", write_to, fn);
  *write_to = (void *)fn;
  dcache_flush(write_to, 64);
  asm volatile("sev");
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
  t->pid = pid_counter++;
  t->ticks_left = PREEMPT_TICKS;

  if (!t) {
    SCHED_ERR("failed to allocate task");
    goto out_err;
  }

  if (strlen(task_name) > sizeof(t->name)) {
    SCHED_ERR("task name too long");
    goto out_err;
  }

  strcpy(t->name, task_name);

  stack = alloc_stack();
  SCHED_DEBUG("task_create: name: %s, stack: %p", task_name, stack);
  if (!stack) {
    SCHED_ERR("failed to allocate stack for new task");
    goto out_err;
  }

  INIT_LIST_HEAD(&t->schedlist);

  flags = 0;
  prep_task_ctx(stack, (uint64_t)fn, flags, t->cpuctx);

  t->stack_base = (uint64_t)stack;
  t->task_state = TASK_STATE_STOPPED;
  return t;
out_err:
  if (stack)
    dealloc_stack(stack);
  if (t)
    dealloc_task(t);
  return ERR_PTR(ERR_GENERIC);
}

void wait_on_timer_ms(uint64_t msec)
{
  uint64_t now = read_cpu_counter_64();
  uint64_t cnt_per_sec = get_cpu_counter_64_freq();
  uint64_t cnt_per_msec = cnt_per_sec / 1000;
  uint64_t until = now + cnt_per_msec * msec;
  get_current()->timer_wait_until = until;
  SCHED_DEBUG("wait_on_timer_ms: %llu, frq: %llu, until: %llu", now, cnt_per_sec, until);
  sched_queue_timewait_task(get_scheduler(), get_current());
  yield();
}

void OPTIMIZED wait_on_waitflag(atomic_t *waitflag)
{
  struct task *t;

  if (atomic_cmp_and_swap(waitflag, 1, 0) == 1) {
    *waitflag = 0;
    return;
  }

  t = get_current();
  t->waitflag = waitflag;
  SCHED_DEBUG2("wait_on_waitflag t:%p, flag %p", t, waitflag);
  sched_queue_flagwait_task(get_scheduler(), t);
  yield();
}

void wakeup_waitflag(uint64_t *waitflag)
{
  int irqflags;
  // putc('+');
  SCHED_DEBUG2("wakeup_waitflag flag %p", waitflag);
  disable_irq_save_flags(irqflags);
  *waitflag = 1;
  atomic_inc(&get_scheduler()->waitflag_is_set);

  restore_irq_flags(irqflags);
  // putc('+');
}

task_t *scheduler_pick_next_task(struct scheduler *s, task_t *prev_task)
{
  struct task *next_task = NULL;

  /*
   * Return new next
   */
  if (!list_empty(&s->running)) {
    next_task = list_first_entry(&s->running, task_t, schedlist);
    next_task->ticks_left = PREEMPT_TICKS;
  }
  return next_task;
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

#define set_current(__new_current)\
  __percpu_data[get_cpu_num()].context_addr = __new_current->cpuctx;

static inline void schedule_handle_timer_waiting(struct scheduler *s)
{
  task_t *t, *tmp;
  uint64_t now = read_cpu_counter_64();
  list_for_each_entry_safe(t, tmp, &s->timer_waiting, schedlist) {
    if (t->timer_wait_until <= now) {
      SCHED_DEBUG3("timeout: now: %llu, until: %llu", now, t->timer_wait_until);
      sched_queue_runnable_task_noirq(s, t);
    }
  }
}

static inline void schedule_handle_flag_waiting(struct scheduler *s)
{
  task_t *t, *tmp;
  // putc(':');
  /*
   * When timer IRQ fires, we call this function to check if there are tasks
   * that wait for their waitflags. s->waitflag_is_set is fast way to say that there are
   * tasks in flag_waiting queue, who's waitflags have been already set, so we can
   * find this flags by iterating the list.
   *
   * We also have to check that the queue itself contains any tasks.
   * It can be that the waitflag has been set BEFORE the task called 'wait_on_waitflag',
   * meaning that it is not in the flag_waiting queue yet, although the flag is already
   * set.
   * The event that has set the flags has happened BEFORE the task put itself to a waitqueue.
   * But it would do so soon and when this happens we want s->waitflag_is_set to be non-zero
   * to be able to get to processing of this event.
   */
  BUG(s->waitflag_is_set > 0xffffffffffff0000ull, "waitflag_is_set value below 0");
  if (s->waitflag_is_set) {
    // putc(':');
    list_for_each_entry_safe(t, tmp, &s->flag_waiting, schedlist) {
      BUG(t->waitflag == NULL, "flagwait queue contains task with no waitflag");
      if (*t->waitflag) {
        SCHED_DEBUG2("flag at %p is set for task %p",t->waitflag, t);
        *t->waitflag = 0;
        t->waitflag = NULL;
        sched_queue_runnable_task_noirq(s, t);
        atomic_dec(&s->waitflag_is_set);
      }
    }
  }
}

static inline bool is_idle_task(struct task *t)
{
  return t->pid == idle_pid;
}

void schedule()
{
  struct scheduler *s = get_scheduler();
  task_t *next_task;
  task_t *prev_task;
  schedule_debug();

  prev_task = get_current();

  /*
   * Put currently executing task to end of list
   */
  if (prev_task->task_state == TASK_STATE_TIMEWAITING) {
    // puts("scheduling out of a timewait task\n");
  } else if (prev_task->task_state == TASK_STATE_FLAGWAITING) {
    /*
     * We are here because the currently executing task has called
     * 'wait_on_waitflag', and this task is already in a waitflag waiting queue,
     * so in general case it can not be selected as a next runnable task.
     *
     * In a specific case the waitflag can be already set, knowing that we can
     * optimize the response and put this task to top of stack, but let's leave
     * it simple for now.
     *
     */
  } else {
    prev_task->ticks_total += PREEMPT_TICKS - prev_task->ticks_left;
    sched_queue_runnable_task_noirq(s, prev_task);
  }

  /*
   * handle tasks waiting on timers.
   */
  schedule_handle_timer_waiting(s);

  /*
   * handle tasks waiting on event flags
   */
  schedule_handle_flag_waiting(s);

  next_task = scheduler_pick_next_task(s, prev_task);
  if (!next_task)
    next_task = idle_task;

  next_task->task_state = TASK_STATE_RUNNING;
  SCHED_DEBUG("schedule:'%s'->'%s'", prev_task->name, next_task->name);
  BUG(!next_task, "scheduler logic failed.");
  set_current(next_task);
}

static inline bool needs_resched(struct scheduler *s, task_t *t)
{
  if (s->waitflag_is_set)
    return true;

  if (t->ticks_left)
    return false;
  return true;
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
static void OPTIMIZED schedule_from_irq()
{
  /*
   * print debug info
   */
  struct scheduler *s = get_scheduler();
  // putc('!');
  if (sched_log_level > 3)
    schedule_debug_info(s);

  /*
   * decrease current task's cpu time
   */
  get_current()->ticks_left--;

  /*
   * decide do we need to preempt current task
   */
  if (needs_resched(s, get_current()))
    schedule();
  SCHED_DEBUG2("returning to process %s", get_current()->name);
}

void sched_timer_cb(void *arg)
{
  SCHED_DEBUG2("sched_timer_cb");
  asm volatile("sev");
  if (preemption_by_timer)
    SCHED_REARM_TIMER(get_scheduler());
  schedule_from_irq();
}

int idle_func(void)
{
  while(1) {
    asm volatile("wfe");
    yield();
  }

  return 0;
}

void scheduler_init(int log_level, task_fn init_func)
{
  int i;
  struct scheduler *s;
  task_t *init_task;

  for (i = 0; i < ARRAY_SIZE(pcpu_schedulers); ++i)
    pcpu_scheduler_init(get_scheduler_n(i));

  s = get_scheduler();
  puts("Starting task scheduler" __endline);
  task_idx = 0;
  stack_idx = 0;
  memset(&tasks, 0, sizeof(tasks));

  idle_task = task_create(idle_func, "idle");
  sched_queue_runnable_task(s, idle_task);

  init_task = task_create(init_func, "init");
  sched_queue_runnable_task(s, init_task);

  s->sched_timer = timer_get(TIMER_ID_ARM_GENERIC_TIMER);
  if (s->sched_timer->interrupt_enable) {
    BUG(s->sched_timer->interrupt_enable() != ERR_OK,
      "Failed to init scheduler timer");
  }
  sched_log_level = log_level;
  intr_ctl_arm_generic_timer_irq_enable(get_cpu_num());
  irq_mask_all();
  enable_irq();
  // SCHED_REARM_TIMER(get_scheduler());
  start_task_from_ctx(init_task->cpuctx);
}
