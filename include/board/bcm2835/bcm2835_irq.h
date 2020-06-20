#pragma once

#define ARM_IRQ1_BASE 0
#define ARM_IRQ2_BASE 32
#define ARM_BASIC_BASE 64

/*
 * !!WARNING!!: BASIC IRQS 10..20 will be returned as IRQNR 64+10,64+11, 64+12, etc...
 * for the same sake of optimization. For example USB irq is GPU1 bit 9, so you would
 * want to see it arrived as irqnr==9, but for that we would need to maintain a mapping
 * for converting bits 10 through 20 of basic pending register to real IRQ numbers.
 * Because there is no visible need to do that right now, we instead accept that USB
 * irq number is 64+11 = 75.
 */
#define ARM_BASIC_USB       (ARM_BASIC_BASE + 11)
#define ARM_IRQ1_SYSTIMER_1 (ARM_IRQ1_BASE + 1)
#define ARM_IRQ2_GPIO_1     (ARM_IRQ2_BASE + 17)

#define ARM_IRQ_TIMER       (ARM_BASIC_BASE + 0)
