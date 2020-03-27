.include "atmega8.inc"
.include "asm_macros.h"
.text

twi_set_bit_rate:
; BIT RATE
; SCL frequency = CPU Clock frequency / (16 + 2(TWBR) * 4**TWPS)
;               = for 1Mhz CPU freq 
;               = 1000000 / (16 + 2*0xff * (4**4)) is minimal freq
; TWPS value 4 is TWSC TWPS1:TWPS0 with values 0b10

ldi  r16, 0xff
out  TWBR, r16
ldi  r16, 0b10
out  TWSR, r16
ret

ERROR:
sbi DDRB, PB0
sbi DDRD, PD6
sbi DDRD, PD7
cbi PORTB, PB0
cbi PORTD, PD6
cbi PORTD, PD7
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
cbi DDRB, PB0
cbi PORTB, PB0
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
rjmp __int_reset

TWI_ERROR:
rjmp ERROR

.equ TWI_START, (1<<TWCR_TWINT) | (1<<TWCR_TWSTA) | (1<<TWCR_TWEN)
.equ TWI_SLA_W, 0xd4
.equ TWI_STATUS_MT_START    , 0x08
.equ TWI_STATUS_MT_REP_START, 0x10
.equ TWI_STATUS_MT_SLAW_ACK , 0x18
.equ TWI_STATUS_MT_SLAW_NACK, 0x20
.equ TWI_STATUS_MT_DATA_ACK , 0x28
.equ TWI_STATUS_MT_DATA_NACK, 0x30
.equ TWI_STATUS_MT_ARBR_LOST, 0x38

.equ TWI_STATUS_SR_SLAW_ACK,  0x60
.equ TWI_STATUS_SR_ARBR_LOST, 0x68
.equ TWI_STATUS_SR_GEN_ACK  , 0x70

.macro TWI_SEND_START
ldi   r16, TWI_START
out   TWCR, r16
.endm

.macro TWI_SEND_STOP
; Send stop
ldi  r16, (1<<TWCR_TWINT)|(1<<TWCR_TWEN)|(1<<TWCR_TWSTO)
out  TWCR, r16
.endm

.macro TWI_WAIT_TWINT
wait_twint\@: 
in r16, TWCR
sbrs r16, TWCR_TWINT
rjmp wait_twint\@
.endm

.macro TWI_ASSERT_STATUS status_value 
; Check status
in   r16, TWSR
; First 2 bits in TWSR
; are prescaler bits, so we discard them.
andi r16, 0xf8
cpi  r16, \status_value
brne TWI_ERROR
.endm

.globl twi_master_start
twi_master_start:
ldi   r16, (1<<TWCR_TWINT) | (1<<TWCR_TWSTA) | (1<<TWCR_TWEN)
out   TWCR, r16
ret

.globl twi_master_send
; r16 - byte to send
twi_master_send:
TWI_SEND_START

1: ; Wait for start SENT
sbi   DDRB, PB0
sbi   PORTB, PB0
in    r16, TWCR
sbrs  r16, TWCR_TWINT
rjmp  1b
sbi   DDRD, PD7
sbi   PORTD, PD7

TWI_ASSERT_STATUS TWI_STATUS_MT_START

sbi   DDRD, PD6
sbi   PORTD, PD6

; Load write command to data register
ldi  r16, TWI_SLA_W
out  TWDR, r16
; Clear TWINT and set enable
ldi  r16, (1<<TWCR_TWINT)|(1<<TWCR_TWEN)
out  TWCR, r16
sbi   PORTB, PB0
1: rjmp 1b
TWI_WAIT_TWINT
TWI_ASSERT_STATUS TWI_STATUS_MT_SLAW_ACK 

; Set data
ldi  r16, TWI_SLA_W
out  TWDR, r16
ldi  r16, (1<<TWCR_TWINT)|(1<<TWCR_TWEN)
out  TWCR, r16
TWI_WAIT_TWINT
TWI_ASSERT_STATUS TWI_STATUS_MT_DATA_ACK
TWI_SEND_STOP
1: rjmp 1b
ret

.globl twi_master_init
twi_master_init:
ldi  r16, 1<<TWCR_TWEN
out  TWCR, r16
rcall twi_set_bit_rate
ret

.globl twi_master_deinit
twi_master_deinit:
clr r16
out TWCR, r16
ret

; I2C Master
; This hasn't proved to be working
.globl sample_twi_0
sample_twi_0:
rcall twi_master_init
rcall twi_master_send

; I2C Slave
.globl sample_twi_1
sample_twi_1:
ldi  r16, (0x1e << 1)
out  TWAR, r16
ldi  r16, 0
out  TWDR, 0x00
ldi  r16, (1<<TWCR_TWINT)|(1<<TWCR_TWEA)|(1<<TWCR_TWIE)
out  TWCR, r16
sbi  DDRB, PB0
sbi  PORTB, PB0
1:
in r16, TWCR
sbrs r16, TWCR_TWINT
rjmp 1b

sbi  DDRD, PD7
sbi  PORTD, PD7

in   r16, TWSR
; First 2 bits in TWSR
; are prescaler bits, so we discard them.
andi r16, 0xf8
cpi  r16, TWI_STATUS_SR_SLAW_ACK
brne 2f
sbi  DDRD, PD6
sbi  PORTD, PD6
1: rjmp 1b
2: rjmp TWI_ERROR
