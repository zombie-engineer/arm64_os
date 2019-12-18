#include <timer.h>
#include <board/bcm2835/bcm2835_system_timer.h>
#include <interrupts.h>
#include <reg_access.h>

#define SYSTEM_TIMER_BASE   (uint64_t)(PERIPHERAL_BASE_PHY + 0x3000)
#define SYSTEM_TIMER_CS     (reg32_t)(SYSTEM_TIMER_BASE + 0x00)
#define SYSTEM_TIMER_CLO    (reg32_t)(SYSTEM_TIMER_BASE + 0x04)
#define SYSTEM_TIMER_CHI    (reg32_t)(SYSTEM_TIMER_BASE + 0x08)
#define SYSTEM_TIMER_C0     (reg32_t)(SYSTEM_TIMER_BASE + 0x0c)
#define SYSTEM_TIMER_C1     (reg32_t)(SYSTEM_TIMER_BASE + 0x10)
#define SYSTEM_TIMER_C2     (reg32_t)(SYSTEM_TIMER_BASE + 0x14)
#define SYSTEM_TIMER_C3     (reg32_t)(SYSTEM_TIMER_BASE + 0x18)

static bcm2835_systimer_callback_timer_1()
{
  interrupt_ctrl_disable_systimer_1();
}

static bcm2835_systimer_callback_timer_3()
{
  interrupt_ctrl_disable_systimer_3();
}

void bcm2835_system_timer_set(uint32_t usec)
{
  uint32_t clo;
  clo = read_reg(SYSTEM_TIMER_CLO);
  write_reg(SYSTEM_TIMER_C1, clo + usec);
  // write_reg(SYSTEM_TIMER_C3, clo + usec);
  write_reg(SYSTEM_TIMER_CS, 1);
  interrupt_ctrl_enable_systimer_1();
}

int bcm2835_system_timer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  bcm2835_system_timer_set(usec);
}

int bcm2835_system_timer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  bcm2835_system_timer_set(usec);
}
