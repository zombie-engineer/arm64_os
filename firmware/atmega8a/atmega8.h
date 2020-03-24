#pragma once

#define RAMEND 0x045f
#define TWBR   0x00
#define TWSR   0x01
#define TWAR   0x02
#define TWDR   0x03
#define ADCL   0x04
#define ADCH   0x05
#define ADCSRA 0x06
#define ADMUX  0x07
#define ACSR   0x08
#define UBRRL  0x09
#define UCSRB  0x0a
#define UCSRA  0x0b
#define UDR    0x0c
#define PIND   0x10
#define DDRD   0x11
#define PORTD  0x12
#define PINC   0x13
#define DDRC   0x14
#define PORTC  0x15
#define PINB   0x16
#define DDRB   0x17
#define PORTB  0x18
#define UBRRH  0x20
#define UCSRC  0x20
#define ICR1L  0x26
#define ICR1H  0x27
#define OCR1BL 0x28
#define OCR1BH 0x29
#define OCR1AL 0x2a
#define OCR1AH 0x2b
#define TCNT1L 0x2c
#define TCNT1H 0x2d
#define TCCR1B 0x2e
#define TCCR1A 0x2f
#define SFIOR  0x30
#define OSCCAL 0x31
#define TCNT0  0x32
#define TCCR0  0x33
#define MCUCR  0x35
#define TWCR   0x36
#define TIFR   0x38
#define TIMSK  0x39
#define GIFR   0x3a
#define GICR   0x3b
#define SPL    0x3d
#define SPH    0x3e
#define SREG   0x3f

#define UCSRB_TXB8  0
#define UCSRB_RXB8  1
#define UCSRB_UCSZ2 2
#define UCSRB_TXEN  3
#define UCSRB_RXEN  4

#define UCSRC_UCPOL 0
#define UCSRC_UCSZ0 1
#define UCSRC_UCSZ1 2
#define UCSRC_USBS  3
#define UCSRC_UPM0  4
#define UCSRC_UPM1  5
#define UCSRC_UMSEL 6
#define UCSRC_URSEL 7

#define UCSRA_MPCM 0
#define UCSRA_U2X  1
#define UCSRA_PE   2
#define UCSRA_DOR  3
#define UCSRA_FE   4
#define UCSRA_UDRE 5
#define UCSRA_TXC  6
#define UCSRA_RXC  7


#define TCCR0_CS00  0
#define TCCR0_CS01  1
#define TCCR0_CS02  2

#define TCCR1B_CS10  0
#define TCCR1B_CS11  1
#define TCCR1B_CS12  2
#define TCCR1B_WGM12 3
#define TCCR1B_WGM13 4
#define TCCR1B_ICES1 6
#define TCCR1B_ICES2 7
#define TCCR1B_PRESCALE_1024 0b00000101

#define TCCR0_NO_CLOCK      0
#define TCCR0_NO_PRESCALE   1
#define TCCR0_PRESCALE_8    2
#define TCCR0_PRESCALE_64   3
#define TCCR0_PRESCALE_256  4
#define TCCR0_PRESCALE_1024 5
#define TCCR0_EXT_SRC_FALL  6
#define TCCR0_EXT_SRC_RISE  7

#define TIMSK_TOIE0  0
#define TIMSK_TOIE1  2
#define TIMSK_OCIE1B 3
#define TIMSK_OCIE1A 4
#define TIMSK_TICIE1 5
#define TIMSK_TOIE2  6
#define TIMSK_OCIE2  7

#define TIFR_TOV0   0
#define TIFR_TOV1   2

#define PB0 0
#define PB1 1
#define PB2 2
#define PB3 3
#define PB4 4
#define PB5 5
#define PB6 6
#define PB7 7

#define PC0 0
#define PC1 1
#define PC2 2
#define PC3 3
#define PC4 4
#define PC5 5
#define PC6 6
#define PC7 7

#define PD0 0
#define PD1 1
#define PD2 2
#define PD3 3
#define PD4 4
#define PD5 5
#define PD6 6
#define PD7 7

#define INT0 6
#define INT1 7

#define ISC00 0
#define ISC01 1
#define ISC10 2
#define ISC11 3

#define TCCR1A_COM1A1 7
#define TCCR1A_COM1A0 6
#define TCCR1A_COM1B1 5
#define TCCR1A_COM1B0 4
#define TCCR1A_FOC1A  3
#define TCCR1A_FOC1B  2
#define TCCR1A_WGM11  1
#define TCCR1A_WGM10  0

#define ADCSRA_ADPS0  0
#define ADCSRA_ADPS1  1
#define ADCSRA_ADPS2  2
#define ADCSRA_ADIE   3
#define ADCSRA_ADIF   4
#define ADCSRA_ADFR   5
#define ADCSRA_ADSC   6
#define ADCSRA_ADEN   7

#define TWCR_TWIE   0
#define TWCR_TWEN   2
#define TWCR_TWWC   3
#define TWCR_TWSTO  4
#define TWCR_TWSTA  5
#define TWCR_TWEA   6
#define TWCR_TWINT  7

#define xh r27
#define xl r26
#define yh r29
#define yl r28
#define zh r31
#define zl r30
