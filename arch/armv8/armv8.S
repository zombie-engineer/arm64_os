   // 
   // Arm Architecture Reference Manual
   // Armv8, for Armv8-A architecture profile
   // DDI0487E_a_armv8_arm.pdf

#include <asm_macros.h>

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

FUNC(enable_irq):
  msr daifclr, #DAIF_IRQ_BIT
  ret

FUNC(disable_irq):
  msr daifset, #DAIF_IRQ_BIT
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


    // D13.2.84 MAIR_EL1, Memory Attribute Indirection Register (EL1)
    // Provides the memory attribute encodings corresponding to the possible AttrIndx values in a
    // Long-descriptor format translation table entry for stage 1 translations at EL1.
    // Attr<n>, bits [8n+7:8n], for n = 0 to 7
    //    0b0000dd00 Device memory. 
    //    0b0000ddxx (xx != 00) unpredictable
    //    0booooiiii (oooo != 0000 and iiii != 0000) Normal memory 
    //    0b11110000 Tagged Normal Memory.
    //    0bxxxx0000 (xxxx != 0000 and xxxx != 1111) unpredictable
    //    dd: 0b00 Device-nGnRnE memory
    //        0b01 Device-nGnRE memory
    //        0b10 Device-nGRE memory
    //        0b11 Device-GRE memory
    //    oooo: 0b0000
    //          0b00RW, RW not 0b00 Normal mem, Outer Write-throught Transient
    //          0b0100              Normal mem, Outer Non-cacheable
    //          0b01RW, RW not 0b00 Normal mem, Outer Write-Back Transient
    //          0b11RW              Normal mem, Outer Write-Back Non-transient
    //    iiii: 0b0000

    // Search "Table D5-33 Stage 1 access permissions for instruction execution for a translation regime that applies to EL0 and a
    // higher Exception level"


.macro LED_ON
  ldr   x6, =0x3F20001c
  mov   w7, #(1<<21)
  str   w7, [x6]
.endm

FUNC(arm_mmu_enable):
  mrs   x0, s3_1_c15_c2_1 // Read CPUECTLR_EL1 register
  orr   x0, x0, #(1 << 6) // SMPEN bit enable data coherency with other cores in the cluster.
  msr   s3_1_c15_c2_1, x0

  // Enable caches and the MMU.
  mrs   x0, SCTLR_EL1
  orr   x0, x0, #(1 << 2)  // C bit (data cache)
  orr   x0, x0, #(1 << 12) // I bit (instruction cache)
  orr   x0, x0, #1         // M bit (MMU)
  bic   x0, x0, #(3 << 19) // UWXN, WXN
  msr   SCTLR_EL1, x0
  // dsb   sy // Full system DMB operation
  ret

.macro ORR_BIT Rd, Rtmp, V, Mask, Pos
  mov   \Rtmp,  #\V
  and   \Rtmp, \Rtmp, \Mask
  orr   \Rd, \Rd, \Rtmp, lsl #\Pos
.endm

.macro SET_LOWER_ATTRS Rd, Rtmp, AttrIdx, NS, AP, SH, AF, nG
  bic   \Rd, \Rd, #0x3ff
  ORR_BIT \Rd, \Rtmp, \AttrIdx, 3, 2
  ORR_BIT \Rd, \Rtmp, \NS, 1, 5
  ORR_BIT \Rd, \Rtmp, \AP, 3, 6
  ORR_BIT \Rd, \Rtmp, \SH, 3, 8
  ORR_BIT \Rd, \Rtmp, \AF, 1, 10
  ORR_BIT \Rd, \Rtmp, \nG, 1, 11
.endm

FUNC(arm_mmu_init):
  ldr   x1, =0x3520     // [5:0]   = 32 region size is pow(2, 64 - 32) = pow(2,32) = 4GB
                        // [9:8]   = 1 Normal Inner cacheable
                        // [11:10] = 1 Normal Outer cacheable
                        // [12:13] = 3 Inner-shareable
  msr   TCR_EL1, x1
  ldr   x1, =0xff440400 // Attr0 - Device-nGnRnE, Attr1 Device, Attr2 - Normal Non-Cacheeable
  msr   MAIR_EL1, x1    // Attr2 - Normal Non-Cacheeable
  adr   x0, ttb0_base   // 4kb-aligned address
  add   x0, x0, #1      // 
  msr   TTBR0_EL1, x0
  bic   x0, x0, #1

  // Set up translation table entries in memory with looped store
  // instructions
  // Set level 1 translation table
  // The first entry points to level2_pagetable
  ldr   x1, =level2_pagetable // 4kb aligned
  ldr   x2, =0x00000000fffff000
  and   x2, x1, x2            // NSTable=0 APTable=0 XNTable=0 PXNTable=0
  orr   x2, x2, #3
  str   x2, [x0]              // Long Format Table Descriptor
  
  // Set level 2 translation table
  ldr   x0, =level2_pagetable // Base address of level2_pagetable.
  ldr   x2, =0x0000074d       // Executale Inner and Outer Shareable.
  bic   x2, x2, #(3<<6)       // R/W at all ELs secure memory.
                              // AttrIdx=011 Normal Cacheable.
  str   x2, [x0]              // Long Format Block Descriptor
  ret   

  // Put a 64-bit value with little endianness.
  .macro PUT_64B high, low
    .word \low
    .word \high
  .endm

  // Create an entry pointing to next-level table
  .macro TABLE_ENTRY PA, ATTR
    PUT_64B \ATTR, (\PA) + 0x3
  .endm

  .macro BLOCK_2MB PHYS_ADDR, ATTR_HI, ATTR_LO
    PUT_64B \ATTR_HI, (\PHYS_ADDR & 0xffe00000)| \ATTR_LO | 0x1
  .endm

  .align 12      // 12 for 4kb granule
  ttb0_base:
  TABLE_ENTRY level2_pagetable, 0

  .align 12      // 12 for 4kb granule
  level2_pagetable:
  .set ADDR, 0x000
  .rept 0x200
  BLOCK_2MB (ADDR << 20), 0, 0x74c
  .set ADDR, ADDR+2 
  .endr

FUNC(armv8_enable_mmu):
  msr   ttbr0_el1, x0
  msr   ttbr1_el1, x1

  mrs   x2, tcr_el1
  // Set T0SZ to 16
  orr   x2, x2, #0b1000
  // Set HA bit
  orr   x2, x2, #(1<<39)
  msr   tcr_el1, x2
  isb
  
  mrs   x0, sctlr_el1
  // mov   x1, x0
  orr   x0, x0, #1
  bic   x0, x0, #(1<<19)
  // Cacheability control
  // bic   x0, x0, #(1<<2)
  msr   sctlr_el1, x0
  // msr   sctlr_el1, x1
  
  isb
  ret
