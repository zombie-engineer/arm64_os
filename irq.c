#include <irq.h>
#include <error.h>
#include <common.h>
#include <stringlib.h>
#include <config.h>
#include <percpu.h>

static irq_desc_t percpu_irq_tables[NUM_CORES][NUM_IRQS];

DECL_PCPU_DATA(irq_desc_t, local_irq_tables);

static int irq_log_level = 0;

static irq_func irq_post_hook ALIGNED(8) = NULL;

static inline void __handle_irq(int irqnr, irq_desc_t *irq_table)
{
  if (irq_log_level > 1)
    printf("__handle_irq: %d, irq_table: %p\n" __endline, irqnr, irq_table);

  if (irqnr >= NUM_IRQS)
    return;

  irq_desc_t *irqd = &irq_table[irqnr];
  if (irqd->handler)
    irqd->handler();

  if (irq_post_hook)
    irq_post_hook();
}

static inline void __handle_local_periph_irq(int cpu_num)
{
  irq_desc_t *irqd = get_pcpu_data_n(cpu_num, local_irq_tables);
  if (irq_log_level > 1)
    printf("__handle_local_periph_irq: %d, handler: %p" __endline, cpu_num, irqd->handler);
  if (irqd->handler)
    irqd->handler();
}

void __handle_local_periph_irq_0(void)
{
  __handle_local_periph_irq(0);
}

void __handle_local_periph_irq_1(void)
{
  __handle_local_periph_irq(1);
}

void __handle_local_periph_irq_2(void)
{
  __handle_local_periph_irq(2);
}

void __handle_local_periph_irq_3(void)
{
  __handle_local_periph_irq(3);
}

void __handle_irq_0(int irqnr)
{
  __handle_irq(irqnr, percpu_irq_tables[0]);
}

void __handle_irq_1(int irqnr)
{
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
  printf("irq_set: cpu:%d, irqnr: %d, irq_func: %p" __endline, cpu, irqnr, func);
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

int irq_local_set(int cpu, irq_func func)
{
  printf("irq_local_set: cpu:%d, irq_func: %p" __endline, cpu, func);
  irq_desc_t *irqd = get_pcpu_data_n(cpu, local_irq_tables);
  irqd->handler = func;
  return ERR_OK;
}
