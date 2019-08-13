#define DAIF_IRQ_BIT    (1 << 1)

.section ".text"

.globl enable_irq
.globl disable_irq
.globl is_irq_enabled

enable_irq:
  msr daifclr, #DAIF_IRQ_BIT
  ret

disable_irq:
  msr daifset, #DAIF_IRQ_BIT
  ret

is_irq_enabled:
  mrs x0, daif
  lsr x0, x0, #1
  mvn x0, x0
  and x0, x0, #1 
  ret
