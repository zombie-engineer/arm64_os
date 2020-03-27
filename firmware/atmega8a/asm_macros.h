.macro set16 value reg_low reg_high
ldi   \reg_high, (\value >> 8) & 0xff
ldi   \reg_low, \value & 0xff
.endm

.macro out16 reg value
set16 0xffff, r16, r17
out \reg\()H, r17
out \reg\()L, r16
.endm

.macro in16 sreg dreg_low dreg_high
in \dreg_low, \sreg\()L 
in \dreg_high, \sreg\()H
.endm

.macro do_countdown_32 byte0 byte1 byte2 byte3
do_countdown_32_\@:
  subi  \byte0, 1
  sbci  \byte1, 0
  sbci  \byte2, 0
  sbci  \byte3, 0
  brne  do_countdown_32_\@
.endm

.equ MSEC_PER_SEC, 1000
.equ USEC_PER_MSEC, 1000
.equ COUNTDOWN_32_CYCLES_PER_COUNT, 5
.equ FCPU, 1000000
.equ COUNTDOWN_32_COUNTS_PER_SEC, FCPU / COUNTDOWN_32_CYCLES_PER_COUNT
.equ COUNTDOWN_32_COUNTS_PER_MSEC, COUNTDOWN_32_COUNTS_PER_SEC / MSEC_PER_SEC
.equ COUNTDOWN_32_USEC_PER_COUNT, USEC_PER_MSEC / COUNTDOWN_32_COUNTS_PER_MSEC

.macro wait_usec usec
  ldi r22, (\usec / COUNTDOWN_32_USEC_PER_COUNT) & 0xff
  ldi r23, ((\usec / COUNTDOWN_32_USEC_PER_COUNT) >> 8) & 0xff
  clr r24
  clr r25
  do_countdown_32 r22, r23, r24, r25
.endm

.macro wait_msec msec
  ldi r22, (\msec * COUNTDOWN_32_COUNTS_PER_MSEC) >> 0
  ldi r23, (\msec * COUNTDOWN_32_COUNTS_PER_MSEC) >> 8
  ldi r24, (\msec * COUNTDOWN_32_COUNTS_PER_MSEC) >> 16
  ldi r25, (\msec * COUNTDOWN_32_COUNTS_PER_MSEC) >> 24
  do_countdown_32 r22, r23, r24, r25
.endm
