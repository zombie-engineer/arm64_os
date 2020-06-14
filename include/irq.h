#pragma once

#define NUM_IRQS 128

/*
 *  __irq_routine - a hint notice, that indicates the function is bound
 *  to a particular IRQ number.
 */
#define __irq_routine

typedef void(*irq_func)();

typedef struct irq_desc {
  irq_func handler;
} irq_desc_t;

void __handle_irq_0(int irqnr);
void __handle_irq_1(int irqnr);
void __handle_irq_2(int irqnr);
void __handle_irq_3(int irqnr);

void __irq_set_post_hook(irq_func);

void irq_init(int log_level);

int irq_set(int cpu, int irqnr, irq_func func);
int irq_local_set(int cpu, irq_func func);
