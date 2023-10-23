#include <config.h>
#include "irq_macros.h"
#include <arch/armv8/percpu_asm_macros.h>
#include <arch/armv8/armv8_asm_macros.h>

.macro vector_entry_irq type, cpu_num
.align 7
.globl __irq_handler_\type\cpu_num
__irq_handler_\type\cpu_num:
/*
 * x19 to x29 are callee-saved registers, so
 * we can safely use them across subroutine calls.
 */

/*
 * base_reg is the only one that needs 64 bit register
 * for load operations.
 */
base_reg .req x19

/*
 * temporary registers don't need to be saved after
 * subroutine call, obviously
 */

/*
 * irqstat is a temp register with name)
 */
irqstat_reg .req w3

/*
 * irqnr will be passed to handler, so it is
 * also corruptable. It's used as first argument,
 * so it has to be w0
 */

irqnr_reg .req w0

/*
 * temp registers
 */
tmp_1_reg .req w4
tmp_2_reg .req w5
  str   x30, [sp, #-8]!
  bl    __armv8_cpuctx_store
  add   sp, sp, 8

  get_irqrn_preamble base_reg
  get_irqnr_and_base irqnr_reg, irqstat_reg, base_reg, tmp_1_reg, tmp_2_reg
  beq  1f
  bl   __handle_irq_\cpu_num
1:
  b     __armv8_cpuctx_eret
.unreq base_reg
.endm
.macro __vector_entry symbol, handler, type
  .align 7
  .globl \symbol
\symbol:
  /*
   * Entry can only occupy 0x80 bytes. So most of the
   * cpu context save/restore code has to be done via
   * subroutine calls. With branching instructions we
   * immediately loose link register ("x30" or "lr").
   * We have to allocate stack now and put it there.
   * "Save context" subroutine will have to take it
   * from stack.
   */

  /*
   * Allocate 8 bytes on stack.
   * Put link-register (x30 / lr) to this allocated space
   * on stack.
   * sp -= 8; *sp = x30;
   */
  str   x30, [sp, #-8]!

  bl    __armv8_cpuctx_store
  add   sp, sp, 8
  mov   x0, #\type
  mov   x1, sp
  bl    __prep_exception_info
  bl    \handler
  b     __armv8_cpuctx_eret
.endm
.macro vector_entry symbol, type, cpu_num
  __vector_entry \symbol\()_\()\cpu_num, __handle_interrupt, \type
.endm

.macro VECTORS cpu_num
.align 11
vector_entry __interrupt_cur_el_sp0_synchronous, 0, \cpu_num
vector_entry_irq cur_el_sp0                       , \cpu_num
vector_entry __interrupt_cur_el_sp0_fiq          2, \cpu_num
vector_entry __interrupt_cur_el_sp0_serror       3, \cpu_num
vector_entry __interrupt_cur_el_spx_synchronous  0, \cpu_num
vector_entry_irq cur_el_spx                       , \cpu_num
vector_entry __interrupt_cur_el_spx_fiq          2, \cpu_num
vector_entry __interrupt_cur_el_spx_serror       3, \cpu_num
vector_entry __interrupt_low_el_sp0_synchronous, 0, \cpu_num
vector_entry_irq low_el_sp0                       , \cpu_num
vector_entry __interrupt_low_el_sp0_fiq          2, \cpu_num
vector_entry __interrupt_low_el_sp0_serror       3, \cpu_num
vector_entry __interrupt_low_el_spx_synchronous  0, \cpu_num
vector_entry_irq low_el_spx                       , \cpu_num
vector_entry __interrupt_low_el_spx_fiq          2, \cpu_num
vector_entry __interrupt_low_el_spx_serror       3, \cpu_num
.endm

.text
.globl __vectors
__vectors:
VECTORS 0
VECTORS 1
VECTORS 2
VECTORS 3

__prep_exception_info:
  // x0 - exception type
  // x1 - top of stack at exception enter
  mov   x8, x1
  ldr   x5, =__exception_info
ctx .req x6
tmp .req x7
  GET_CURRENT_CTX ctx, tmp
  // ldr   ctx, [ctx]
  mov   x7, x5
  mrs   x1, esr_el1
  mrs   x2, spsr_el1
  mrs   x3, elr_el1
  mrs   x4, far_el1
  ldr   x9, =__stack_base
  stp   x1, x2, [x5], #16
  stp   x3, x4, [x5], #16
  stp   x0, x6, [x5], #16
  stp   x8, x9, [x5]
  mov   x0, x7
.unreq ctx
.unreq tmp
  ret
