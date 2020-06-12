#include <arch/armv8/armv8_generic_timer.h>
#include <timer.h>
#include <error.h>
#include <types.h>
#include <common.h>
#include <percpu.h>
#include <irq.h>

struct armv8_generic_timer_data {
  timer_callback_t cb;
  void *cb_arg;
  bool is_oneshot;
};

static struct armv8_generic_timer_data armv8_generic_timer_data = {
  .cb = NULL,
  .cb_arg = NULL,
  .is_oneshot = false
};

/*
 * Taken from pdf "Arm Architecture Reference Manual Armv8"
 * See chapter D13.8.16
 */
#define timer_data (&armv8_generic_timer_data)

#define armv8_generic_timer_interrupt_disable() \
  asm volatile (\
      "mrs x0, cntp_ctl_el0\n"\
      "orr x0, x0, #2\n"\
      "msr cntp_ctl_el0, x0\n":::"x0")

#define armv8_generic_timer_interrupt_enabled() \
{(bool res;\
  asm volatile (\
      "mrs %0, cntp_ctl_el0\n"\
      "lsr %0, #1\n" : "=r"*(res));\
 res;)}

#define armv8_generic_timer_interrupt_enable() \
  asm volatile (\
      "mrs x0, cntp_ctl_el0\n"\
      "bic x0, x0, #2\n"\
      "msr cntp_ctl_el0, x0\n":::("x0"))

#define armv8_generic_timer_enable() \
  asm volatile (\
      "mrs x0, cntp_ctl_el0\n"\
      "orr x0, x0, #1\n"\
      "msr cntp_ctl_el0, x0\n":::("x0"))

#define armv8_generic_timer_disable() \
  asm volatile (\
      "mrs x0, cntp_ctl_el0\n"\
      "bic x0, x0, #1\n"\
      "msr cntp_ctl_el0, x0\n":::("x0"))

static __percpu_func __irq_routine void irq_handler_arm_generic_timer(void)
{
  printf("irq_handler_arm_timer"__endline);
  if (timer_data->is_oneshot) {
    armv8_generic_timer_interrupt_disable();
  }
  if (timer_data->cb)
    timer_data->cb(timer_data->cb_arg);
}

static int armv8_generic_timer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  timer_data->cb = cb;
  timer_data->cb_arg = cb_arg;
  timer_data->is_oneshot = true;

  asm volatile ("msr cntp_tval_el0, %0\n" : : "r"(0x400000));
  asm volatile ("msr cntp_ctl_el0, %0\n" : : "r"(1));
  *(uint32_t *)0x40000044 = 0x0f;
  return ERR_OK;
}

static int armv8_generic_timer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  return ERR_OK;
}

static void armv8_genertic_timer_print()
{
  uint64_t cntp_ctl_el0;
  uint64_t cntp_cval_el0;
  uint64_t cntp_tval_el0;
  uint64_t cntpct_el0;
  asm volatile (
    "mrs %0, cntp_ctl_el0\n"
    "mrs %1, cntp_cval_el0\n"
    "mrs %2, cntp_tval_el0\n"
    "mrs %3, cntpct_el0\n"
    :
    "=r"(cntp_ctl_el0),
    "=r"(cntp_cval_el0),
    "=r"(cntp_tval_el0),
    "=r"(cntpct_el0));
    printf("cpu_test: cntp_ctl_el: %llx,cval:%llx,tval:%llx,ct:%llx" __endline, 
      cntp_ctl_el0, cntp_cval_el0, cntp_tval_el0, cntpct_el0);
    printf("src: %08x" __endline, *(uint32_t*)0x40000064);
}

static struct timer armv8_generic_timer = {
  .set_oneshot = armv8_generic_timer_set_oneshot,
  .set_periodic = armv8_generic_timer_set_periodic,
  .flags = 0,
  .name = "arm_generic_timer"
};

int armv8_generic_timer_init()
{
  int ret;
  ret = timer_register(&armv8_generic_timer, TIMER_ID_ARM_GENERIC_TIMER);
  if (ret != ERR_OK)
    return ret;
  return ERR_OK;
}
