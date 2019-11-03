#include <asm_macros.h>


FUNC(get_system_timer_freq):
  mrs x0, cntfrq_el0 
  ubfx x0, x0, #0, #32
  ret


FUNC(get_system_timer):
  mrs x0, cntpct_el0
  ret
