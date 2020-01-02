// @ instructions at 0x00
// @ d2a80000 @ b900001f @ 52b00001 @ b9000801
// @ 58000400 @ d51be000 @ d51ce07f @ d2867fe0
// @ d51e1140 @ d280b620 @ d51e1100 @ d2800800
// @ d519f220 @ 58000320 @ d51c1000 @ d2807920
// @ d51e4000 @ 10000060 @ d51e4020 @ d69f03e0
// @ d53800a6 @ 924004c6 @ b40000e6 @ 100003e5
// @ d503205f @ f86678a4 @ b4ffffc4 @ d2800000
// @ 14000003 @ 18000444 @ 18000400 @ d2800001
// @ d2800002 @ d2800003 @ d61f0080 @ 00000000
// @ 0124f800 @ 00000000 @ 30c50830 @ 00000000
// @ 
// @ same, but singled lined 

.section ".text.boot"
.global _start

// Bootstap code at 0x0 first runs at EL3, it goes through the followind setup steps:
// - Sets EL0, EL1 to Non-Secure mode, no access to Secure Memory
// - Disables interrupt routing to EL3 from non-EL3
// - Enables HYP call
// - Sets EL2 to AArch64 state
// - Sets SCTLR_EL2 to basic non-MMU non-cached state
// - Sets SPSR_EL3: all exceptions masked, exception level EL2/SP_EL2
// - ERETs from EL3 to EL2/SP_EL2
// - Branches to _start
_start:
  // Start in EL2/SP_EL2, exceptions masked, non-MMU
  // read cpu id, stop slave cores

  // Store arm_init_regvalue
  mrs   x1, hcr_el2
  ldr   x2, =__arm_initial_reg_value_address_hcr_el2
  str   x1, [x2]

  mrs   x1, mpidr_el1
  // get CPU core number
  and   x1, x1, #3        
  // jump if core number is 0
  cbnz   x1, _cpu_wait_for_event
  // cpu id > 0, stop


2: // cpu id == 0
  
  // set stack 
  ldr   x1, =_start

  // set up EL1
  // Get current exception level
  mrs   x0, CurrentEl
  // bits [3:2] are exception level

  // 0b00 - EL0, 0b01 - EL1, 0b10 - EL2, 0b11 - EL3
  and   x0, x0, #(3 << 2)  
  cmp   x0, #(3 << 2)
  // check if EL3
  bne   _check_el1
  mov   sp, x1


// ******************************************************************************
// Check if we are in EL1 execution state and set it if not
// ******************************************************************************
.local _check_el1
_check_el1: 
  // check if EL1
  cmp   x0, #(1 << 2)
  beq   _in_el1
  // on not EL3 and not EL1
  // Set EL1 stack to _start
  msr   sp_el1, x1

  // ***************************************************************** // 
  // enable CNTP for EL1
  // mrs   x0, cnthctl_el2
  // orr   x0, x0, #3
  // msr   cnthctl_el2, x0   // Hypervisor Configuration Register Enables second stage translation for execution in Non-secure EL1 and EL0.
  // ***************************************************************** // 

  msr   cntvoff_el2, xzr  // 

  bl    _disable_coproc_traps

  // enable AArch64 in EL1


  // HCR_EL2.RW bit - Execution state for EL1 is AArch64,
  //                - Execution state for EL0 is taken from PSTATE.nRW when at EL0
  mov   x0, #(1 << 31)    // AArch64

  // HCW_EL2.SWIO   - 1 
  // SWIO hardwired on Pi3 Set/Way Invalidation Override 
  orr   x0, x0, #(1 << 1) 
  msr   hcr_el2, x0

  // ***************************************************************** // 
  // mrs   x0, hcr_el2
  // ***************************************************************** // 


  // Setup SCTLR access x2 = 0x30d00800
  // SCTLR.EOS    = 1 Exeption exit is context synchronizing
  // SCTLR.TSCTX  = 1 Trap EL0 access to SCTXNUM_EL0 reg when EL0 in AArch64
  // SCTLR.EIS    = 1 Exception entry is synchronizing
  // SCTLR.SPAN   = 1 PSTATE.PAN is unchanged on taking an exception to EL1
  //                  (Set Privelidged Access Never = false)
  // SCTLR.TLSMD  = 1 All EL0 A32/T32 accesses to stage 1 marged as 
  //                  Device-nGRE/Device-nGnRE/Device-nGnRnE are not trapped
  // SCTLR.LSMAOE = 1 Load multiple/Store multiple Atomicity and Ordering Enable

  mov   x2, #0x0800
  movk  x2, #0x30d0, lsl #16
  msr   sctlr_el1, x2


  // Setup exception handlers
  ldr   x2, =_vectors
  msr   vbar_el1, x2



  // Change execution level to EL1

  // Set SPSR_EL2 to simulate exception context to which to return
  // SPSR_EL2.M = EL1 (0b0100) - exception came from EL1
  // SPSR_EL2.F = 1 - FIQ interrupts will be masked upon returning to EL1
  // SPSR_EL2.I = 1 - IRQ interrupts will be masked upon returning to EL1
  // SPSR_EL2.A = 1 - SError interrupts will be masked upon returning to EL1
  // SPSR_EL2.D = 1 - Debug interrupts will be masked upon returning to EL1
  mov   x2, #0x3c4
  msr   spsr_el2, x2

  // Set ELR_EL2 to point to _in_el1 from where it proceeds execution
  adr   x2, _in_el1
  msr   elr_el2, x2
  // Jump out of exception
  eret



