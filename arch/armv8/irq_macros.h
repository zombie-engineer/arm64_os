#include <board/bcm2835/bcm2835_irq_ctrl.h>
#include <board/bcm2835/bcm2835_irq.h>


.equ IC_BASE, BCM2835_IC_BASE
.equ BASIC_PENDING, 0x0
.equ IC_PEND1_OFF, 0x4
.equ IC_PEND2_OFF, 0x8

/*
 * Macros get_irqrn_preamble and get_irqnr_and_base are
 * copied from BCM2835 ARM Peripherals datasheet, page 111
 *
 * The concept behind those is as following:
 * IRQ_BASIC_PENDING register, given at address 0x7E00B200 
 * or 0x3F00B200 or BCM2835_INTERRUPT_CONTROLLER_BASE + 0x200
 * This register is optimized for fast querying of which 
 * interrupts are pending. 
 * Here is a problematics slow path scenario of handling IRQ:
 *    - CPU Core recieved an IRQ interrupt, it starts executing
 *      code under IRQ interrupt address. It does not know yet, 
 *      what peripheral or event has triggered the interrupt.
 *    - In total it has to read 3 registers to get all penging
 *      bits: IRQ_BASIC_PENDING, IRQ_GPU0_PENDING, IRQ_GPU1_PENDING, 
 *      where IRQ_BASIC_PENDING  has 8 bits, both GPU registers have 
 *      32 bits each, summing up to 32+32+8 = 72 bits to check,
 *      3 device memory accesses and a for loop with 72 bits 
 *      iterations.
 *
 * Optimization is in utilizing free bits of IRQ_BASIC_PENDING
 * register, remember that only 8bits are used to describe which
 * IRQ is pending. Bits 8 and 9 show if there is something of interest
 * in IRQ_GPU0_PENDING and IRQ_GPU1_PENDING registers.
 * Even more, bits 10 ... to... 20 are mapped to the most frequent
 * IRQs from GPU0/GPU1 pending registers. Check with the datasheet
 * for an exact mapping. NOTE: if bits 10...20 are set in BASIC 
 * pending register, that means these IRQs won't set bits 8 or 9
 * for corresponding GPU pending reg. This is also an optimization,
 * so that we didn't have to check the whole 32bit GPU0 bit-space
 * if only IRQ 7 is set and signalled to us via bit 10 of BASIC_PENDING
 * 
 * 'get_irqrn_preamble' is initialization macro-API that inits base register
 * address.
 * 'get_irqnr_and_base' actually sets the irqnr register with first met pending
 * IRQ bit and sets Z (zero flag) in CPU status register when no pending irqs
 * are present. So this macro should be used in a while loop with beq leading to
 * loop-break when all interrupts are processed and cleared.
 */

.macro get_irqrn_preamble, base
  mov   \base, (BCM2835_IC_BASE & 0xffff0000) 
  movk  \base, (BCM2835_IC_BASE & 0xffff) 
.endm

.macro get_irqnr_and_base irqnr, irqstat, base, tmp, tmp2
  /* get masked status */
  ldr   \irqstat, [\base, #BASIC_PENDING] 

  mov   \irqnr, #(ARM_BASIC_BASE + 31)
  /* save bits 8, 9 of basic pending register */
  and   \tmp, \irqstat, #((1<<8)|(1<<9))
  /* 
   * clear bits 8, 9 and test. If irqstat has anything
   * aside of bits 8 or 9, then we are ready to set
   * irqnr and exit
   */
  bics  \irqstat, \irqstat, \tmp
  bne   get_irqnr_and_base_set_irqnr_\@

get_irqnr_and_base_test_bit_8_\@: 
  tbz   \tmp, #8, get_irqnr_and_base_test_bit_9_\@
  ldr   \irqstat, [\base, #IC_PEND1_OFF]
  mov   \irqnr, #(ARM_IRQ1_BASE + 31)

  /*
   * Bits 7,9,10,18,19 in IRQ_GPU1_PENDING register
   * are bits 10,11,12,13,14 in IRQ_BASIC_PENDING,
   * we already checked them above, that's why we mask 
   * them out here.
   */ 
  mov   \tmp2, #((1<<7)|(1<<9)|(1<<10))
  orr   \tmp2, \tmp2, #((1<<18)|(1<<19))
  bic   \irqstat, \irqstat, \tmp2
  b     get_irqnr_and_base_set_irqnr_\@

get_irqnr_and_base_test_bit_9_\@:
  tbz   \tmp, #9, get_irqnr_and_base_end_\@
  ldr   \irqstat, [\base, #IC_PEND2_OFF]
  mov   \irqnr, #(ARM_IRQ2_BASE + 31)
  /*
   * Bits 21,22,23,24,25,30 in IRQ_GPU2_PENDING register
   * are bits 15,16,17,18,19,20 in IRQ_BASIC_PENDING,
   * we already checked them above, that's why we mask 
   * them out here.
   */ 
  mov   \tmp2, #((1<<21)|(1<<22)|(1<<23)|(1<<24)|(1<<25))
  orr   \tmp2, \tmp2, #(1<<30)
  bic   \irqstat, \irqstat, \tmp2

get_irqnr_and_base_set_irqnr_\@:
  /* 
   * LSB(x) = 31 - CLZ(x^(x-1))
   */ 
  sub   \tmp, \irqstat, #1 
  eor   \irqstat, \irqstat, \tmp
  clz   \tmp, \irqstat
  subs  \irqnr, \irqnr, \tmp

get_irqnr_and_base_end_\@:
.endm
