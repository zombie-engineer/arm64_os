.include "atmega8.inc"
.include "asm_macros.h"
.text

.equ fosc, 1000000
.text

.globl init_delay 
init_delay:
  ret
  

.globl delay_ns
delay_ns:
  push yh
  push yl
  clr r18
   
1: ; 10 operations loop
  inc  r18
  cpse r18, r19
  rjmp 1b

  pop yl
  pop yh
  ret

; void delay_cycles_16(uint16_t num_cycles)
; r24:r25 - num_cycles
.globl delay_cycles_16
delay_cycles_16:
  lsr  r25
  ror  r24
  lsr  r25
  ror  r24
  sbiw r24, 1
  brmi 2f
1:
  sbiw r24, 1; 2 cycles
  brpl 1b    ; 1 cycles
  nop        ; 1 cycles
2:
ret          ; 1 cycles

; void countdown_32(uint16_t count_hi, uint16_t count_lo)
; Will decrement 32 bit counter until it reaches 0.
; cycles = count * 5
; 1 second with 1Mhz fCPU = 1000000 = (200000) 
; r24:r25 - cmp_hi 
; r22:r23 - cmp_lo 
.globl countdown_32 
countdown_32:
; 32bit word = r25:r24:r23:r22
1:
  subi  r22, 1
  sbci  r23, 0
  sbci  r24, 0
  sbci  r25, 0
  brne  1b
ret

.globl wait_usec
wait_usec:

