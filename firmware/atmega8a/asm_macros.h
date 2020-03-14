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
