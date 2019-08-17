#include "asm_macros.h"

#define DAIF_IRQ_BIT    (1 << 1)

.section ".text"

.globl enable_irq
.globl disable_irq
.globl is_irq_enabled

.globl mem_model_max_pa_bits
.globl mem_model_num_asid_bits
.globl mem_model_4k_granule_support
.globl mem_model_16k_granule_support
.globl mem_model_64k_granule_support
.globl mem_model_st2_4k_granule_support
.globl mem_model_st2_16k_granule_support
.globl mem_model_st2_64k_granule_support
.globl disable_l1_caches
.globl disable_l2_caches
.globl disable_l3_caches

enable_irq:
  msr daifclr, #DAIF_IRQ_BIT
  ret

disable_irq:
  msr daifset, #DAIF_IRQ_BIT
  ret

is_irq_enabled:
  mrs x0, daif
  lsr x0, x0, #1
  mvn x0, x0
  and x0, x0, #1 
  ret


  // CACHE 
  ////////////////////////////////////////////////////////////////////////
  // AArch64 Memory Model Feature Register 2
  // ID_AA64MMFR2_EL1
  // |----|-----|-----|-----|-----|-----|-----|-----|
  // |E0PD| EVT | BBM | TTL |RES0 | FWB | IDS | AT  |
  // |----|-----|-----|-----|-----|-----|-----|-----|
  // 63 60 59 56 55 52 51 48 47 44 43 40 39 36 35 32 
  //
  // |----|-----|-------|-------|-----|-----|-----|-----|
  // | ST | NV  | CCIDX |VARange|IESB |LSM  | UAO | CnP |
  // |----|-----|-------|-------|-----|-----|-----|-----|
  // 31 28 27 24 23   20 19   16 15 12 11 8 7 4 3 0
  // 
  // 
mem_model_max_pa_bits:
  mrs x0, id_aa64mmfr0_el1 // AArch64 Memory Model Feature Register 0
  // x0 = 0x1122
  and x0, x0, #0xf
  cmp x0, #0
  beq 1f
  cmp x0, #0b0001
  beq 2f
  cmp x0, #0b0010
  beq 3f
  cmp x0, #0b0011
  beq 4f
  cmp x0, #0b0100
  beq 5f
  cmp x0, #0b0101
  beq 6f
  mov x0, #52 
  b   0f
6:
  mov x0, #48
  b   0f
5:
  mov x0, #44
  b   0f
4:
  mov x0, #42
  b   0f
3:
  mov x0, #40
  b   0f
2:
  mov x0, #36
  b   0f
1:
  mov x0, #32
0:
  ret

mem_model_num_asid_bits:
  mrs   x0, id_aa64mmfr0_el1 // AArch64 Memory Model Feature Register 0
  mov   x1, #0xf
  lsr   x0, x0, #4
  and   x0, x0, x1
  cmp   x0, #0
  beq   1f
  mov   x0, #16
  ret
1:
  mov   x0, #8
  ret

mem_model_4k_granule_support:
  mrs   x0, id_aa64mmfr0_el1
  mov   x1, #0xf
  and   x0, x1, x0, lsr #28
  cmp   x0, #0
  beq   0f
  cmp   x0, #0xf
  beq   1f
  mov   x0, #3
  ret
0:
  mov   x0, #1
  ret
1:
  mov   x0, #0
  ret

mem_model_16k_granule_support:
  mrs   x0, id_aa64mmfr0_el1
  mov   x1, #0xf
  and   x0, x1, x0, lsr #20
  cmp   x0, #0
  beq   0f
  cmp   x0, #0xf
  beq   1f
  mov   x0, #3
  ret
0:
  mov   x0, #1
  ret
1:
  mov   x0, #0
  ret

mem_model_64k_granule_support:
  mrs   x0, id_aa64mmfr0_el1
  mov   x1, #0xf
  and   x0, x1, x0, lsr #20
  cmp   x0, #0
  beq   0f
  cmp   x0, #0xf
  beq   1f
  mov   x0, #3
  ret
0:
  mov   x0, #1
  ret
1:
  mov   x0, #0
  ret

mem_model_st2_4k_granule_support:
  mrs   x0, id_aa64mmfr0_el1
  mov   x1, #0xf
  and   x0, x1, x0, lsr #40
  cmp   x0, #0
  beq   0f
  cmp   x0, #1
  beq   1f
  cmp   x0, #2
  beq   2f
  mov   x0, #3
  ret
