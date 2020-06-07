/*
 * Macros below rely heavily on cache line width, assuming it is 64 bytes.
 * Currently I don't have any good ideas of how this could be solved.
 */

.equ CACHE_LINE_WIDTH_LOG, 6
.equ CACHE_LINE_WIDTH, 64
.macro PERCPU_DATA
.align CACHE_LINE_WIDTH_LOG
GLOBAL_VAR(__percpu_data):
.rept NUM_CORES
/*
 * Stack pointer EL0
 */
.align CACHE_LINE_WIDTH_LOG
.quad 0

/*
 * Stack pointer EL1
 */
.quad 0

/*
 * Jump instruction address
 */
.quad 0

/*
 * Value of MPIDR_EL1 register
 */
.quad 0

/*
 * Address to saved CPU state
 */
.quad 0

/*
 * Padding to 64 bytes ( cache line width )
 */
// .rept (CACHE_LINE_WIDTH / 8 - 5)
// .quad 0
// .endr /* 64 bytes padding */
.endr /* NUM_CORES */
.endm

.equ PERCPU_OFFSET_STACK_EL0, 0
.equ PERCPU_OFFSET_STACK_EL1, 8
.equ PERCPU_OFFSET_JUMP_ADDR, 16
.equ PERCPU_OFFSET_MPIDR_EL1, 24
.equ PERCPU_OFFSET_CPU_STATE, 32

.macro get_percpu_data out, cpu_num
  ldr \out, =__percpu_data
  add \out, \out, \cpu_num, lsl #CACHE_LINE_WIDTH_LOG
.endm

.macro percpu_data_set_stack cpu, el, val, tmp
  get_percpu_data \tmp, \cpu
  str \val, [\tmp, #PERCPU_OFFSET_STACK_EL\()\el]
.endm

.macro percpu_data_get_stack cpu, el, out, tmp
  get_percpu_data \tmp, \cpu
  ldr \out, [\tmp, PERCPU_OFFSET_STACK_EL\()\el]
.endm

.macro percpu_data_set_jmp cpu, val, tmp
  get_percpu_data \tmp, \cpu
  str \val, [\tmp, PERCPU_OFFSET_JUMP_ADDR]
.endm

.macro percpu_data_get_jmp cpu, out, tmp
  get_percpu_data \tmp, \cpu
  ldr \out, [\tmp, PERCPU_OFFSET_JUMP_ADDR]
.endm

.macro percpu_data_set_mpidr cpu, val, tmp
  get_percpu_data \tmp, \cpu
  str \val, [\tmp, PERCPU_OFFSET_MPIDR_EL1]
.endm

.macro percpu_data_get_mpidr cpu, out, tmp
  get_percpu_data \tmp, \cpu
  ldr \out, [\tmp, PERCPU_OFFSET_MPIDR_EL1]
.endm

.macro percpu_data_set_cpu_state cpu, val, tmp
  get_percpu_data \tmp, \cpu
  str \val, [\tmp, PERCPU_OFFSET_CPU_STATE]
.endm

.macro percpu_data_get_cpu_state cpu, out, tmp
  get_percpu_data \tmp, \cpu
  ldr \out, [\tmp, PERCPU_OFFSET_CPU_STATE]
.endm

.macro percpu_init cpu, stack_el0, stack_el1, jump_addr, mpidr_el1, cpu_state, tmp
  get_percpu_data \tmp, \cpu
  str \stack_el0, [\tmp, PERCPU_OFFSET_STACK_EL0]
  str \stack_el1, [\tmp, PERCPU_OFFSET_STACK_EL1]
  str \jump_addr, [\tmp, PERCPU_OFFSET_JUMP_ADDR]
  str \mpidr_el1, [\tmp, PERCPU_OFFSET_MPIDR_EL1]
  str \cpu_state, [\tmp, PERCPU_OFFSET_CPU_STATE]
.endm
