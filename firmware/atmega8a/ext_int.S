.include "atmega8.inc"
.include "asm_macros.h"
.text

.globl sample_ext_int0
sample_ext_int0:
//ldi r16, 0
//out DDRB, r16
//out PORTB, r16
//
//out DDRD, r16
//out PORTD, r16
//
//out PORTC, r16
//out DDRC, r16
//
//out TIMSK, r16
//
//out SFIOR, r16

// Set PD0 mode to output value = 0
sbi DDRD, PD0
cbi PORTD, PD0
sbi DDRD, PD1
sbi PORTD, PD1
// Load MCUCR register to interrupt at any logical change
ldi r16, 0b00000000
out MCUCR, r16
ldi r16, 0b01000000
out GICR, r16
out GIFR, r16
// enable interrupts
sei
// Loop and enjoy
1: rjmp 1b
