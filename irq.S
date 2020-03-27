#include "intr_ctl_mem.h"
.equ IC_BASE, BCM2835_IC_BASE
.equ IC_PEND0_OFF, 0x0
.equ IC_PEND1_OFF, 0x4
.equ IC_PEND2_OFF, 0x8
.equ ARM_IRQ0_BASE, 0
.equ ARM_IRQ1_BASE, 0
.equ ARM_IRQ2_BASE, 32

.macro get_irqrn_preamble, base, tmp
  ldr   \base, =BCM2835_IC_BASE
.endm

.macro get_irqnr_and_base, irqnr, irqstat, base, tmp, tmp2
  // get masked status
  ldr   \irqstat, [\base, #IC_PEND0_OFF] 
  mov   \irqnr, #(ARM_IRQ0_BASE + 31)
  // save bits 8, 9 of basic pending register
  and   \tmp, \irqstat, #((1<<8)|(1<<9))
  // clear bits 8, 9 and test
  bics  \irqstat, \irqstat, \tmp
  bne   1020f
  
  tst   \tmp, #(1<<8)
  beq   1010f
  ldr   \irqstat, [\base, #IC_PEND1_OFF]
  mov   \irqnr, #(ARM_IRQ1_BASE + 31)
  // mask out basic pending irqs
  mov   \tmp2, #((1<<7)|(1<<9)|(1<<10))
  bic   \irqstat, \irqstat, \tmp2
  mov   \tmp2, #((1<<18)|(1<<19))
  bic   \irqstat, \irqstat, \tmp2
  b     1020f

1010:
  tst   \tmp, #(1<<9)
  beq   1030f
  ldr   \irqstat, [\base, #IC_PEND2_OFF]
  mov   \irqnr, #(ARM_IRQ1_BASE + 31)
  // mask out basic pending irqs
  mov   \tmp2, #((1<<21)|(1<<22)|(1<<23)|(1<<24)|(1<<25))
  bic   \irqstat, \irqstat, \tmp2
  mov   \tmp2, #(1<<30)
  bic   \irqstat, \irqstat, \tmp2

1020:
  // LSB(x) = 31 - CLZ(x^(x-1))
  // 
  sub   \tmp, \irqstat, #1 
  eor   \irqstat, \irqstat, \tmp
  clz   \tmp, \irqstat
  sub   \irqnr, \irqnr, \tmp
  

1030:
.endm

.section ".text.interrupt"
.globl __intr_ctl_handle
__intr_ctl_handle:
  get_irqnr_and_base x0, x1, x2, x3, x4
  ret
