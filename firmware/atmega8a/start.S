.include "atmega8.inc"

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

main:

rcall usart_initialize
uart_loop:
rcall usart_recieve
rcall usart_transmit
ldi   r16, '-'
rcall usart_transmit
ldi   r16, '$'
rcall usart_transmit
rjmp  uart_loop

# Set PD0 as output pin with logical 0
sbi   DDRD, PD0
sbi   PORTD, PD0
# Set PD2 to recieve logic 0 signal
cbi   DDRD, PD2
cbi   PORTD, PD2
ldi   r16, (1<<ISC00) + (1<<ISC01)
out   MCUCR, r16
ldi   r16, 1<<INT0
out   GICR, r16

# Put toggle var
ldi   r16, 1
ldi   r27, 0
ldi   r26, 0x60
st    X, r16

rcall timer0_enable_overflow_interrupt
sei
# Set PD1 as output pin with logical 0
sbi   DDRD, PD1
sbi   PORTD, PD1
1:
rjmp 1b


cbi   PORTD, 0
cbi   PORTD, 1
cbi   PORTD, 2
cbi   PORTD, 3
cbi   PORTD, 4
cbi   PORTD, 5
cbi   PORTD, 6
cbi   PORTD, 7

sbi   PORTD, 1

# mov   r16, 0
# out   PORTD, r16

nop
nop
nop
nop
nop
nop
nop
nop

1:
rjmp 1b

__halt_on_interrupt:
  rjmp __halt_on_interrupt

__handle_interrupt:
  push r16
  in   r16, SREG
  push r16

  ldi   r27, 0
  ldi   r26, 0x60
  ld    r16, X
  sbrs  r16, 0
  rjmp  1f
  ldi   r16, 0
  cbi   PORTD, PD0
  rjmp 2f
1:
  ldi   r16, 1
  sbi   PORTD, PD0
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
__int_timer1_ovf:
rjmp __halt_on_interrupt
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
