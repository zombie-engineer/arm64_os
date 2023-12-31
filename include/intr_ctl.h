#pragma once
#include "reg_access.h"
#include "gpio.h"

/*
 * Optimizations in API is not a good thing, but still
 * macro values are optimized for performance as code
 * that works with interrupt controller has to be fast.
 */
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

/* Below are interrupts that come from broadcom peripheral interrupts controller
 * with irq numbers above 63. They are documented as FIQ-reroutable with numbers 64,65,66,..
 * in the docs. but can also be accepted as IRQs with same number due to logic of the irq
 * handling routine, descibed in the same interrupts chapter in BCM2837-ARM-Peripherals pdf.
 * Same irq handling code is used in linux kernel and here as well.
 */
#define INTR_CTL_IRQ_GPU_DOORBELL_0 (INTR_CTL_IRQ_TYPE_ARM + INTR_CTL_IRQ_ARM_DOORBELL_0)
#define INTR_CTL_IRQ_GPU_DOORBELL_1 (INTR_CTL_IRQ_TYPE_ARM + INTR_CTL_IRQ_ARM_DOORBELL_1)


uint32_t intr_ctl_read_pending_gpu_1();

void intr_ctl_dump_regs(const char* tag);

void intr_ctl_enable_gpio_irq(void);

void intr_ctl_disable_gpio_irq(void);

int intr_ctl_arm_irq_enable(int irq_num);

int intr_ctl_arm_irq_disable(int irq_num);

int intr_ctl_gpu_irq_disable(int irq_num);

int intr_ctl_gpu_irq_enable(int irq_num);

int intr_ctl_arm_interrupt_enable(int irq_num);

int intr_ctl_arm_interrupt_disable(int irq_num);

void intr_ctl_arm_generic_timer_irq_enable(int cpu);

void intr_ctl_arm_generic_timer_irq_disable(int cpu);

void intr_ctl_usb_irq_enable(void);

void intr_ctl_usb_irq_disable(void);