0:
  mov   x0, #2
  ret
1:
  mov   x0, #0
  ret
1:
  mov   x0, #1
  ret

mem_model_st2_16k_granule_support:
  mrs   x0, id_aa64mmfr0_el1
  mov   x1, #0xf
  and   x0, x1, x0, lsr #32
  cmp   x0, #0
  beq   0f
  cmp   x0, #1
  beq   1f
  cmp   x0, #2
  beq   2f
  mov   x0, #3
  ret
0:
  mov   x0, #2
  ret
1:
  mov   x0, #0
  ret
2:
  mov   x0, #1
  ret

mem_model_st2_64k_granule_support:
  mrs   x0, id_aa64mmfr0_el1
  mov   x1, #0xf
  and   x0, x1, x0, lsr #36
  cmp   x0, #0
  beq   0f
  cmp   x0, #1
  beq   1f
  cmp   x0, #2
  beq   2f
  mov   x0, #3
  ret
0:
  mov   x0, #2
  ret
1:
  mov   x0, #0
  ret
2:
  mov   x0, #1
  ret


  // CCSIDR_EL1
  // |-------|---------|---------|---------------|---------|
  // | RES0  | UNKNOWN | NumSets | Associativity | LineSize|
  // |-------|---------|---------|---------------|---------|
  // 63    32 31     28 27     13 12            3 2        0
  //
  // LineSize [2:0] Log2(Number of bytes in cache line)

disable_l1_caches:
  mrs x0, sctlr_el1
  bic x0, x0, #(0x1 << 2)
  msr sctlr_el1, x0
  mov x0, #0x0             // x0 - Cache Level
  msr csselr_el1, x0
  mrs x4, ccsidr_el1       // Read Cache Size ID

  // x4 = 0x700fe01a
  and x1, x4, #0x7
  add x1, x1, #0x4         // x1 = Cache Line Size
  ldr x3, =0x7fff
  and x2, x3, x4, lsr #13  // x2 = Cache Set Number - 1
  ldr x3, =0x3ff
  and x3, x3, x4, lsr #3   // x3 = Cache Assosiativity Number - 1
  clz w4, w3               // x4 = way position in CISW instruction
  // Cache has 128 sets and 4 way-associative
  mov x5, #0
  way_loop:
  mov x6, #0
  set_loop:
  lsl   x7, x5, x4
  orr   x7, x0, x7         // Set way
  lsl   x8, x6, x1
  orr   x7, x7, x8         // Set set
  dc    cisw, x7           // Clean and Invalidate cache line.
  add   x6, x6, #1         // Set counter++
  cmp   x6, x2
  ble   set_loop
  add   x5, x5, #1         // Way counter++
  cmp   x5, x3
  ble   way_loop
  ret

disable_l2_caches:
  mrs x0, sctlr_el2
  bic x0, x0, #(0x1 << 2)
  msr sctlr_el2, x0
  ret

disable_l3_caches:
  mrs x0, sctlr_el3
  bic x0, x0, #(0x1 << 2)
  msr sctlr_el3, x0
  ret


// CSSELR_EL1, Cache Size Selection Register
// InD, bit [0] 0b0 Data or unified cache.
//              0b1 Instruction cache.


.macro read_ccsidr_el1
  // x0 - cache level
  // x1 - 1 instruction, 0 data
  orr   x0, x1, x0, lsl #1
  msr   csselr_el1, x0
  mrs   x0, ccsidr_el1       // Read Cache Size ID
.endm

.macro get_cache_line_size
  read_ccsidr_el1
  cmp   x0, #0
  beq   0f
  and   x1, x0, #7
  add   x1, x1, #4
  mov   x0, #1
  lsl   x0, x0, x1
0:
  ret
.endm

.macro get_cache_num_sets
  read_ccsidr_el1
  cmp   x0, #0
  beq   0f
  mov   x1, #0x7fff
  and   x0, x1, x0, lsr #13
  add   x0, x0, #1
0:
  ret
.endm

.macro get_cache_num_ways
  read_ccsidr_el1
  cmp   x0, #0
  beq   0f
  mov   x1, #0x3ff
  and   x0, x1, x0, lsr #3
  add   x0, x0, #1
0:
  ret
.endm
  
FUNC(mem_cache_get_line_size):
  get_cache_line_size

FUNC(mem_cache_get_num_sets):
  get_cache_num_sets

FUNC(mem_cache_get_num_ways):
  get_cache_num_ways

