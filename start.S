.section ".text.boot"
.global _start

_start:
0:
  wfe
  b 0b
