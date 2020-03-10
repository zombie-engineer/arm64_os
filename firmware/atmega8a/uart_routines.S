.include "atmega8.inc"
.equ BAUD_RATE, 1000000 / (16 * 115200) - 1

.globl usart_transmit
usart_transmit:
// Wait for empty transmit buffer
sbis   UCSRA, UCSRA_UDRE
rjmp   usart_transmit
out    UDR, r16
ret

.globl usart_recieve
usart_recieve:
sbis  UCSRA, UCSRA_RXC
rjmp  usart_recieve
in    r16, UDR
ret

.globl usart_initialize
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
