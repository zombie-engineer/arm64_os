#include <timer.h>
#include <board/bcm2835/bcm2835_systimer.h>
#include <board/bcm2835/bcm2835_irq.h>

#include <intr_ctl.h>
#include <stringlib.h>
#include <common.h>

#include <reg_access.h>
#include <irq.h>
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

static bcm2835_systimer_t systimer1;

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
  if (systimer1.cb)
    systimer1.cb(systimer1.cb_arg);
}

static void bcm2835_systimer_cb_periodic_timer_1()
{
  bcm2835_systimer_clear_irq_1();
  bcm2835_systimer_run_cb_1();
}

int bcm2835_systimer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  bcm2835_systimer_info_set(&systimer1, cb, cb_arg, usec);
  intr_ctl_set_cb(INTR_CTL_IRQ_TYPE_GPU, INTR_CTL_IRQ_GPU_SYSTIMER_1, bcm2835_systimer_cb_periodic_timer_1);
  return bcm2835_systimer_set(usec);
}

int bcm2835_systimer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  bcm2835_systimer_info_set(&systimer1, cb, cb_arg, 0);
  return bcm2835_systimer_set(usec);
}

// Count maximum cycles for execution of bcm2835_systimer_set function
uint32_t bcm2835_systimer_get_min_set_time()
{
  uint32_t t1, t2;
  uint32_t max_delta;
  const unsigned num_samples = 8;
  unsigned i;
  if (read_reg(SYSTEM_TIMER_CS))
    kernel_panic("bcm2835_systimer_get_min_set_time should not be called after timer is set\n");

  max_delta = 0;
  // While loop here to calculate time range only when 
  // non overflowed values
  for (i = 0; i < num_samples; ++i) {
    while(1) {
      t1 = read_reg(SYSTEM_TIMER_CLO);
      bcm2835_systimer_set(10);
      t2 = read_reg(SYSTEM_TIMER_CLO);
      write_reg(SYSTEM_TIMER_CS, 0);
      if (t2 > t1) {
        if (t2 - t1 > max_delta)
          max_delta = t2 - t1;
        break;
      }
    }
  }
  return max_delta;
}

void irq_handler_systimer_1(void)
{
  bcm2835_systimer_clear_irq_1();
  if (systimer1.cb)
    systimer1.cb(systimer1.cb_arg);
}

struct timer bcm2835_system_timer = {
  .set_oneshot = bcm2835_systimer_set_oneshot,
  .set_periodic = bcm2835_systimer_set_periodic,
  .flags = 0,
  .name = "systimer"
};

int bcm2835_systimer_init()
{
  int ret;
  uint32_t min_timer_set;
  ret = timer_register(&bcm2835_system_timer, TIMER_ID_SYSTIMER);
  if (ret != ERR_OK)
    return ret;

  min_timer_set = bcm2835_systimer_get_min_set_time();
  printf("bcm2835_systimer_init: min timer_set value: %u\n", min_timer_set);
  bcm2835_systimer_info_reset(&systimer1);
  irq_set(0, ARM_IRQ1_SYSTIMER_1, irq_handler_systimer_1);
  return ERR_OK;
}

