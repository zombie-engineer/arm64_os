.include "atmega8.inc"
.include "asm_macros.h"
.text

.globl SPI_SlaveInit
SPI_SlaveInit:
  ; Set MISO output, MOSI, SCLK, SS input
  ldi r17, (1<<PB4)
  out DDRB, r17
  ; Enable SPI
  ldi r17, (1<<SPCR_SPE)
  out SPCR, r17
  ldi r17, 0x66
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


.globl __vector_10
__vector_10:
 cbi PORTB, PB0 
1:
 rjmp 1b
