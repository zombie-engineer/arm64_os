#include <system_timer.h>
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

void system_timer_set(uint32_t msec)
{
  uint32_t clo;
  interrupt_ctrl_enable_sys_timer_irq();
  clo = read_reg(SYSTEM_TIMER_CLO);
  write_reg(SYSTEM_TIMER_C1, clo + msec);
  write_reg(SYSTEM_TIMER_C3, clo + msec);
  write_reg(SYSTEM_TIMER_CS, 1);
}
