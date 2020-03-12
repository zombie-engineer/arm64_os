.include "atmega8.inc"
.section ".text"

// INTERRUPTS VECTOR
rjmp __int_reset        // RESET
rjmp __int_int0         // INT0 External interrupt 0
rjmp __int_int1         // INT1 External interrupt 1
rjmp __int_timer2_comp  // TIMER2 COMP Timer/Counter2 Compare Match
rjmp __int_timer2_ovf   // TIMER2 OVF Timer/Counter2 Overflow
rjmp __int_timer1_capt  // TIMER1 CAPT Timer/Counter1 Capture Event
rjmp __int_timer1_compa // TIMER1 COMPA Timer/Counter1 Compare Match A
rjmp __int_timer1_compb // TIMER1 COMPB Timer/Counter1 Compare Match B
rjmp __int_timer1_ovf   // TIMER1 OVF Timer/Counter1 Overflow
rjmp __int_timer0_ovf   // TIMER0 OVF Timer/Counter0 Overflow
rjmp __int_spi_stc      // SPI, STC Serial Transfer Complete
rjmp __int_usart_rxc    // USART, RXC USART, Rx Complete
rjmp __int_usart_udre   // USART, UDRE USART, Data Register Empty
rjmp __int_usart_txc    // USART, TXC USART, Tx Complete
rjmp __int_adc          // ADC ADC Conversion Complete
rjmp __int_ee_rdy       // EE_RDY EEPROM Ready
rjmp __int_ana_comp     // ANA_COMP Analog Comparator
rjmp __int_twi          // TWI Two-wire Serial Interface
rjmp __int_spm_rdy      // SPM_RDY Store Program Memory Ready

.section ".text"
timer0_enable_overflow_interrupt:
ldi   r16, TCCR0_PRESCALE_1024
out   TCCR0, r16
ldi   r16, 1<<TIMSK_TOIE0
out   TIMSK, r16
ret

__int_reset:
ldi   r16, RAMEND & 0xff
out   SPL, r16
ldi   r16, (RAMEND >> 8) & 0xff
out   SPH, r16

.globl main
main:
rjmp timer_sample_fast_pwm

# Set PD0 as output pin with logical 0
// sbi   DDRD, PD0
// sbi   PORTD, PD0
// sbi   PORTD, PD1
# Set PD2 to recieve logic 0 signal
// cbi   DDRD, PD2
// cbi   PORTD, PD2
// ldi   r16, (1<<ISC00) + (1<<ISC01)
// out   MCUCR, r16
// ldi   r16, 1<<INT0
// out   GICR, r16

# Set PWM
ldi   r16, 0xff
ldi   r17, 0x1f
out   OCR1AH, r17
out   OCR1AL, r16

sbi   DDRB, PB1
ldi   r16, 0b10000001
// ldi   r16, (1<<TCCR1A_COM1A0) | (1<<TCCR1A_WGM10)
out   TCCR1A, r16

// ldi   r16, (1<<TCCR1B_WGM13) | (1<<TCCR1B_CS10) | (1<<TCCR1B_CS12)
ldi   r16, 0b00010001
out   TCCR1B, r16


# Put toggle var
ldi   r16, 1
ldi   r27, 0
ldi   r26, 0xf0
st    X, r16

// rcall timer0_enable_overflow_interrupt
// sei
# Set PD1 as output pin with logical 0
sbi   DDRD, PD1
sbi   PORTD, PD1
1:
rjmp 1b

__halt_on_interrupt:
  rjmp __halt_on_interrupt

__handle_interrupt:
  push r16
  in   r16, SREG
  push r16

  ldi   r27, 0
  ldi   r26, 0xf0
  ld    r16, X
  sbrs  r16, 0
  rjmp  1f
  ldi   r16, 0
  cbi   PORTB, PB1
  rjmp 2f
1:
  ldi   r16, 1
  sbi   PORTB, PB1
2:
  st   X, r16
  pop  r16
  out  SREG, r16
  pop  r16
  reti

__int_int0:
__int_int1:
__int_timer2_comp:
__int_timer2_ovf:
__int_timer1_capt:
__int_timer1_compa:
__int_timer1_compb:
rjmp __halt_on_interrupt
__int_timer1_ovf:
rjmp __handle_interrupt
__int_timer0_ovf:
rjmp __handle_interrupt
__int_spi_stc:
__int_usart_rxc:
__int_usart_udre:
__int_usart_txc:
__int_adc:
__int_ee_rdy:
__int_ana_comp:
__int_twi:
__int_spm_rdy:
rjmp __halt_on_interrupt
