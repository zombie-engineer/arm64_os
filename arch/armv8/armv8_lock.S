#include <asm_macros.h>

.section ".text"

# LOCKED 1
# UNLOCKED 0
FUNC(__armv8_spinlock_lock):
  # pld   x0
  # prfm	pstl1strm, [x0]
  mov   x1, #1
  ldxr  x2, [x0]
  cbnz  x2, 1f
  stxr  w3, x1, [x0]
  cbnz  w3, 1f
  dsb   ish
  ret
1:  
  wfe
  b __armv8_spinlock_lock
  
FUNC(__armv8_spinlock_unlock):
  mov   x1, #0
  dsb sy
  str   x1, [x0]
  sev
  ret
  

