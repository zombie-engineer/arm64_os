.equ PSTATE_DAIF_OFFSET, 6
.equ DAIF_IRQ_OFFSET, 1
.equ DAIF_IRQ_BIT, 1 << DAIF_IRQ_OFFSET

.macro IRQ_DISABLE
  msr daifset, #DAIF_IRQ_BIT
.endm

.macro IRQ_ENABLE
  msr daifclr, #DAIF_IRQ_BIT
.endm

.macro IRQ_ENABLED tmp
  mrs \tmp, daif
  lsr \tmp, \tmp, #(PSTATE_DAIF_OFFSET + DAIF_IRQ_OFFSET)
  mvn \tmp, \tmp
  and \tmp, \tmp, #1
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

.macro get_arm_timer_freq dest
  mrs \dest, cntfrq_el0
  ubfx \dest, \dest, #0, #32
.endm
