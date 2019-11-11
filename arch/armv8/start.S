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
  mrs   x1, mpidr_el1
  and   x1, x1, #3        // get CPU core number
  cbz   x1, 2f            // jump if core number is 0
  // cpu id > 0, stop
1: 
  wfe
  b 1b
2: // cpu id == 0
  
  // set stack 
  ldr   x1, =_start

  // set up EL1
  mrs   x0, CurrentEl
  and   x0, x0, #0xc // clear reserved bits
  cmp   x0, #0xc
  bne 5f
  mov   sp, x1

5: 
  cmp   x0, #4
  beq   5f
  msr   sp_el1, x1
  // enable CNTP for EL1
  mrs   x0, cnthctl_el2
  orr   x0, x0, #3
  msr   cnthctl_el2, x0   // Hypervisor Configuration Register Enables second stage translation for execution in Non-secure EL1 and EL0.
  msr   cntvoff_el2, xzr  // 
  bl    _disable_coproc_traps
  // enable AArch64 in EL1
  mov   x0, #(1 << 31)    // AArch64
  orr   x0, x0, #(1 << 1) // SWIO hardwired on Pi3 Set/Way Invalidation Override 
  msr   hcr_el2, x0
  mrs   x0, hcr_el2
  // Setup SCTLR access x2 = 0x30d00800
  mov   x2, #0x0800
  movk  x2, #0x30d0, lsl #16
  msr   sctlr_el1, x2
  // setup exception handlers
  ldr   x2, =_vectors
  msr   vbar_el1, x2
  // change execution level to EL1
  mov   x2, #0x3c4
  // [9:8] [7:6] [3] -> bits D, A, I, F, M 0b0100
  msr   spsr_el2, x2
  adr   x2, 5f
  msr   elr_el2, x2
  eret
5:
  mov   sp, x1

  // clear bss
  ldr   x1, =__bss_start 
  ldr   w2, =__bss_size
3:
  cbz   w2, 4f
  str   xzr, [x1], #8
  sub   w2, w2, #1
  cbnz  w2, 3b
4:
  bl    main
  b     1b
  
.globl timer_irq_init
timer_irq_init:
  mov   w1, #0
  str   w1, [x0, #4]
  ldr   x0, =0x3b00b400
  ldr   w1, =0xa00
  str   w1, [x0]
  ldr   w1, [x0, #4]
  mov   w2, #0xa2
  orr   w1, w1, w2
  str   w1, [x0, #4]
  ret

.macro vector_entry symbol
  .align 7
  .globl \symbol
  mov   x0, #0
  mrs   x1, esr_el1
  mrs   x2, elr_el1
  mrs   x3, spsr_el1
  mrs   x4, far_el1
  b exc_handler
.endm
  

// important, code has to be properly aligned
.align 11
_vectors:
vector_entry __interrupt_sp0_synchronous
vector_entry __interrupt_sp0_irq
vector_entry __interrupt_sp0_fiq
vector_entry __interrupt_sp0_serror
vector_entry __interrupt_spx_synchronous
vector_entry __interrupt_spx_irq
vector_entry __interrupt_spx_fiq
vector_entry __interrupt_spx_serror
vector_entry __interrupt_l_sp0_synchronous
vector_entry __interrupt_l_sp0_irq
vector_entry __interrupt_l_sp0_fiq
vector_entry __interrupt_l_sp0_serror
vector_entry __interrupt_l_spx_synchronous
vector_entry __interrupt_l_spx_irq
vector_entry __interrupt_l_spx_fiq
vector_entry __interrupt_l_spx_serror

  
_disable_coproc_traps:
  // [9:0] -> RES1
  // [10]  => 0 to disable FPU trap
  // [13:12] -> RES1
  // => 0x33ff
  mov   x0, #0x33ff
  msr   cptr_el2, x0
  msr   hstr_el2, xzr
  mov   x0, #(3 << 20)
  msr   cpacr_el1, x0
  ret

