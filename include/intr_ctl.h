#pragma once
#include "reg_access.h"
#include "gpio.h"

// Optimizations in API is not a good thing, 
// but still macro values are optimized for performance
// as code that works with interrupt controller has to
// be fast.
#define INTR_CTL_IRQ_TYPE_GPU 0
#define INTR_CTL_IRQ_TYPE_ARM 64

#define INTR_CTL_IRQ_GPU_SYSTIMER_0 0
#define INTR_CTL_IRQ_GPU_SYSTIMER_1 1
#define INTR_CTL_IRQ_GPU_SYSTIMER_2 2
#define INTR_CTL_IRQ_GPU_SYSTIMER_3 3
#define INTR_CTL_IRQ_GPU_USB        9
#define INTR_CTL_IRQ_GPU_AUX        29
#define INTR_CTL_IRQ_GPU_UART0      57
#define INTR_CTL_IRQ_GPU_MAX        63

#define INTR_CTL_IRQ_ARM_TIMER             0
#define INTR_CTL_IRQ_ARM_MAILBOX           1
#define INTR_CTL_IRQ_ARM_DOORBELL_0        2
#define INTR_CTL_IRQ_ARM_DOORBELL_1        3
#define INTR_CTL_IRQ_ARM_GPU_0_HALTED      4
#define INTR_CTL_IRQ_ARM_GPU_1_HALTED      5
#define INTR_CTL_IRQ_ARM_ACCESS_ERR_TYPE_0 6
#define INTR_CTL_IRQ_ARM_ACCESS_ERR_TYPE_1 7
#define INTR_CTL_IRQ_ARM_MAX               INTR_CTL_IRQ_ARM_ACCESS_ERR_TYPE_1

typedef void (*intr_ctl_irq_cb)(void);

uint32_t intr_ctl_read_pending_gpu_1();

void intr_ctl_dump_regs(const char* tag);

void intr_ctl_enable_gpio_irq(int gpio_num);

void intr_ctl_disable_gpio_irq(int gpio_num);

int intr_ctl_arm_irq_enable(int irq_num);

int intr_ctl_arm_irq_disable(int irq_num);

int intr_ctl_gpu_irq_disable(int irq_num);

int intr_ctl_gpu_irq_enable(int irq_num);
