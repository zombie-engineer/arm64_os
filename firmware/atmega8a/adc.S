.include "atmega8.inc"
.include "asm_macros.h"
.text

.globl adc_init_free_int
adc_init_free_int:
  cbi DDRC, PC0
  ldi r16, 0b01000000
  out ADMUX, r16
  ldi r16, 0b11001011;1<<ADCSRA_ADIE|1<<ADCSRA_ADFR|1<<ADCSRA_ADEN
  out ADCSRA, r16
  ret

.globl sample_adc_0
sample_adc_0:
sbi DDRD, PD4
sbi DDRD, PD5
sbi DDRD, PD6
sbi DDRD, PD7
cbi PORTD, PD4
cbi PORTD, PD5
cbi PORTD, PD6
cbi PORTD, PD7
// Set prescaler division to 32
// Not free running
// 10 bits resolution
ldi r16, (1<<ADCSRA_ADEN)|(1<<ADCSRA_ADPS2)|(1<<ADCSRA_ADPS0)
out ADCSRA, r16
// Select PIN5 for ADC input
ldi r16, 0b01000101
out ADMUX, r16

set16 64, r26, r27
set16 768, r28, r29

main_loop:
detect_x:
ldi r16, 0b01000101
out ADMUX, r16
sbi ADCSRA, ADCSRA_ADSC

wait_adc_complete:
sbic  ADCSRA, ADCSRA_ADSC
rjmp  wait_adc_complete
in16  ADC, r16, r17
cp    r16, r26
cpc   r17, r27
brlt  less_x
cp    r16, r28
cpc   r17, r29
brge  greater_x
cbi   PORTD, PD6
cbi   PORTD, PD7
rjmp  detect_y

less_x:
cbi   PORTD, PD6
sbi   PORTD, PD7
rjmp  detect_y

greater_x:
sbi   PORTD, PD6
cbi   PORTD, PD7

detect_y:
ldi r16, 0b01000100
out ADMUX, r16
sbi ADCSRA, ADCSRA_ADSC

wait_adc_complete_y:
sbic  ADCSRA, ADCSRA_ADSC
rjmp  wait_adc_complete_y
in16  ADC, r16, r17
cp    r16, r26
cpc   r17, r27
brlt  less_y
cp    r16, r28
cpc   r17, r29
brge  greater_y
cbi   PORTD, PD4
cbi   PORTD, PD5
rjmp  main_loop

greater_y:
sbi   PORTD, PD4
cbi   PORTD, PD5
rjmp  detect_y

less_y:
cbi   PORTD, PD4
sbi   PORTD, PD5
rjmp  main_loop

