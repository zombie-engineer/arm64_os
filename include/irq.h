#pragma once

#define NUM_IRQS 128

typedef void(*irq_func)();

typedef struct irq_desc {
  irq_func handler;
} irq_desc_t;

void __handle_irq(int irqnr);

void irq_init();

int irq_set(int irqnr, irq_func func);
