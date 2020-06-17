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

static struct timer *test_timer;

static int run_cmdrunner_thread()
{
  task_t *t;
  t = task_create(cmdrunner_process, "cmdrunner_process");
  if (t);
  return ERR_OK;
}

static int run_uart_thread()
{
  task_t *t;
  t = task_create(pl011_io_thread, "pl011_io_thread");
  if (t);
  return ERR_OK;
}

int usb_init_func()
{
  int flags;
  // disable_irq_save_flags(flags);
  usbd_init();
  usbd_print_device_tree();
  // restore_irq_flags(flags);

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

static void cpu_run(int cpu_num, void (*fn)(void))
{
  void **write_to = &__percpu_data[cpu_num].jmp_addr;
  printf("setting %p to %p\n", write_to, fn);
  *write_to = (void *)fn;
  dcache_flush(write_to, 64);
  asm volatile("sev");
}

int init_func(void)
{
  printf("starting init function"__endline);
  // run_uart_thread();
  // run_cmdrunner_thread();
  BUG(run_usb_initialization() != ERR_OK, "failed to start usb init thread");
  SCHED_REARM_TIMER;
  enable_irq();
  // cpu_run(1, cpu_test);

  while(1) {
    asm volatile("wfe");
    yield();
  }
}

