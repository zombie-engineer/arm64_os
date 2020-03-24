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

; void delay_cycles(uint16_t num_cycles)
; r24:r25 - num_cycles
.globl delay_cycles
delay_cycles:
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
