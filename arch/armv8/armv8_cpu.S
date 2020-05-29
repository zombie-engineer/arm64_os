#include <asm_macros.h>
#include "armv8_asm_macros.h"

.section ".text"

.macro STORE_REGS_X0_X29, to
  stp   x0 , x1 , [\to], #16
  stp   x2 , x3 , [\to], #16
  stp   x4 , x5 , [\to], #16
  stp   x6 , x7 , [\to], #16
  stp   x8 , x9 , [\to], #16
  stp   x10, x11, [\to], #16
  stp   x12, x13, [\to], #16
  stp   x14, x15, [\to], #16
  stp   x16, x17, [\to], #16
  stp   x18, x19, [\to], #16
  stp   x20, x21, [\to], #16
  stp   x22, x23, [\to], #16
  stp   x24, x25, [\to], #16
  stp   x26, x27, [\to], #16
  stp   x28, x29, [\to], #16
.endm

.macro LOAD_REGS_X0_X30, from
  ldp   x0 , x1 , [\from], #16
  ldp   x2 , x3 , [\from], #16
  ldp   x4 , x5 , [\from], #16
  ldp   x6 , x7 , [\from], #16
  ldp   x8 , x9 , [\from], #16
  ldp   x10, x11, [\from], #16
  ldp   x12, x13, [\from], #16
  ldp   x14, x15, [\from], #16
  ldp   x16, x17, [\from], #16
  ldp   x18, x19, [\from], #16
  ldp   x20, x21, [\from], #16
  ldp   x22, x23, [\from], #16
  ldp   x24, x25, [\from], #16
  ldp   x26, x27, [\from], #16
  ldp   x28, x29, [\from], #16
  ldr   x30, [\from]
.endm

.macro ZERO_REGS_X0_X29, __zero_reg, __ctx_reg
  mov   \__zero_reg, #0
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x0, x1
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x2, x3
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x4, x5
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x6, x7
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x8, x9
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x10, x11
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x12, x13
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x14, x15
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x16, x17
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x18, x19
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x20, x21
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x22, x23
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x24, x25
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x26, x27
  stp   \__zero_reg, \__zero_reg, [\__ctx_reg], #16 // x28, x29
.endm

FUNC(__armv8_cpuctx_store):
  // of all general purpose registers
  // x0 - x29 are uncorrupted and
  // link register x30 is in [sp]
  str   x30, [sp, #-8]
  ldr   x30, =__current_cpuctx
  ldr   x30, [x30]

  STORE_REGS_X0_X29 x30

  // store original link register (x30)
  // of the caller function
  ldr   x28, [sp] 

  // store stack pointer from a task
  // before exception happened
  mrs   x29, sp_el0
  stp   x28, x29, [x30], #16
  // store program counter and cpsr from a task
  // before exception happened
  mrs   x28, elr_el1
  mrs   x29, spsr_el1
  stp   x28, x29, [x30]

  // restore this function's link register (x30)
  // to return
  ldr   x30, [sp, #-8]
  ret


FUNC(__armv8_cpuctx_eret):
  ldr   x30, =__current_cpuctx
  ldr   x30, [x30]
  // restore elr and sp0 first 
  ldp   x0 , x1 , [x30, #(31 << 3)]
  msr   sp_el0, x0
  msr   elr_el1, x1
  // restore spsr
  ldr   x0 , [x30, #(33 << 3)]
  // msr   spsr_el1, x0
  LOAD_REGS_X0_X30 x30
  eret

/*
 * Ready context to jump to second part of yield
 * function. This is needed to restore irq after
 * jump to initial context is complete
 */
FUNC(__armv8_prep_context):
  // x0 - sp for a new task
  // x1 - link register value
  // x2 - flags
  // x3 - cpu_context
  stack     .req x0
  link      .req x1
  flags     .req x2
  ctx       .req x3
  tmp       .req x4
  ZERO_REGS_X0_X29 tmp, ctx
  // store x30(link) and sp
  stp   link, stack, [ctx], #16
  .unreq stack
  .unreq link
  // zero out pc, put flags in cpsr
  stp   tmp, flags, [ctx], #16
  .unreq tmp
  .unreq flags
  .unreq ctx
  ret

FUNC(__armv8_pickup_context):
  // x0 - cpu context to restore
  ldr x30, =__current_cpuctx
  str x0, [x30]
  mov x30, x0
  ctx .req x30
  stack .req x0
  // restore stack pointer
  ldr stack, [ctx, #(31 << 3)]
  mov sp, stack
  .unreq stack
  tmpdaif .req x0
  // do not restore pc
  // restore cpsr and put it on stack
  ldr tmpdaif, [ctx, #(33 << 3)] 
  str tmpdaif, [sp, #-8]
  .unreq tmpdaif
  LOAD_REGS_X0_X30 ctx
  str x0, [sp, #-16]
  tmpdaif .req x0
  ldr tmpdaif, [sp, #-8]
  msr daif, tmpdaif
  .unreq tmpdaif
  ldr x0, [sp, #-16]
  ret

.macro STORE_CONTEXT
  // x30 - is a link register
  // at store context we will use x30 as a writing 
  // so we first stack it to later recover
  str   x30, [sp, #-8]
  // now let's write start of context to x30
  ldr   x30, =__current_cpuctx
  ldr   x30, [x30]

  STORE_REGS_X0_X29 x30

  // switch writing register from x30 to x0
  // and pickup x30 original value from the stack
  mov   x0, x30
  ldr   x30, [sp, #-8]
  mov   x1, sp

  // store x30(link register) and sp
  stp   x30, x1, [x0], #16

  // make patched program counter, that
  // will point at end second part of yield
  // function for proper return
  adr   x30, __return_from_yield_addr
  ldr   x30, [x30]
  mov   x30, #1
  mov   x30, #1
  
  // make cpsr
  mrs   x1, daif
  stp  x30, x1, [x0], #16
.endm

.macro RESTORE_CONTEXT
  ldr   x30, =__current_cpuctx
  ldr   x0, [x30, #(31 << 3)]
  mov   sp, x0
  LOAD_REGS_X0_X30 x30
 // ldr   x30, [x30, ]
  // restore daif
.endm

/*
 * 1. disable irq
 * 2. save current cpu context
 * 3. patch stored cpu context pc to resume after scheduling
 * 4. choose next task
 * 5. restore next task cpu context
 * 6. enable irq
 */
FUNC(__armv8_yield):
  // 1
  IRQ_DISABLE
  // 2,3
  STORE_CONTEXT
  // 4
  bl schedule

FUNC(__armv8_ret_to_task):
  RESTORE_CONTEXT
__return_from_yield_addr:
  ret

