#include <asm_macros.h>
#include <arch/armv8/armv8_asm_macros.h>


FUNC(get_cpu_counter_64_freq):
  get_arm_timer_freq x0
  ret


FUNC(read_cpu_counter_64):
  mrs x0, cntpct_el0
  ret
