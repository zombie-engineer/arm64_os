.include "atmega8.inc"
.include "asm_macros.h"
.text

.globl timer_sample_normal_mode
timer_sample_normal_mode:
// Set PB1 mode to output 
sbi DDRB, PB1
cbi PORTB, PB1
// Set TCCNT to 0000
out16 TCNT1, 0x0000
// Set normal mode, prescaler 1/8, this will enable count
ldi r16, 0
out TCCR1A, r16
ldi r16, (1<<TCCR1B_CS11) | (1<<TCCR1B_CS10)
out TCCR1B, r16
// Set interrupt capture to timer1 overflow
ldi r16, (1<<TIMSK_TOIE1)
out TIMSK, r16
sei
1: rjmp 1b

.globl timer_sample_fast_pwm
timer_sample_fast_pwm:
// Set PB1 mode to output 
sbi DDRB, PB1
cbi PORTB, PB1
// Set TCCNT to 0000
out16 TCNT1, 0x0000
// Set Fast PWM mode 15
ldi r16, (1<<TCCR1A_COM1A0)|(1<<TCCR1A_WGM11)|(1<<TCCR1A_WGM10)
out TCCR1A, r16
ldi r16, (1<<TCCR1B_CS11)|(1<<TCCR1B_CS10)|(1<<TCCR1B_WGM13)|(1<<TCCR1B_WGM12)
out TCCR1B, r16
// Set OCR1A to 0xffff to act as TOP
out16 OCR1A, 0x3fff
// Set interrupt capture to timer1 overflow
ldi r16, (1<<TIMSK_TOIE1)
out TIMSK, r16
sei
1: rjmp 1b
