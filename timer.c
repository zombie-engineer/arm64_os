#include <timer.h>
#include <mbox/mbox_props.h>
#include <common.h>
#include <exception.h>

#define MICROSECONDS_PER_SECOND 1000000


void arm_timer_dump_regs(const char* tag)
{
  printf("ARM_TIMER_REGS: tag: %s\n", tag);
  print_reg32(ARM_TIMER_LOAD_REG);
  print_reg32(ARM_TIMER_VALUE_REG);
  print_reg32(ARM_TIMER_CONTROL_REG);
  print_reg32(ARM_TIMER_IRQ_CLEAR_ACK_REG);
  print_reg32(ARM_TIMER_RAW_IRQ_REG);
  print_reg32(ARM_TIMER_MASKED_IRQ_REG);
  print_reg32(ARM_TIMER_RELOAD_REG);
  print_reg32(ARM_TIMER_PRE_DIVIDED_REG);
  print_reg32(ARM_TIMER_FREE_RUNNING_COUNTER_REG);
  printf("---------\n");
}

void arm_timer_set(unsigned period_in_us, void(*irq_timer_handler)(void))
{
  unsigned clock_rate;
  unsigned clocks_per_us;
  // clock rate - is HZ : number of clocks per sec
  // clocks_per_us : number of clocks per us
  if (mbox_get_clock_rate(4, &clock_rate))
     generate_exception();

  clocks_per_us = clock_rate / MICROSECONDS_PER_SECOND;
  // arm_timer_dump_regs("before");
  ARM_TIMER_LOAD_REG = period_in_us / clocks_per_us;
  ARM_TIMER_LOAD_REG = 0x00e00000;
  // arm_timer_dump_regs("after load");
  ARM_TIMER_CONTROL_REG &= ~(1 << ARM_TMR_CTRL_R_TMR_EN_BP);
  // arm_timer_dump_regs("after disable");
  ARM_TIMER_CONTROL_REG |= (1 << ARM_TMR_CTRL_R_WIDTH_BP);
  ARM_TIMER_CONTROL_REG |= (1 << ARM_TMR_CTRL_R_IRQ_EN_BP);
  ARM_TIMER_CONTROL_REG |= (1 << ARM_TMR_CTRL_R_TMR_EN_BP);
  ARM_TIMER_IRQ_CLEAR_ACK_REG = 0xffffffff;
  // arm_timer_dump_regs("after enable");
}