// ******************************************************************************
// In EL1
// ******************************************************************************
.local _in_el1
_in_el1:
  mov   x2, #1
  msr   SPSel, x2
  // Set stack
  mov   sp, x1

  // clear bss
  ldr   x1, =__bss_start 
  ldr   w2, =__bss_size
1:
  cbz   w2, 2f
  str   xzr, [x1], #8
  sub   w2, w2, #1
  cbnz  w2, 1b


2:
  // clear GP registers
  mov x0, #0
  mov x1, #0
  mov x2, #0
  mov x3, #0
  mov x4, #0
  mov x5, #0
  mov x6, #0
  mov x7, #0
  mov x8, #0
  mov x9, #0
  mov x10, #0
  mov x11, #0
  mov x12, #0
  mov x13, #0
  mov x14, #0
  mov x15, #0
  mov x16, #0
  mov x17, #0
  mov x18, #0
  mov x19, #0
  mov x20, #0
  mov x21, #0
  mov x22, #0
  mov x23, #0
  mov x24, #0
  mov x25, #0
  mov x26, #0
  mov x27, #0
  mov x28, #0
  mov x29, #0
  mov x30, #0

  // Jump to main
  bl    main
  // After main returns halt 
  b     _cpu_wait_for_event

// ******************************************************************************

.local _cpu_wait_for_event
_cpu_wait_for_event: 
  wfe
  b _cpu_wait_for_event

// 8 byte aligned cpu context
.align 3
.globl __aarch64_cpu_ctx
__aarch64_cpu_ctx:
.rept 33
.quad 0x6666666666666666
.endr

.align 3
.globl __exception_info
__exception_info:
.rept 7
.quad 0x6666666666666666
.endr


.local __store_cpu_ctx
__store_cpu_ctx:
  // of all general purpose registers
  // x0 - x29 are uncorrupted and
  // link register x30 is in [sp]
  str   x30, [sp, #-8]
  ldr   x30, =__aarch64_cpu_ctx

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
  mov   x29, #0
  stp   x28, x29, [x30], #16
  mrs   x28, elr_el1
  str   x28, [x30]

  // restore this function's link register (x30)
  // to return
  ldr   x30, [sp, #-8]
  ret

.local __eret_to_cpu_ctx
__eret_to_cpu_ctx:
  ldr   x30 , =__aarch64_cpu_ctx
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


.local __prep_exception_info
__prep_exception_info:
  // x0 - exception type
  // x1 - top of stack at exception enter
  mov   x8, x1
  ldr   x5, =__exception_info
  ldr   x6, =__aarch64_cpu_ctx
  mov   x7, x5
  mrs   x1, esr_el1
  mrs   x2, spsr_el1
  mrs   x3, elr_el1
  mrs   x4, far_el1
  stp   x1, x2, [x5], #16
  stp   x3, x4, [x5], #16
  stp   x0, x6, [x5], #16
  str   x8, [x5]
  mov   x0, x7
  ret


.macro __vector_entry symbol handler type
  .align 7
  .globl \symbol
\symbol:
# Entry can only occupy 0x80 bytes
  # x30 is lr (link register)
  # it should be saved in a correct order
  str   x30, [sp, #-8]!
  bl    __store_cpu_ctx
  add   sp, sp, 8
  mov   x0, #\type
  mov   x1, sp
  bl    __prep_exception_info  
  bl    \handler
  b     __eret_to_cpu_ctx
.endm

.macro vector_entry symbol type
  __vector_entry \symbol __handle_interrupt \type
.endm


// important, code has to be properly aligned
// VBAR_EL1
.align 11
_vectors:
// 
vector_entry __interrupt_cur_el_sp0_synchronous   0
vector_entry __interrupt_cur_el_sp0_irq           1
vector_entry __interrupt_cur_el_sp0_fiq           2
vector_entry __interrupt_cur_el_sp0_serror        3

vector_entry __interrupt_cur_el_spx_synchronous   0
vector_entry __interrupt_cur_el_spx_irq           1
vector_entry __interrupt_cur_el_spx_fiq           2
vector_entry __interrupt_cur_el_spx_serror        3

vector_entry __interrupt_low_el_sp0_synchronous 0
vector_entry __interrupt_low_el_sp0_irq         1
vector_entry __interrupt_low_el_sp0_fiq         2
vector_entry __interrupt_low_el_sp0_serror      3

vector_entry __interrupt_low_el_spx_synchronous 0
vector_entry __interrupt_low_el_spx_irq         1
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

