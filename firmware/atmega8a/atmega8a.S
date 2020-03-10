.equ RAMEND, 0x045f
.equ UBRRL , 0x09
.equ UCSRB , 0x0a
.equ UCSRA , 0x0b
.equ UDR   , 0x0c
.equ PIND  , 0x10
.equ DDRD  , 0x11
.equ PORTD , 0x12
.equ PINB  , 0x16
.equ DDRB  , 0x17
.equ PORTB , 0x18
.equ UBRRH , 0x20
.equ UCSRC , 0x20
.equ TCNT0 , 0x32
.equ TCCR0 , 0x33
.equ MCUCR , 0x35
.equ TIFR  , 0x38
.equ TIMSK , 0x39
.equ GICR  , 0x3b
.equ SPL   , 0x3d
.equ SPH   , 0x3e
.equ SREG  , 0x3f

.equ UCSRB_TXB8 , 0
.equ UCSRB_RXB8 , 1
.equ UCSRB_UCSZ2, 2
.equ UCSRB_TXEN , 3
.equ UCSRB_RXEN , 4

.equ UCSRC_UCPOL, 0
.equ UCSRC_UCSZ0, 1
.equ UCSRC_UCSZ1, 2
.equ UCSRC_USBS , 3
.equ UCSRC_UPM0 , 4
.equ UCSRC_UPM1 , 5
.equ UCSRC_UMSEL, 6
.equ UCSRC_URSEL, 7

.equ UCSRA_MPCM, 0
.equ UCSRA_U2X , 1
.equ UCSRA_PE  , 2
.equ UCSRA_DOR , 3
.equ UCSRA_FE  , 4
.equ UCSRA_UDRE, 5
.equ UCSRA_TXC , 6
.equ UCSRA_RXC , 7


.equ TCCR0_CS00 , 0
.equ TCCR0_CS01 , 1
.equ TCCR0_CS02 , 2

.equ TCCR0_NO_CLOCK     , 0
.equ TCCR0_NO_PRESCALE  , 1
.equ TCCR0_PRESCALE_8   , 2
.equ TCCR0_PRESCALE_64  , 3
.equ TCCR0_PRESCALE_256 , 4
.equ TCCR0_PRESCALE_1024, 5
.equ TCCR0_EXT_SRC_FALL , 6
.equ TCCR0_EXT_SRC_RISE , 7

.equ TIMSK_TOIE0 , 0
.equ TIMSK_TOIE1 , 2
.equ TIMSK_OCIE1B, 3
.equ TIMSK_OCIE1A, 4
.equ TIMSK_TICIE1, 5
.equ TIMSK_TOIE2 , 6
.equ TIMSK_OCIE2 , 7

.equ TIFR_TOV0  , 0
.equ TIFR_TOV1  , 2

.equ PB0, 0
.equ PB1, 1
.equ PB2, 2
.equ PB3, 3
.equ PB4, 4
.equ PB5, 5
.equ PB6, 6
.equ PB7, 7

.equ PC0, 0
.equ PC1, 1
.equ PC2, 2
.equ PC3, 3
.equ PC4, 4
.equ PC5, 5
.equ PC6, 6
.equ PC7, 7

.equ PD0, 0
.equ PD1, 1
.equ PD2, 2
.equ PD3, 3
.equ PD4, 4
.equ PD5, 5
.equ PD6, 6
.equ PD7, 7

.equ INT0, 6
.equ INT1, 7

.equ ISC00, 0
.equ ISC01, 1
.equ ISC10, 2
.equ ISC11, 3


.equ BAUD_RATE, 1000000 / (16 * 115200) - 1

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

usart_transmit:
// Wait for empty transmit buffer
sbis   UCSRA, UCSRA_UDRE
rjmp   usart_transmit
out    UDR, r16
ret

usart_recieve:
sbis  UCSRA, UCSRA_RXC
rjmp  usart_recieve
in    r16, UDR
ret


usart_initialize:
// Set baud rate
ldi   r16, 0x19
ldi   r17, 0x00
out   UBRRH, r17
out   UBRRL, r16
// Only Rx enable
// UCSZ2 = 0 (8 bits char)
ldi  r16, (1<<UCSRB_TXEN) | (1<<UCSRB_RXEN)
out  UCSRB, r16
// Set frame format
// UMSEL = 0 (asynchronous mode)
// UPM0,UPM1 = 0 (Parity off)
// USBS = 0 (1 Stop Bits)
// UCSZ0,UCSZ1 = 1,1 = (8 bits char)
ldi  r16, (1<<UCSRC_URSEL) | (1<<UCSRC_UCSZ0) | (1<<UCSRC_UCSZ1)
out  UCSRC, r16
ret

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
