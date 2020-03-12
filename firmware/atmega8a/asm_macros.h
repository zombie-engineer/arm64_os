.macro out16 reg value
ldi   r17, (\value >> 8) & 0xff
ldi   r16, \value & 0xff
out \reg\()H, r17
out \reg\()L, r16
.endm

