
.macro	str_l, src, sym, tmp
adrp	\tmp, \sym
str	\src, [\tmp, :lo12:\sym]
.endm

.globl _start
_start:
  str_l x4, idmap_ptrs_per_pgd, x5


