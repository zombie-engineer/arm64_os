.equ RAMEND, 0x045f
.equ TWBR  , 0x00
.equ TWSR  , 0x01
.equ TWAR  , 0x02
.equ TWDR  , 0x03
.equ ADCL  , 0x04
.equ ADCH  , 0x05
.equ ADCSRA, 0x06
.equ ADMUX , 0x07
.equ ACSR  , 0x08
.equ UBRRL , 0x09
.equ UCSRB , 0x0a
.equ UCSRA , 0x0b
.equ UDR   , 0x0c
.equ SPCR  , 0x0d
.equ SPSR  , 0x0e
.equ SPDR  , 0x0f
.equ PIND  , 0x10
.equ DDRD  , 0x11
.equ PORTD , 0x12
.equ PINC  , 0x13
.equ DDRC  , 0x14
.equ PORTC , 0x15
.equ PINB  , 0x16
.equ DDRB  , 0x17
.equ PORTB , 0x18
.equ UBRRH , 0x20
.equ UCSRC , 0x20
.equ ICR1L , 0x26
.equ ICR1H , 0x27
.equ OCR1BL, 0x28
.equ OCR1BH, 0x29
.equ OCR1AL, 0x2a
.equ OCR1AH, 0x2b
.equ TCNT1L, 0x2c
.equ TCNT1H, 0x2d
.equ TCCR1B, 0x2e
.equ TCCR1A, 0x2f
.equ SFIOR , 0x30
.equ OSCCAL, 0x31
.equ TCNT0 , 0x32
.equ TCCR0 , 0x33
.equ MCUCR , 0x35
.equ TWCR  , 0x36
.equ TIFR  , 0x38
.equ TIMSK , 0x39
.equ GIFR  , 0x3a
.equ GICR  , 0x3b
.equ SPL   , 0x3d
.equ SPH   , 0x3e
.equ SREG  , 0x3f

.equ SPCR_SPR0  , 0
.equ SPCR_SPR1  , 1
.equ SPCR_CPHA  , 2
.equ SPCR_CPOL  , 3
.equ SPCR_MSTR  , 4
.equ SPCR_DORD  , 5
.equ SPCR_SPE   , 6
.equ SPCR_SPIE  , 7

.equ SPSR_SPI2X , 0
.equ SPSR_WCOL  , 6
.equ SPSR_SPIF  , 7

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

.equ TCCR1B_CS10 , 0
.equ TCCR1B_CS11 , 1
.equ TCCR1B_CS12 , 2
.equ TCCR1B_WGM12, 3
.equ TCCR1B_WGM13, 4
.equ TCCR1B_ICES1, 6
.equ TCCR1B_ICES2, 7
.equ TCCR1B_PRESCALE_1024, 0b00000101

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

.equ TCCR1A_COM1A1, 7
.equ TCCR1A_COM1A0, 6
.equ TCCR1A_COM1B1, 5
.equ TCCR1A_COM1B0, 4
.equ TCCR1A_FOC1A , 3
.equ TCCR1A_FOC1B , 2
.equ TCCR1A_WGM11 , 1
.equ TCCR1A_WGM10 , 0

.equ ADCSRA_ADPS0 , 0
.equ ADCSRA_ADPS1 , 1
.equ ADCSRA_ADPS2 , 2
.equ ADCSRA_ADIE  , 3
.equ ADCSRA_ADIF  , 4
.equ ADCSRA_ADFR  , 5
.equ ADCSRA_ADSC  , 6
.equ ADCSRA_ADEN  , 7

.equ TWCR_TWIE  , 0
.equ TWCR_TWEN  , 2
.equ TWCR_TWWC  , 3
.equ TWCR_TWSTO , 4
.equ TWCR_TWSTA , 5
.equ TWCR_TWEA  , 6
.equ TWCR_TWINT , 7

.equ xh, r27
.equ xl, r26
.equ yh, r29
.equ yl, r28
.equ zh, r31
.equ zl, r30
