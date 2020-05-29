#define DAIF_IRQ_BIT    (1 << 1)

.macro IRQ_DISABLE
  msr daifset, #DAIF_IRQ_BIT
.endm

.macro IRQ_ENABLE
  msr daifclr, #DAIF_IRQ_BIT
.endm
