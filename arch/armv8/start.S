#include <asm_macros.h>
#include <arch/armv8/armv8_asm_macros.h>
#include <arch/armv8/percpu_asm_macros.h>
#include "irq_macros.h"

/*
 * PER_CORE_STARTUP_ADDRESSES_BASE - address for array of
 * function pointers, monitored by each core to jump to.
 *
 * Pseudocode:
 *
 * typedef void (func_ptr*)(void);
 * static func_ptr per_cpu_core_functions[NUMBER_OF_CORES];
 *
 * void wait_to_branch(int cpu_core_id)
 * {
 *   volatile func_ptr f = 0;
 *   while(!f)
 *     f = per_cpu_core_functions[cpu_core_id];
 *   f();
 *
 */
.equ PER_CORE_STARTUP_ADDRESSES_BASE, 0xd8

/*
 * instructions at 0x00
 * d2a80000 @ b900001f @ 52b00001 @ b9000801
 * 58000400 @ d51be000 @ d51ce07f @ d2867fe0
 * d51e1140 @ d280b620 @ d51e1100 @ d2800800
 * d519f220 @ 58000320 @ d51c1000 @ d2807920
 * d51e4000 @ 10000060 @ d51e4020 @ d69f03e0
 * d53800a6 @ 924004c6 @ b40000e6 @ 100003e5
 * d503205f @ f86678a4 @ b4ffffc4 @ d2800000
 * 14000003 @ 18000444 @ 18000400 @ d2800001
 * d2800002 @ d2800003 @ d61f0080 @ 00000000
 * 0124f800 @ 00000000 @ 30c50830 @ 00000000
 *
 * same, but singled lined
 */

.section ".text.boot"
.globl _start

/*
 * Bootstap code at 0x0 first runs at EL3, it goes through the followind setup steps:
 * - Sets EL0, EL1 to Non-Secure mode, no access to Secure Memory
 * - Disables interrupt routing to EL3 from non-EL3
 * - Enables HYP call
 * - Sets EL2 to AArch64 state
 * - Sets SCTLR_EL2 to basic non-MMU non-cached state
 * - Sets SPSR_EL3: all exceptions masked, exception level EL2/SP_EL2
 * - ERETs from EL3 to EL2/SP_EL2
 * - Branches to _start
 */
