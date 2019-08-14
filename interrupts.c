#include "interrupts.h"
#include "common.h"

void interrupt_ctrl_dump_regs(const char* tag)
{
  printf("ARM_TIMER_REGS: tag: %s\n", tag);
  print_reg32(INT_CTRL_IRQ_BASIC_PENDING);
  print_reg32(INT_CTRL_IRQ_PENDING_1);
  print_reg32(INT_CTRL_IRQ_PENDING_2);
  print_reg32(INT_CTRL_IRQ_FIQ_CONTROL);
  print_reg32(INT_CTRL_IRQ_ENABLE_IRQS_1);
  print_reg32(INT_CTRL_IRQ_ENABLE_IRQS_2);
  print_reg32(INT_CTRL_IRQ_ENABLE_BASIC_IRQS);
  // print_reg32(INT_CTRL_IRQ_DISABLE_IRQS_1);
  // print_reg32(INT_CTRL_IRQ_DISABLE_IRQS_2);
  // print_reg32(INT_CTRL_IRQ_DISABLE_BASIC_IRQS);
  printf("---------\n");
}


void interrupt_ctrl_enable_timer_irq(void)
{
  DECL_INTERRUPT_CONTROLLER(irq);
  INT_CTRL_IRQ_ENABLE_BASIC_IRQS |= 1;

  interrupt_ctrl_dump_regs("before");
}

void interrupt_ctrl_enable_gpio_irq(int gpio_num)
{
  INT_CTRL_IRQ_ENABLE_IRQS_2 |= 1 << (49 - 32);
  INT_CTRL_IRQ_ENABLE_IRQS_2 |= 1 << (50 - 32);
  INT_CTRL_IRQ_ENABLE_IRQS_2 |= 1 << (51 - 32);
  INT_CTRL_IRQ_ENABLE_IRQS_2 |= 1 << (52 - 32);
  INT_CTRL_IRQ_ENABLE_BASIC_IRQS |= 1 << gpio_num;
}
