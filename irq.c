#include <irq.h>
#include <error.h>
#include <common.h>
#include <stringlib.h>
#include <config.h>

static irq_desc_t percpu_irq_tables[NUM_CORES][NUM_IRQS];

static int irq_log_level = 0;

static irq_func irq_post_hook ALIGNED(8) = NULL;

static inline void __handle_irq(int irqnr, irq_desc_t *irq_table)
{
  if (irq_log_level > 1)
    printf("__handle_irq: %d" __endline, irqnr);

  if (irqnr >= NUM_IRQS)
    return;

  irq_desc_t *irqd = &irq_table[irqnr];
  if (irqd->handler)
    irqd->handler();

  if (irq_post_hook)
    irq_post_hook();
}

void __handle_irq_0(int irqnr)
{
  __handle_irq(irqnr, percpu_irq_tables[0]);
}

void __handle_irq_1(int irqnr)
{
  printf("-------%d-"__endline, irqnr);
  __handle_irq(irqnr, percpu_irq_tables[1]);
}

void __handle_irq_2(int irqnr)
{
  __handle_irq(irqnr, percpu_irq_tables[2]);
}

void __handle_irq_3(int irqnr)
{
  __handle_irq(irqnr, percpu_irq_tables[3]);
}

void __irq_set_post_hook(irq_func hook)
{
  irq_post_hook = hook;
}

void irq_init(int log_level)
{
  memset(percpu_irq_tables, 0, sizeof(percpu_irq_tables));
  irq_log_level = log_level;
  if (log_level > 1)
    puts("irq_init complete\n");
}

int irq_set(int cpu, int irqnr, irq_func func)
{
  if (irqnr >= NUM_IRQS) {
    if (irq_log_level > 0)
      printf("irq_set: irq: %d, func: %p, irq number too high.\n", irqnr, func);
    return ERR_INVAL_ARG;
  }

  irq_desc_t *irqd = &percpu_irq_tables[cpu][irqnr];
  if (irqd->handler) {
    if (irq_log_level > 0)
      printf("irq_set: irq: %d, func: %p, irq handler already set.\n", irqnr, func);
    return ERR_BUSY;
  }

  irqd->handler = func;

  if (irq_log_level > 0)
    printf("irq_set: irq: %d, func: %p completed.\n", irqnr, func);
  return ERR_OK;
}
