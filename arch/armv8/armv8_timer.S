#include <asm_macros.h>


FUNC(get_cpu_counter_64_freq):
  mrs x0, cntfrq_el0 
  ubfx x0, x0, #0, #32
  ret


FUNC(read_cpu_counter_64):
  mrs x0, cntpct_el0
  ret