_start:
  /*
   * Starting in EL2/SP_EL2, exceptions masked, non-MMU
   */

  /*
   * Store HCR_EL2 value that was present at the time of entering
   * _start.
   */
  mrs   x0, hcr_el2
  ldr   x1, =__aarch64_hcr_el2_init_value
  str   x0, [x1]

  /*
   * Wake up other cores from SWE, all cores except this
   * are waiting to branch, monitoring the PER_CORE_STARTUP_ADDRESSES_BASE
   * location (read comments on it). Remaining cores will jump to any
   * location we write there. We set same address for each core and
   * wake them up by SEV (set event)
   * instruction.
   */
  mov   x0, #PER_CORE_STARTUP_ADDRESSES_BASE
  ldr   x1, =__cpu_parking
  str   x1, [x0, #8]
  str   x1, [x0, #16]
  str   x1, [x0, #24]
  sev

/*
 * CPU cores are going to park here, they will:
 * - sleep most of the time.
 * - when awake check if they've been assigned an instruction address to run
 * - when instruction pointer recieved:
 * - - also recieve stack pointer
 * - - run through cpu configuration code
 * - - zero out moset of CPU state
 * - - branch to recieved instruction.
 */
__cpu_parking:
  /*
   * Find the number of running CPU core and fill
   * percpu data with some values.
   */
  cpu_id         .req x0
  pcpu_stack_el0 .req x1
  pcpu_stack_el1 .req x2
  pcpu_mpidr_el1 .req x3
  pcpu_jump_addr .req x4
  pcpu_cpu_state .req x5
  tmp            .req x6
  tmp2           .req x7

  get_cpu_id cpu_id

 /*
  * Deduce stacks for each cpu core
  */
.macro get_cpu_stack cpu, dest, el, tmp, tmp2
  /* \()\el\() - is substitution with \() acting as separators that disabmiguate macro boundaries */
  ldr \tmp, =__stack_size_el\()\el\()_log
  mov \tmp2, \cpu
  lsl \tmp, \tmp2, \tmp
  ldr \dest, =__stacks_el\()\el\()_base
  add \dest, \dest, tmp
.endm

  get_cpu_stack cpu_id, pcpu_stack_el1, 1, tmp, tmp2
  get_cpu_stack cpu_id, pcpu_stack_el0, 0, tmp, tmp2

  /*
   * Set jump address. Only core 0 will have this set to non-zero value.
   * other's will wait for it to be set latet from scheduler code.
   */
  cbz  cpu_id, 1f
  /*
   * non-0 cores jump_addr are set to zero.
   */
  mov  pcpu_jump_addr, xzr
  b 2f
1:
  /*
   * core 0 jump set to proceed to main
   */
  adr pcpu_jump_addr, __jump_addr_core0
2:
  /*
   * MPIDR_EL1 is saved for debug of cpu startup context.
   */
  mrs   pcpu_mpidr_el1, mpidr_el1

  /*
   * set initial cpu state address. This cpu state is the buffer,
   * to where state is stored during exception before task scheduling
   * starts up.
   */
  ldr  pcpu_cpu_state, =__init_cpu_state
  ldr  tmp, =__init_cpu_state_max_size
  mul  tmp, cpu_id, tmp
  add  pcpu_cpu_state, pcpu_cpu_state, tmp

  /*
   * percpu_data is an array of $NUM_CORES structs
   * each core has it's own cpu_id to access corresponding element
   * in that array.
   */
  percpu_init cpu_id, \
    pcpu_stack_el0, \
    pcpu_stack_el1, \
    pcpu_jump_addr, \
    pcpu_mpidr_el1, \
    pcpu_cpu_state, \
    tmp

  .unreq cpu_id
  .unreq pcpu_stack_el0
  .unreq pcpu_stack_el1
  .unreq pcpu_mpidr_el1
  .unreq pcpu_jump_addr
  .unreq pcpu_cpu_state
  .unreq tmp
  .unreq tmp2

  /*
   * Get current exception level
   * bits [3:2] are exception level
   */
.equ CURRENT_EL_EL0, 0b00
.equ CURRENT_EL_EL1, 0b01
.equ CURRENT_EL_EL2, 0b10
.equ CURRENT_EL_EL3, 0b11

  mrs   x0, CurrentEl
  lsr   x0, x0, #2
  and   x0, x0, #3

  /*
   * Check if we are in EL1 execution state. If not set it.
   * x0 = current el.
   */
  cmp   x0, #CURRENT_EL_EL1
  beq   __start_el1

  /*
   * Set up EL1.
   */
  // ***************************************************************** //
  // enable CNTP for EL1
  // mrs   x0, cnthctl_el2
  // orr   x0, x0, #3
  // msr   cnthctl_el2, x0   // Hypervisor Configuration Register Enables second stage translation for execution in Non-secure EL1 and EL0.
  // ***************************************************************** //

  msr   cntvoff_el2, xzr

  bl    _disable_coproc_traps

  /*
   * Enable AArch64 in EL1 execution state for EL1.
   * HCR_EL2.RW bit - Execution state for EL1 is AArch64,
   *                - Execution state for EL0 is taken from PSTATE.nRW when at EL0
   */
  mov   x0, #(1 << 31)

  // HCW_EL2.SWIO   - 1
  // SWIO hardwired on Pi3 Set/Way Invalidation Override
  orr   x0, x0, #(1 << 1)
  msr   hcr_el2, x0

  /*
   * Setup SCTLR access x2 = 0x30d00800
   * SCTLR.EOS    = 1 Exeption exit is context synchronizing
   * SCTLR.TSCTX  = 1 Trap EL0 access to SCTXNUM_EL0 reg when EL0 in AArch64
   * SCTLR.EIS    = 1 Exception entry is synchronizing
   * SCTLR.SPAN   = 1 PSTATE.PAN is unchanged on taking an exception to EL1
   *                  (Set Privelidged Access Never = false)
   * SCTLR.TLSMD  = 1 All EL0 A32/T32 accesses to stage 1 marked as
   *                  Device-nGRE/Device-nGnRE/Device-nGnRnE are not trapped
   * SCTLR.LSMAOE = 1 Load multiple/Store multiple Atomicity and Ordering Enable
   */
  mov   x0, #0x0800
  movk  x0, #0x30d0, lsl #16
  msr   sctlr_el1, x0

  /*
   * Setup exception handlers
   */
  ldr   x0, =_vectors
  msr   vbar_el1, x0

  /*
   * Change execution level to EL1.
   * Set SPSR_EL2 to simulate exception context to which to return
   * spsr_el2 = 0x3c4
   * SPSR_EL2.M = EL1 (0b0100) - exception came from EL1
   * SPSR_EL2.F = 1 - FIQ interrupts will be masked upon returning to EL1
   * SPSR_EL2.I = 1 - IRQ interrupts will be masked upon returning to EL1
   * SPSR_EL2.A = 1 - SError interrupts will be masked upon returning to EL1
   * SPSR_EL2.D = 1 - Debug interrupts will be masked upon returning to EL1
   */
  mov   x0, #0x3c4
  msr   spsr_el2, x0

  /*
   * Set ELR_EL2 to point to __start_el1 from where it proceeds execution
   */
  adr   x0, __start_el1
  msr   elr_el2, x0

  /*
   * Set EL1 stack to _start
   */

  /*
   * This is something we are not allowed to do if we are not at EL3,
   * so we have to setup stack at el1
   * ldr   x0, =_start
   * msr   sp_el2, x0
   */

  /*
   * Return from fake exception.
   */
  eret

__clear_bss:
  ldr   x0, =__bss_start
  ldr   w1, =__bss_size
1:
  cbz   w1, 1f
  str   xzr, [x0], #8
  sub   w1, w1, #8
  cbnz  w1, 1b
1:
  ret

__start_el1:
  /*
   * Use sp_el0
   * While catching exception from el1 to el1
   * use sp_el0 to store stack pointer to
   * restore to after eret
   * sp_el1 is not convenient to write/read
   */
_set_stack:
.macro cpu_set_stack_ptrs cpu_id, el, tmp
  mov   \tmp, #\()\el
  msr   SPSel, \tmp
  percpu_data_get_stack \cpu_id \el \tmp \tmp
  mov   sp, \tmp
.endm

  cpu_id      .req x0
  tmp         .req x1

  get_cpu_id cpu_id

  cpu_set_stack_ptrs cpu_id, 1, tmp
  cpu_set_stack_ptrs cpu_id, 0, tmp

__parking_loop:
  jump_addr .req x2
  percpu_data_get_jmp cpu_id, jump_addr, tmp
  cbz  jump_addr, __parking_loop_wait
__parking_loop_exit:
  br   jump_addr
__parking_loop_wait:
  wfe
  b __parking_loop


__jump_addr_core0:
  /*
   * Zero out .bss section
   */
  bl __clear_bss
2:
  // clear GP registers
  mov x0, xzr
  mov x1, xzr
  mov x2, xzr
  mov x3, xzr
  mov x4, xzr
  mov x5, xzr
  mov x6, xzr
  mov x7, xzr
  mov x8, xzr
  mov x9, xzr
  mov x10, xzr
  mov x11, xzr
  mov x12, xzr
  mov x13, xzr
  mov x14, xzr
  mov x15, xzr
  mov x16, xzr
  mov x17, xzr
  mov x18, xzr
  mov x19, xzr
  mov x20, xzr
  mov x21, xzr
  mov x22, xzr
  mov x23, xzr
  mov x24, xzr
  mov x25, xzr
  mov x26, xzr
  mov x27, xzr
  mov x28, xzr
  mov x29, xzr
  mov x30, xzr

  bl    main
1:
  wfe
  b 1b


LFUNC(_cpu_wait_for_event):
  wfe
  b _cpu_wait_for_event

.pushsection ".data"

// 8 byte aligned cpu context
.align 3
.globl __initial_cpuctx
__initial_cpuctx:
.rept 80
.quad 0x00
.endr

.align 3
GLOBAL_VAR(__exception_info):
.rept 8
.quad 0x00
.endr

/*
 * pointer to current cpu context
 */
.align 6
GLOBAL_VAR(__current_cpuctx):
.rept 4
.rept 8
.quad 0
.endr
.endr

GLOBAL_VAR(__aarch64_hcr_el2_init_value):
.quad 0

.popsection

.pushsection ".el2_stack", "a"
.align 12
/* 4Kb aligned address of EL1 exception stack area */
.globl __el2_stack_top
__el2_stack_top:

.org  . + 0x4000
.globl __el2_stack_base
__el2_stack_base:
.popsection

LFUNC(__prep_exception_info):
  // x0 - exception type
  // x1 - top of stack at exception enter
  mov   x8, x1
  ldr   x5, =__exception_info
  ldr   x6, =__current_cpuctx
  ldr   x6, [x6]
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
  ret

.macro __irq_handler type
.align 7
.globl __irq_handler_\()\type
__irq_handler_\()\type:
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
1:
  get_irqnr_and_base irqnr_reg, irqstat_reg, base_reg, tmp_1_reg, tmp_2_reg
  beq  1f
  bl   __handle_irq
  b    1b
1:
  b     __armv8_cpuctx_eret
.unreq base_reg
.endm

.macro __vector_entry symbol handler type
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

.macro vector_entry symbol type
  __vector_entry \symbol __handle_interrupt \type
.endm

.align 11
_vectors:
vector_entry __interrupt_cur_el_sp0_synchronous   0
__irq_handler cur_el_sp0
vector_entry __interrupt_cur_el_sp0_fiq           2
vector_entry __interrupt_cur_el_sp0_serror        3

vector_entry __interrupt_cur_el_spx_synchronous   0
__irq_handler cur_el_spx
vector_entry __interrupt_cur_el_spx_fiq           2
vector_entry __interrupt_cur_el_spx_serror        3

vector_entry __interrupt_low_el_sp0_synchronous 0
__irq_handler low_el_sp0
vector_entry __interrupt_low_el_sp0_fiq         2
vector_entry __interrupt_low_el_sp0_serror      3

vector_entry __interrupt_low_el_spx_synchronous 0
__irq_handler low_el_spx
vector_entry __interrupt_low_el_spx_fiq         2
vector_entry __interrupt_low_el_spx_serror      3


_disable_coproc_traps:
  // Trap SVE , dont trap SIMD/Floating point instructions

  // bits [7:0] -> RES1
  // bit  8     -> TZ, Trap SVE (Scalable Vector Extension) instructions
  //               at EL0, EL1 and EL2
  // bit  9     -> RES1
  // bit 10     -> TFP, Trap access to SVE FPCR, FPSR, FPEXC32_EL2 (SIMD/FP)
  //               Set to 0 means dont trap
  // [13:12] -> RES1
  // => 0x33ff
  mov   x0, #0x33ff
  msr   cptr_el2, x0

  // Disable access to coproc == 0b1111 encoding space in AArch32
  msr   hstr_el2, xzr

  // Do not trap accesses to SVE, SIMD or Floating point registers
  // to EL1
  mov   x0, #(3 << 20)
  msr   cpacr_el1, x0
  ret
