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
