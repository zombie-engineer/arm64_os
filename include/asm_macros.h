#define FUNC(name) \
    .globl name; name

#define LFUNC(name) \
    .local name; name

#define GLOBAL_VAR(name) \
    .globl name; name

.macro reg_set_addr32 reg addr32
  mov \reg, #(\addr32 & 0xffff)
  movk \reg, #(\addr32 >> 16), lsl #16
.endm

