   // 
   // Arm Architecture Reference Manual
   // Armv8, for Armv8-A architecture profile
   // DDI0487E_a_armv8_arm.pdf

#include <asm_macros.h>
#include <arch/armv8/armv8_asm_macros.h>

#define DAIF_IRQ_BIT    (1 << 1)

// CurrentEL - Current Exception Level
// bits [63:4] - RES0
// bits [3:2]  - EL, Current exception level
//             - 0b00 EL0
//             - 0b01 EL1
//             - 0b10 EL2
//             - 0b11 EL3

.macro GET_CURRENT_EXCEPTION_LEVEL dreg 
  // dreg - destination register
  mrs \dreg, CurrentEl
  // clear reserved bits
  and \dreg, \dreg, #0xc 
.endm


.section ".text"

FUNC(__armv8_set_elr_el1):
  # x0 - value
  msr elr_el1, x0
  ret

FUNC(__armv8_set_sp_el1):
  # x0 - value
  msr sp_el1, x0
  ret

FUNC(armv8_get_mpidr_el1):
  # x0 - cpu core number 0-3
  ldr x1, =__percpu_data
  add x1, x1, x0, lsl #3
  ldr x0, [x1]
  ret

FUNC(enable_irq):
  IRQ_ENABLE
  ret

FUNC(disable_irq):
  IRQ_DISABLE
  ret

FUNC(is_irq_enabled):
  mrs x0, daif
  lsr x0, x0, #1
  mvn x0, x0
  and x0, x0, #1 
  ret


// MEMORY MODEL -----------------------------------------------
// CACHE ------------------------------------------------------

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
FUNC(mem_model_max_pa_bits):
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

FUNC(mem_model_num_asid_bits):
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

FUNC(mem_model_4k_granule_support):
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

FUNC(mem_model_16k_granule_support):
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

FUNC(mem_model_64k_granule_support):
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

FUNC(mem_model_st2_4k_granule_support):
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

FUNC(mem_model_st2_16k_granule_support):
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

FUNC(mem_model_st2_64k_granule_support):
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


  // CSSELR_EL1, Cache Size Selection Register
  // |-----------|---|-------|---|
  // |   RES0    |TnD| Level |InD|
  // |-----------|---|-------|---|
  // 63         5  4  3     1  0

  // InD,   bit    [0] 0b0 Data or unified cache.
  //                   0b1 Instruction cache.
  // Level, bits [3:1] Cache level of required cache.
  // TnD,   ibt    [4] Allocation Tag not Data bit

  // CCSIDR_EL1
  // Format 1
  // |-------|---------|---------|---------------|---------|
  // | RES0  | UNKNOWN | NumSets | Associativity | LineSize|
  // |-------|---------|---------|---------------|---------|
  // 63    32 31     28 27     13 12            3 2        0
  //
  // LineSize [2:0] Log2(Number of bytes in cache line) - 4

.macro READ_CACHE_SIZE level, type, to
  orr   \to, \type, \level, lsl #1
  msr   csselr_el1, \to       // Set Needed Cache Type
  mrs   \to, ccsidr_el1       // Read Cache Size ID
.endm

.macro CACHE_SIZE_NUM_SETS cache_size, to, tmp
  mov   \tmp, #0x7fff
  and   \to, \tmp, \cache_size, lsr #13
  add   \to, \to, #1
.endm

.macro CACHE_SIZE_NUM_WAYS cache_size, to, tmp
  mov   \tmp, #0x3ff
  and   \to, \tmp, \cache_size, lsr #3
  add   \to, \to, #1
.endm

.macro CACHE_SIZE_LINE_SIZE cache_size, to, tmp
  and   \tmp, \cache_size, #7
  add   \tmp, \tmp, #4
  mov   \to, #1
  lsl   \to, \to, \tmp
