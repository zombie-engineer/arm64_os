#define DAIF_IRQ_BIT    (1 << 1)

.macro IRQ_DISABLE
  msr daifset, #DAIF_IRQ_BIT
.endm

.macro IRQ_ENABLE
  msr daifclr, #DAIF_IRQ_BIT
.endm

/*
 * Writes 32bit value to a register
 * this way we do not need load operation from literal pool
 */
.macro mov32 value, dest
  mov \dest, #(\value & 0xffff0000)
  movk \dest, #(\value & 0xffff)
.endm

.macro get_cpu_id dest
/*
 * mpidr_el1 & 3 = Aff1 for 4-core systems holds
 * the core number
 */
  mrs   \dest, mpidr_el1
  and   \dest, \dest, #3
.endm

.equ PERCPU_OFFSET_MPIDR         , 0
.equ PERCPU_OFFSET_CURRENT_CPUCTX, 8
.equ PERCPU_OFFSET_STARTUP_FN    , 16
.equ PERCPU_OFFSET_EL1_STACK     , 24

.macro get_percpu_ctx current_ctx, cpu_num
  ldr \current_ctx, =__percpu_context
  add \current_ctx, \current_ctx, \cpu_num, lsl #6
.endm

.macro get_current_percpu_ctx current_ctx, current_cpu
  get_cpu_id \current_cpu
  get_percpu_ctx \current_ctx, \current_cpu
.endm

.macro get_percpu_mpidr cpu_num, tmp, out_mpidr
   get_percpu_ctx \tmp, \cpu_num
   ldr \out_mpidr, [\tmp, PERCPU_OFFSET_MPIDR]
.endm

.macro get_percpu_el1_stack cpu, tmp, out_stack
   get_percpu_ctx \tmp, \cpu_num
   ldr \out_stack, [\tmp, PERCPU_OFFSET_EL1_STACK]
.endm

.macro set_percpu_el1_stack cpu, tmp, stack
   get_percpu_ctx \tmp, \cpu_num
   str \stack, [\tmp, PERCPU_OFFSET_EL1_STACK]
.endm
