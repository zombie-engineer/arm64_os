.include "atmega8.inc"
.include "asm_macros.h"
.text

.globl SPI_SlaveInit
SPI_SlaveInit:
  ; Set MISO output, MOSI, SCLK, SS input
  ldi r17, (1<<PB4)
  out DDRB, r17
  ; Enable SPI
  ldi r17, ((1<<SPCR_SPE)|(1<<SPCR_SPIE))
  out SPCR, r17
  ldi r17, 0x00 
  out SPDR, r17
  ret

.globl SPI_SlaveReceive
SPI_SlaveReceive:
; Wait for reception complete
sbis SPSR, SPSR_SPIF
rjmp SPI_SlaveReceive
cbi PORTD, PD7 
1:
  rjmp 1b


;.globl __vector_10
;__vector_10:
;  ; Store r16, store SREG
;  push r16
;  in   r16, SREG
;  push r16
;
;  in r16, SPDR
;  cpi r16, ATMCMD_CMD_READ_ADC
;  brne
;  
;  ; Restore SREG, restore r16
;  pop  r16
;  out  SREG, r16
;  pop  r16
;  ; Exit
;  reti
