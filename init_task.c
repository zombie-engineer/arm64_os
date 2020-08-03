#include <init_task.h>
#include <delays.h>
#include <debug.h>
#include <irq.h>
#include <sched.h>
#include <intr_ctl.h>
#include <uart/uart.h>
#include <percpu.h>
#include <common.h>
#include <timer.h>
#include <cmdrunner.h>
#include <uart/pl011_uart.h>
#include <drivers/usb/usbd.h>
#include <mmu.h>

static struct timer *test_timer;

static int run_cmdrunner_thread()
{
  task_t *t;
  t = task_create(cmdrunner_process, "cmdrunner_process");
  if (IS_ERR(t))
    return PTR_ERR(t);
  sched_queue_runnable_task(get_scheduler(), t);
  return ERR_OK;
}

static int run_uart_thread()
{
  task_t *t;
  t = task_create(pl011_io_thread, "pl011_io_thread");
  if (IS_ERR(t))
    return PTR_ERR(t);
  sched_queue_runnable_task(get_scheduler(), t);
  return ERR_OK;
}

int usb_init_func()
{
  usbd_init();
  usbd_print_device_tree();
  usbd_monitor();

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

int run_usb_initialization()
{
  task_t *t;
  t = task_create(usb_init_func, "usb_init");
  if (IS_ERR(t))
    return PTR_ERR(t);
  sched_queue_runnable_task(get_scheduler(), t);
  return ERR_OK;
}

void other_cpu_timer_handler(void *arg)
{
  puts("++"__endline);
  test_timer->set_oneshot(1000 * 1000, other_cpu_timer_handler, NULL);
}

void cpu_test(void)
{
  int err;
  list_timers();
  test_timer = timer_get(TIMER_ID_ARM_GENERIC_TIMER);
  BUG(!test_timer, "Failed to get arm generic timer");
  if (test_timer->interrupt_enable) {
    err = test_timer->interrupt_enable();
    BUG(err != ERR_OK, "Failed to enable timer interrupt");
  }
  intr_ctl_arm_generic_timer_irq_enable(get_cpu_num());

  enable_irq();

  test_timer->set_oneshot(100 * 1000, other_cpu_timer_handler, NULL);
  while(1) {
    wait_msec(500);
  }
}

static int idle(void)
{
  printf("starting idle process on cpu %d\n", get_cpu_num());
  while(1) {
    asm volatile("wfi");
    wait_msec(500);
    // printf("__cpu:%d"__endline, get_cpu_num());
  }
  return 0;
}

static void scheduler_startup_idle(void)
{
  task_t *t;
  struct scheduler *s = get_scheduler();

  s->sched_timer = timer_get(TIMER_ID_ARM_GENERIC_TIMER);
  mmu_enable_configured();
  printf("startup idle on cpu: %d\n", get_cpu_num());
  t = task_create(idle, "idle");
  BUG(IS_ERR(t), "failed to create idle task");
  intr_ctl_arm_generic_timer_irq_enable(get_cpu_num());
  s->timer_interval_ms = 1000;
  sched_queue_runnable_task(s, t);
  // s->sched_timer->set_oneshot(s->timer_interval_ms, sched_timer_cb, 0);
  // SCHED_REARM_TIMER(s);
  enable_irq();
  start_task_from_ctx(t->cpuctx);
}

void scheduler_second_cpu_startup(int cpu_n)
{
  run_on_cpu(cpu_n, scheduler_startup_idle);
}

int init_func(void)
{
  SCHED_DEBUG("starting init function");
  BUG(run_uart_thread()        != ERR_OK, "failed to run uart_thread");
  BUG(run_cmdrunner_thread()   != ERR_OK, "failed to start command runner");
  BUG(run_usb_initialization() != ERR_OK, "failed to start usb init thread");
  scheduler_second_cpu_startup(1);
  scheduler_second_cpu_startup(2);
  scheduler_second_cpu_startup(3);

  while(1) {
    // debug_event_1();
    asm volatile("wfe");
    yield();
  }
}

