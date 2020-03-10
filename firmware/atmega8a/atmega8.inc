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

