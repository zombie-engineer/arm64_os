#include <irq.h>
#include <error.h>
#include <common.h>
#include <stringlib.h>

static irq_desc_t irq_descriptors[NUM_IRQS];
static int irq_log_level = 0;
static irq_func irq_post_hook ALIGNED(8) = NULL;

void __handle_irq(int irqnr)
{
  if (irq_log_level > 1)
    printf("__handle_irq: %d" __endline, irqnr);

  if (irqnr >= NUM_IRQS)
    return;

  irq_desc_t *irqd = &irq_descriptors[irqnr];
  if (irqd->handler)
    irqd->handler();

  if (irq_post_hook)
    irq_post_hook();
}

void __irq_set_post_hook(irq_func hook)
{
  irq_post_hook = hook;
}

void irq_init(int log_level)
{
  memset(irq_descriptors, 0, sizeof(irq_descriptors));
  irq_log_level = log_level;
  if (log_level > 1)
    puts("irq_init complete\n");
}

int irq_set(int irqnr, irq_func func)
{
  if (irqnr >= NUM_IRQS) {
    if (irq_log_level > 0)
      printf("irq_set: irq: %d, func: %p, irq number too high.\n", irqnr, func);
    return ERR_INVAL_ARG;
  }

  irq_desc_t *irqd = &irq_descriptors[irqnr];
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