.endm


  // DC CISW, Data or unified Cache line Clean and Invalidate by Set/Way
  // Clean and Invalidate data cache by set/way.
  // 
  // |-------|--------|-------|-|
  // | RES0  | SetWay | Level |0|
  // |-------|--------|-------|-|
  // 63    32 31      4 3    1 0
  // SetWay, bits[31:4]
  //  -- Way, bits[31:32-A], the number of the way to operate on.
  //  -- Set, bits[B-1:L], the number of the set to operate on.
  //  -- Bits[L-1:4] are RES 0.
  //     A = Log 2 (ASSOCIATIVITY), L = Log 2 (LINELEN), B = (L + S), S = Log 2 (NSETS).
  // Bit[0] - RES0
  // 
  // |--------|-------|------|
  // |  Way   |  Set  | RES0 |
  // |--------|-------|------|
  // |31  32-A| B-1  L|L-1  4|
  // 

  // clz - get leading zeroes explained 
  // x3 = 3 = 0b0000 0000 0000 0000 0000 0000 0000 0011 - last way idx - 
  // all idxes fit into 2 bits
  // A = 2 bits
  // Way offset = 32 - 2 = 30
  // Way bits = [31:30] - 0, 1, 2, 3
  // Set bits = [B-1:6]
  // B = Log2(LINELEN) + Log2(NSETS)
  // w4 = 32 - 2 leading zeroes
  // setway = (set_idx << log2(LINELEN)) | (way_idx << 30)

FUNC(disable_l1_caches):
  mrs   x0, sctlr_el1
  bic   x0, x0, #(0x1 << 2)
  msr   sctlr_el1, x0
  mov   x0,  #0        // cache level 1, cache type: data
  READ_CACHE_SIZE x0, x0, x4
  // x4 = 0x700fe01a
  and   x1, x4, #7
  add   x1, x1, #4          // cache line size
  ldr   x3, =0x7fff
  and   x2, x3, x4, lsr #13 // cache set number - 1
  ldr   x3, = 0x3ff
  and   x3, x3, x4, lsr #3  // cache associativity number - 1
  clz w4, w3                // way position in CISW instruction
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


.macro GET_CACHE_LINE_SIZE
  READ_CACHE_SIZE x1, x0, x0
  cmp   x0, #0
  beq   0f
  CACHE_SIZE_LINE_SIZE x0, x0, x1
0:
  ret
.endm

.macro GET_CACHE_NUM_SETS
  READ_CACHE_SIZE x1, x0, x0
  cmp   x0, #0
  beq   0f
  CACHE_SIZE_NUM_SETS x0, x0, x1
0:
  ret
.endm

.macro GET_CACHE_NUM_WAYS
  READ_CACHE_SIZE x1, x0, x0
  cmp   x0, #0
  beq   0f
  CACHE_SIZE_NUM_WAYS x0, x0, x1
0:
  ret
.endm
  
FUNC(mem_cache_get_line_size):
  GET_CACHE_LINE_SIZE

FUNC(mem_cache_get_num_sets):
  GET_CACHE_NUM_SETS

FUNC(mem_cache_get_num_ways):
  GET_CACHE_NUM_WAYS

/*
 * dcache_clean_and_invalidate_rng - cleans and invalidates
 * all data cache lines that form whole virtual address range
 * given by the arguments.
 * x0 - virtual address of first byte in range
 * x1 - virtual address of end of range
 */
FUNC(dcache_clean_and_invalidate_rng):
  /*
   * 1. Get line width from
   * CTR_EL0 DminLine [16:19]
   * Log2 of number of words in the smallest cache line
   */
  mrs  x3, CTR_EL0
  lsr  x3, x3, #16
  and  x3, x3, #0xf
  mov  x2, #4
  lsl  x2, x2, x3
  /* Now we know cache line size in x2 */
  sub x3, x2, #1
                       // x0 = aligned(start)
                       // align address to cache line offset
  bic x0, x0, x3   // x0 = start & ~(4^(DminLine) - 1)
1:
  dc  civac, x0    // clean & invalidate data or unified cache
  add x0, x0, x2   // x0 += DminLine
  cmp x0, x1       // check end reached
  b.lo  1b
  dsb sy
  ret

FUNC(dcache_line_width):
  /*
   * IN CTL_EL0 DminLine is Log2 of number of WORDS(4bytes)
   * in a cache line.
   */
  mrs x0, ctr_el0
  lsr x0, x0, #16
  and x0, x0, #0xf
  /* x0 - log2 num words */
  mov x1, #4
  lsl x0, x1, x0
  /* x0 - log2 num words * 4 (word size) */
  ret

// MMU --------------------------------------------------------

  ////////////////////////////////////////////////////////////////////////
FUNC(arm_get_ttbr0_el1):
  mrs x0, ttbr0_el1
  ret

FUNC(arm_get_ttbr1_el1):
  mrs x0, ttbr1_el1
  ret

FUNC(arm_set_ttbr0_el1):
  msr ttbr0_el1, x0
  ret

FUNC(arm_set_ttbr1_el1):
  msr ttbr1_el1, x0
  ret

