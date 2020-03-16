.include "atmega8.inc"
.include "asm_macros.h"
.text

.equ TWCR, 0x36
.equ TWCR_TWIE  , 0
.equ TWCR_TWEN  , 2
.equ TWCR_TWWC  , 3
.equ TWCR_TWSTO , 4
.equ TWCR_TWSTA , 5
.equ TWCR_TWEA  , 6
.equ TWCR_TWINT , 7

ERROR:
rjmp ERROR

TWI_ERROR:
rjmp ERROR

.equ TWI_START, (1<<TWCR_TWINT) | (1<<TWCR_TWSTA) | (1<<TWCR_TWEN)
.equ TWI_SLA_W, 0x66 | 1
.equ TWI_STATUS_MT_START    , 0x08
.equ TWI_STATUS_MT_REP_START, 0x10
.equ TWI_STATUS_MT_SLAW_ACK , 0x18
.equ TWI_STATUS_MT_SLAW_NACK, 0x20
.equ TWI_STATUS_MT_DATA_ACK , 0x28
.equ TWI_STATUS_MT_DATA_NACK, 0x30
.equ TWI_STATUS_MT_ARBR_LOST, 0x38



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

.globl twi_master_send
; r16 - byte to send
twi_master_send:
TWI_SEND_START
TWI_WAIT_TWINT
TWI_ASSERT_STATUS TWI_STATUS_MT_START

; Load write command to data register
ldi  r16, TWI_SLA_W
out  TWDR, r16
; Clear TWINT and set enable
ldi  r16, (1<<TWCR_TWINT)|(1<<TWCR_TWEN)
out  TWCR, r16
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


.globl sample_twi_0
sample_twi_0:
rcall twi_master_send
