.equ PORTD, 0x12
.equ DDRD, 0x11
.equ PIND, 0x10
.equ PORTB, 0x18
.equ DDRB, 0x17


rjmp main


.org 0x20
main:
ser   r16
out   DDRD, r16 
clr   r16
sbr   r16, 2
out   PORTD, r16
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