FUNC(arm_get_tcr_el1):
  mrs x0, tcr_el1
  ret

FUNC(arm_set_tcr_el1):
  msr tcr_el1, x0
  ret

    // G8.2.162 TTBCR, Translation Table Base Control Register
    // 


    // D13.2.112 TCR_EL1, Translation Control Register (EL1)
    // The control register for stage 1 of the EL1&0 translation regime.
    // T0SZ, bits  [5:0] The size offset of the memory region addressed by TTBR0_EL
    // EPD0, bit   [7]   Translation table walk disable for translations using TTBR0_EL
    //                   0 Enable
    //                   1 Disable -> Translation fault
    // IRGN0, bits [9:8] Inner cacheability 
    //                  0 Normal memory, Inner Non-cacheable
    //                  1 Normal memory, Inner Write-Back Read-Alloc Write Alloc Cacheable 
    //                  2 Normal memory, Inner Write-Through Read-Alloc No-Write-Alloc Cacheable 
    //                  3 Normal memory, Inner Write-Back Read-Alloc No-Write-Alloc Cacheable 
    // ORGN0, bits[11:10] Outer cacheability
    // SH0,   bits[13:12] Sharebility attr
    //                  0 Non-shareable
    //                  2 Outer Shareable
    //                  3 Inner Shareable
    // TG0,   bits[15:14] Granule size for TTBR0_EL1
    //                  0: 4kb, 1: 64kb, 2: 16kb
    // T1SZ,  bits[21:16] Size offset of the memory region addressed by TTBR1_EL1
    // A1,    bit [22]    Selects whether TTBR0_EL1 or TTBR1_EL1 defines the ASID
    //                  0 - TTBR0_EL1.ASID, 1 - TTBR1_EL1
    // EPD1,  bit [23]  Traslation table walk disable for TTBR1_EL
    // IRGN1, bits[25:24] Inner cacheability 
    // ORGN1, bits[27:26] Outer cacheability 
    // SH1,   bits[29:28] Shareability
    // TG1,   bits[31:30] Granule size for TTBR1_EL1
    // IPS,   bits[34:32] Intermediate Physical Address Size
    //                  0: 32bits 4ggb
    //                  1: 36bits 64gb
    //                  2: 40bits 01tb
    //                  3: 42bits 04tb
    //                  4: 44bits 16tb
    //                  5: 48bits 256tb
    //                  6: 52bits 4pb
    // AS,    bit[35]   ASID Size.  0 - 8 bit, 1 - 16 bit
    // TBI0,  bit[37]   Top Byte ignored
    // TBI1,  bit[38]   Top Byte ignored
    // NFD0,  bit[53]   Non-fault translation table walk disable for stage 1 tranlations
    //                  for TTBR0_EL1
    // NFD1,  bit[54]   Non-fault translation table disable for stage 1 TTBR1_EL1



    // Search "Table D5-33 Stage 1 access permissions for instruction execution for a translation regime that applies to EL0 and a
    // higher Exception level"


.macro LED_ON
  ldr   x6, =0x3F20001c
  mov   w7, #(1<<21)
  str   w7, [x6]
.endm

FUNC(armv8_set_mair_el1):
  msr mair_el1, x0
  ret

FUNC(armv8_get_mair_el1):
  mrs x0, mair_el1
  ret
   
FUNC(__armv8_enable_mmu):
  msr   ttbr0_el1, x0
  msr   ttbr1_el1, x1

  mrs   x2, tcr_el1
   
  // Set T0SZ to 16
  orr   x2, x2, #0b1000

  // Ensure EPD0 bit
  bic   x2, x2, #(1<<7)

  //    IGRN0 - Normal memory, Inner Write-Back Write-Allocate Cacheable.
  bic   x2, x2, #(3<<8)
  orr   x2, x2, #(1<<8)

  //    OGRN0 - Normal memory, Outer Write-Back Write-Allocate Cacheable.
  bic   x2, x2, #(3<<10)
  orr   x2, x2, #(1<<10)

  //    SH0 - Inner shareable
  bic   x2, x2, #(3<<12)
  orr   x2, x2, #(3<<12)

  // Set HA bit
  orr   x2, x2, #(1<<39)
  msr   tcr_el1, x2
  isb
  
  mrs   x0, sctlr_el1
  // M bit
  orr   x0, x0, #(1<<0)
  // bic   x0, x0, #(1<<19)
  // C bit
  orr   x0, x0, #(1<<2)
  msr   sctlr_el1, x0
  
  isb
  ret
