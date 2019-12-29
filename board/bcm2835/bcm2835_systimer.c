#include <timer.h>
#include <board/bcm2835/bcm2835_systimer.h>
#include <interrupts.h>
#include <exception.h>
#include <stringlib.h>
#include <common.h>

#include <reg_access.h>
#include <error.h>

#define SYSTEM_TIMER_BASE   (uint64_t)(PERIPHERAL_BASE_PHY + 0x3000)
#define SYSTEM_TIMER_CS     (reg32_t)(SYSTEM_TIMER_BASE + 0x00)
#define SYSTEM_TIMER_CLO    (reg32_t)(SYSTEM_TIMER_BASE + 0x04)
#define SYSTEM_TIMER_CHI    (reg32_t)(SYSTEM_TIMER_BASE + 0x08)
#define SYSTEM_TIMER_C0     (reg32_t)(SYSTEM_TIMER_BASE + 0x0c)
#define SYSTEM_TIMER_C1     (reg32_t)(SYSTEM_TIMER_BASE + 0x10)
#define SYSTEM_TIMER_C2     (reg32_t)(SYSTEM_TIMER_BASE + 0x14)
#define SYSTEM_TIMER_C3     (reg32_t)(SYSTEM_TIMER_BASE + 0x18)

typedef struct bcm2835_systimer {
  timer_callback_t cb;
  void *cb_arg;
  uint32_t period;
} bcm2835_systimer_t;

static bcm2835_systimer_t bcm2835_systimer_info_1;

static void bcm2835_systimer_clear_irq_1()
{
  write_reg(SYSTEM_TIMER_CS, (1<<1));
}

static int bcm2835_systimer_set(uint32_t usec)
{
  uint32_t clo;
  clo = read_reg(SYSTEM_TIMER_CLO);
  write_reg(SYSTEM_TIMER_C1, clo + usec);
  write_reg(SYSTEM_TIMER_CS, 1);
  interrupt_ctrl_enable_systimer_1();
  return ERR_OK;
}

static void bcm2835_systimer_info_reset(bcm2835_systimer_t *t) 
{
  memset(t, 0, sizeof(*t));
}

static void bcm2835_systimer_info_set(bcm2835_systimer_t *t, timer_callback_t cb, void *cb_arg, uint32_t period)
{
  t->cb = cb;
  t->cb_arg = cb_arg;
  t->period = period;
}

static void bcm2835_systimer_run_cb_1()
{
  if (bcm2835_systimer_info_1.cb)
    bcm2835_systimer_info_1.cb(bcm2835_systimer_info_1.cb_arg);
}

static int bcm2835_systimer_cb_periodic_timer_1()
{
  // bcm2835_systimer_set(bcm2835_systimer_info_1.period);
  bcm2835_systimer_clear_irq_1();
  bcm2835_systimer_run_cb_1();
  return ERR_OK;
}

static int bcm2835_systimer_cb_oneshot_timer_1()
{
  // puts("bcm2835_systimer_cb_oneshot_timer_1\n");
  bcm2835_systimer_clear_irq_1();
  interrupt_ctrl_disable_systimer_1();
  bcm2835_systimer_run_cb_1();
  return ERR_OK;
}

/*static void bcm2835_systimer_cb_oneshot_timer_3()
{
  interrupt_ctrl_disable_systimer_3();
  if (timer_3_cb)
    timer_3_cb(timer_3_cb_arg);
}*/

int bcm2835_systimer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  bcm2835_systimer_info_set(&bcm2835_systimer_info_1, cb, cb_arg, usec);
  set_irq_cb(bcm2835_systimer_cb_periodic_timer_1);
  return bcm2835_systimer_set(usec);
}

int bcm2835_systimer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  bcm2835_systimer_info_set(&bcm2835_systimer_info_1, cb, cb_arg, 0);
  set_irq_cb(bcm2835_systimer_cb_oneshot_timer_1);
  return bcm2835_systimer_set(usec);
}

int bcm2835_systimer_init()
{
  bcm2835_systimer_info_reset(&bcm2835_systimer_info_1);
  return ERR_OK;
}

