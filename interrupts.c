#include <interrupts.h>
#include <common.h>
#include <arch/armv8/armv8.h>
#include <bits_api.h>

#define BCM2835_IC_BASE          (PERIPHERAL_BASE_PHY  + 0xb200)
#define BCM2835_IC_PENDING_BASIC (reg32_t)(BCM2835_IC_BASE + 0x00)
#define BCM2835_IC_PENDING_GPU_1 (reg32_t)(BCM2835_IC_BASE + 0x04)
#define BCM2835_IC_PENDING_GPU_2 (reg32_t)(BCM2835_IC_BASE + 0x08)
#define BCM2835_IC_FIQ_CONTROL   (reg32_t)(BCM2835_IC_BASE + 0x0c)
#define BCM2835_IC_ENABLE_GPU_1  (reg32_t)(BCM2835_IC_BASE + 0x10)
#define BCM2835_IC_ENABLE_GPU_2  (reg32_t)(BCM2835_IC_BASE + 0x14)
#define BCM2835_IC_ENABLE_BASIC  (reg32_t)(BCM2835_IC_BASE + 0x18)
#define BCM2835_IC_DISABLE_GPU_1 (reg32_t)(BCM2835_IC_BASE + 0x1c)
#define BCM2835_IC_DISABLE_GPU_2 (reg32_t)(BCM2835_IC_BASE + 0x20)
#define BCM2835_IC_DISABLE_BASIC (reg32_t)(BCM2835_IC_BASE + 0x24)

#define BASIC_IRQ_TIMER             (1 << 0)
#define BASIC_IRQ_MAILBOX           (1 << 1)
#define BASIC_IRQ_DOORBELL_0        (1 << 2)
#define BASIC_IRQ_DOORBELL_1        (1 << 3)
#define BASIC_IRQ_GPU_0_HALTED      (1 << 4)
#define BASIC_IRQ_GPU_1_HALTED      (1 << 5)
#define BASIC_IRQ_ACCESS_ERR_TYPE_0 (1 << 6)
#define BASIC_IRQ_ACCESS_ERR_TYPE_1 (1 << 7)

#define GPU_1_SYSTIMER_0 (1 << 0)
#define GPU_1_SYSTIMER_1 (1 << 1)
#define GPU_1_SYSTIMER_2 (1 << 2)
#define GPU_1_SYSTIMER_3 (1 << 3)
#define GPU_1_USB        (1 << 9)
#define GPU_1_AUX        (1 << 29)

void interrupt_ctrl_dump_regs(const char* tag)
{
  printf("ARM_TIMER_REGS: tag: %s\n", tag);
  print_reg32(BCM2835_IC_PENDING_BASIC);
  print_reg32(BCM2835_IC_PENDING_GPU_1);
  print_reg32(BCM2835_IC_PENDING_GPU_2);
  print_reg32(BCM2835_IC_FIQ_CONTROL);
  print_reg32(BCM2835_IC_ENABLE_GPU_1);
  print_reg32(BCM2835_IC_ENABLE_GPU_2);
  print_reg32(BCM2835_IC_ENABLE_BASIC);
  print_reg32(BCM2835_IC_DISABLE_GPU_1);
  print_reg32(BCM2835_IC_DISABLE_GPU_2);
  print_reg32(BCM2835_IC_DISABLE_BASIC);
  printf("---------\n");
}


void interrupt_ctrl_enable_systimer_1(void)
{
  write_reg(BCM2835_IC_ENABLE_GPU_1, GPU_1_SYSTIMER_0);
  write_reg(BCM2835_IC_ENABLE_GPU_1, GPU_1_SYSTIMER_1);
  write_reg(BCM2835_IC_ENABLE_GPU_1, GPU_1_SYSTIMER_2);
}

void interrupt_ctrl_enable_systimer_3(void)
{
  write_reg(BCM2835_IC_ENABLE_GPU_1, GPU_1_SYSTIMER_3);
}

void interrupt_ctrl_disable_systimer_1(void)
{
  write_reg(BCM2835_IC_DISABLE_GPU_1, GPU_1_SYSTIMER_1);
}

void interrupt_ctrl_disable_systimer_3(void)
{
  write_reg(BCM2835_IC_DISABLE_GPU_1, GPU_1_SYSTIMER_3);
}

void interrupt_ctrl_enable_timer_irq(void)
{
  write_reg(BCM2835_IC_ENABLE_BASIC, BCM2835_IC_ENABLE_BASIC);
}

void interrupt_ctrl_enable_gpio_irq(int gpio_num)
{
  uint32_t irq_enable_reg;

  irq_enable_reg = 0;
  irq_enable_reg |= 1 << (49 - 32);
  irq_enable_reg |= 1 << (50 - 32);
  irq_enable_reg |= 1 << (51 - 32);
  irq_enable_reg |= 1 << (52 - 32);
  write_reg(BCM2835_IC_ENABLE_GPU_2, irq_enable_reg);
  write_reg(BCM2835_IC_ENABLE_BASIC, 1 << gpio_num);
}

