.section ".text.boot"
.global _start

_start:
  // read cpu id, stop slave cores
  mrs   x1, mpidr_el1
  and   x1, x1, #3
  cbz   x1, 2f
  // cpu id > 0, stop
1: 
  wfe
  b 1b
2: // cpu id == 0
  
  // set stack 
  ldr   x1, =_start

  // set up EL1
  mrs   x0, CurrentEL
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
  msr   cnthctl_el2, x0
  msr   cntvoff_el2, xzr
  // enable AArch64 in EL1
  mov   x0, #(1 << 31)    // AArch64
  orr   x0, x0, #(1 << 1) // SWIO hardwired on Pi3
  msr   hcr_el2, x0
  mrs   x0, hcr_el2
  // change execution level to EL1
  mov   x2, #0x3c4
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
  
