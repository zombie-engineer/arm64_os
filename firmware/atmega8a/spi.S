.include "atmega8.inc"
.include "asm_macros.h"
.text

.globl spi_init_slave
spi_init_slave:
  ; Set MISO output, MOSI, SCLK, SS input
  ldi r16, (1<<PB4)
  out DDRB, r16
  ; Enable SPI
  ldi r16, ((1<<SPCR_SPE)|(1<<SPCR_SPIE))
  out SPCR, r16
  ldi r16, 0x00 
  out SPDR, r16
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
