#include <interrupts.h>
#include <common.h>
#include <arch/armv8/armv8.h>

#define INT_CTRL_BASE                   (PERIPHERAL_BASE_PHY  + 0xb200)
#define INT_CTRL_IRQ_BASIC_PENDING      (reg32_t)(INT_CTRL_BASE + 0x00)
#define INT_CTRL_IRQ_PENDING_1          (reg32_t)(INT_CTRL_BASE + 0x04)
#define INT_CTRL_IRQ_PENDING_2          (reg32_t)(INT_CTRL_BASE + 0x08)
#define INT_CTRL_IRQ_FIQ_CONTROL        (reg32_t)(INT_CTRL_BASE + 0x0c)
#define INT_CTRL_IRQ_ENABLE_IRQS_1      (reg32_t)(INT_CTRL_BASE + 0x10)
#define INT_CTRL_IRQ_ENABLE_IRQS_2      (reg32_t)(INT_CTRL_BASE + 0x14)
#define INT_CTRL_IRQ_ENABLE_BASIC_IRQS  (reg32_t)(INT_CTRL_BASE + 0x18)
#define INT_CTRL_IRQ_DISABLE_IRQS_1     (reg32_t)(INT_CTRL_BASE + 0x1c)
#define INT_CTRL_IRQ_DISABLE_IRQS_2     (reg32_t)(INT_CTRL_BASE + 0x20)
#define INT_CTRL_IRQ_DISABLE_BASIC_IRQS (reg32_t)(INT_CTRL_BASE + 0x24)

#define BASIC_IRQ_TIMER             (1 << 0)
#define BASIC_IRQ_MAILBOX           (1 << 1)
#define BASIC_IRQ_DOORBELL_0        (1 << 2)
#define BASIC_IRQ_DOORBELL_1        (1 << 3)
#define BASIC_IRQ_GPU_0_HALTED      (1 << 4)
#define BASIC_IRQ_GPU_1_HALTED      (1 << 5)
#define BASIC_IRQ_ACCESS_ERR_TYPE_0 (1 << 6)
#define BASIC_IRQ_ACCESS_ERR_TYPE_1 (1 << 7)

#define IRQ_ENABLE_1_SYSTEM_TIMER_1 (1 << 1)
#define IRQ_ENABLE_1_SYSTEM_TIMER_3 (1 << 3)
#define IRQ_ENABLE_1_USB            (1 << 9)
#define IRQ_AUX_INT                 (1 << 29)

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
  print_reg32(INT_CTRL_IRQ_DISABLE_IRQS_1);
  print_reg32(INT_CTRL_IRQ_DISABLE_IRQS_2);
  print_reg32(INT_CTRL_IRQ_DISABLE_BASIC_IRQS);
  printf("---------\n");
}


void interrupt_ctrl_enable_sys_timer_irq(void)
{
  uint32_t irq_enable_1;
  irq_enable_1 = read_reg(INT_CTRL_IRQ_ENABLE_IRQS_1); 
  irq_enable_1 |= IRQ_ENABLE_1_SYSTEM_TIMER_1;
  irq_enable_1 |= IRQ_ENABLE_1_SYSTEM_TIMER_3;
  write_reg(INT_CTRL_IRQ_ENABLE_IRQS_1, irq_enable_1); 
}

void interrupt_ctrl_enable_timer_irq(void)
{
  uint32_t basic_irq_reg;
  basic_irq_reg = read_reg(INT_CTRL_IRQ_ENABLE_BASIC_IRQS);
  basic_irq_reg |= BASIC_IRQ_TIMER;
  write_reg(INT_CTRL_IRQ_ENABLE_BASIC_IRQS, basic_irq_reg);
}

void interrupt_ctrl_enable_gpio_irq(int gpio_num)
{
  uint32_t irq_enable_reg, irq_enable_basic_reg;

  irq_enable_reg = read_reg(INT_CTRL_IRQ_ENABLE_IRQS_2);
  irq_enable_reg |= 1 << (49 - 32);
  irq_enable_reg |= 1 << (50 - 32);
  irq_enable_reg |= 1 << (51 - 32);
  irq_enable_reg |= 1 << (52 - 32);
  write_reg(INT_CTRL_IRQ_ENABLE_IRQS_2, irq_enable_reg);

  irq_enable_basic_reg = read_reg(INT_CTRL_IRQ_ENABLE_BASIC_IRQS);
  irq_enable_basic_reg |= 1 << gpio_num;
  write_reg(INT_CTRL_IRQ_ENABLE_BASIC_IRQS, irq_enable_basic_reg);
}
