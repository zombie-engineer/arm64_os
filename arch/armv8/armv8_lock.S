#include <asm_macros.h>

.section ".text"

#define LOCKED 1
#define UNLOCKED 0

FUNC(__armv8_spinlock_init):
  str xzr, [x0]
  ret

FUNC(__armv8_spinlock_lock):
  mov   x1, #LOCKED
  # Mark address to exclusive monitor
  ldxr  x2, [x0]
  # If not zero (LOCKED by other), goto wait
  cbnz  x2, 1f
  # If zero (not LOCKED), try to set LOCKED
  # state to that address
  stxr  w3, x1, [x0]
 //   # stxr sets store result to w3
 //   # if w3 == 0, store completed with success.
 //   # if w3 == 1, store failed. This can happen 
 //   # if for example we context switched right 
 //   # before the store operation. This effectively
 //   # cleared exclusive state and some other process
 //   # used ldxr to reown the access.
 //   # in this case we go to sleep until next time 
  cbnz  w3, 1f
 //   # if w3 is 0 - we are ready to enter critical section
 //   # barrier is to order all memory accesses in critical
 //   # section after LOCK was stored
  dmb   sy
  ret
1:  
  # sleep
  wfe
  b __armv8_spinlock_lock
  
FUNC(__armv8_spinlock_unlock):
  str   xzr, [x0]
  # After we stored unlocked state to 
  # the address we put memory synchronization 
  # barrier to make sure SEV is executed
  # AFTER store operation.
  dsb   st
  sev
  ret
  
