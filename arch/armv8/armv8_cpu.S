#include <asm_macros.h>

.section ".text"

FUNC(__armv8_cpuctx_store):
  // of all general purpose registers
  // x0 - x29 are uncorrupted and
  // link register x30 is in [sp]
  str   x30, [sp, #-8]
  ldr   x30, =__current_cpuctx
  ldr   x30, [x30]

  stp   x0 , x1 , [x30], #16
  stp   x2 , x3 , [x30], #16
  stp   x4 , x5 , [x30], #16
  stp   x6 , x7 , [x30], #16
  stp   x8 , x9 , [x30], #16
  stp   x10, x11, [x30], #16
  stp   x12, x13, [x30], #16
  stp   x14, x15, [x30], #16
  stp   x16, x17, [x30], #16
  stp   x18, x19, [x30], #16
  stp   x20, x21, [x30], #16
  stp   x22, x23, [x30], #16
  stp   x24, x25, [x30], #16
  stp   x26, x27, [x30], #16
  stp   x28, x29, [x30], #16

  // store original link register (x30)
  // of the caller function
  ldr   x28, [sp] 

  // store stack pointer from a task
  // before exception happened
  mrs   x29, sp_el0
  stp   x28, x29, [x30], #16
  // store program counter from a task
  // before exception happened
  mrs   x28, elr_el1
  str   x28, [x30]

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
  ldp   x0 , x1 , [x30], #16
  ldp   x2 , x3 , [x30], #16
  ldp   x4 , x5 , [x30], #16
  ldp   x6 , x7 , [x30], #16
  ldp   x8 , x9 , [x30], #16
  ldp   x10, x11, [x30], #16
  ldp   x12, x13, [x30], #16
  ldp   x14, x15, [x30], #16
  ldp   x16, x17, [x30], #16
  ldp   x18, x19, [x30], #16
  ldp   x20, x21, [x30], #16
  ldp   x22, x23, [x30], #16
  ldp   x24, x25, [x30], #16
  ldp   x26, x27, [x30], #16
  ldp   x28, x29, [x30], #16
  ldr   x30, [x30]
  eret
