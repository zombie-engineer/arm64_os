.include "atmega8.inc"
.include "asm_macros.h"
.text

.globl sample_adc_0
sample_adc_0:
sbi DDRD, PD6
sbi DDRD, PD7
sbi PORTD, PD6
cbi PORTD, PD7
// Set prescaler division to 32
// Not free running
// 10 bits resolution
ldi r16, (1<<ADCSRA_ADEN)|(1<<ADCSRA_ADPS2)|(1<<ADCSRA_ADPS0)
out ADCSRA, r16
// Select PIN5 for ADC input
ldi r16, 0b01000101
out ADMUX, r16

main_loop:
sbi ADCSRA, ADCSRA_ADSC
set16 512, r28, r29

wait_adc_complete:
sbic  ADCSRA, ADCSRA_ADSC
rjmp  wait_adc_complete
in16  ADC, r16, r17
cp    r28, r16
cpc   r29, r17
brlt  less

greater_or_equ:
sbi   PORTD, PD6
cbi   PORTD, PD7
rjmp main_loop

less:
cbi   PORTD, PD6
sbi   PORTD, PD7
rjmp main_loop
