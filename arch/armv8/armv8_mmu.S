#include "armv8_mmu_config.h"

.section .text

.section .text.init_mmu

// Map sequence of contignous physical pages to an l3 page table
// x0 - base address to an l3 page table
// x1 - starting physical address of a page to map
// x2 - number of contignous pages to map
// x3 - lower attributes
// x4 - upper attributes
.globl _armv8_mmu_map_pte_l3
_armv8_mmu_map_pte_l3:
  // BaseAddr[PAMax-1:x]:IA[y:12]:0b000; if 43 < TnSZ <= 48 x = 55 - TnSZ else = 12
  // PAMax = 48; TnSZ = 16; x = 12; y = x + 8 = 20
  // pte_l3 = pte_l2[47:12]:IA[20:12]:0b000

  // Compose a page descriptor entry 
  // dst_reg     - register to store the entry
  // lo_attr_reg - lower attributes
  // oa_reg      - physical address of a page
  // up_attr_reg - upper attributes

  // 1. Set lower attributes 
  pte_addr .req x0
  oa       .req x1
  pg_cnt   .req x2
  lo_attr  .req x3
  up_attr  .req x4
  pte_val  .req x5
  end_oa   .req x6

  mov  pte_val, xzr
  ubfx pte_val, lo_attr, #ARMV8_PTE_LOWATTR_BIT_WIDTH_4K, #ARMV8_PTE_LOWATTR_BIT_POS_4K

  // 2. Set bits 0 and 1 to 1 for a valid page descriptor entry
  orr   pte_val, pte_val, #0b11

  // 3. Set output address to bits[47:12]
  // x1 - is a physical address of a page with width of 48 bits and
  // aligned to 4K, meaning 12 first bits are 0, we do not write
  // these 12 bits to descriptor, which leaves us with a 48 - 12 = 36
  // bits written down to a page descriptor
  ubfx  oa, oa, #ARMV8_PAGE_BITS_4K, #ARMV8_PTE_OA_BIT_WIDTH_4K
  mov   end_oa, oa 
  add   end_oa, end_oa, pg_cnt
  ubfx  pte_val, oa, #ARMV8_PTE_OA_BIT_POS_4K, #ARMV8_PTE_OA_BIT_WIDTH_4K
  
  // 4. Set uppert attributes to bits[63:51]
  ubfx  pte_val, up_attr, #ARMV8_PTE_UPATTR_BIT_WIDTH_4K, #ARMV8_PTE_UPATTR_BIT_POS_4K
1:
  add   oa, oa, #1
  ubfx  pte_val, oa, #ARMV8_PTE_OA_BIT_POS_4K, #ARMV8_PTE_OA_BIT_WIDTH_4K
  str   pte_val, [pte_addr], #8
  cmp   oa, end_oa
  blt 1b

  .unreq pte_addr
  .unreq oa
  .unreq pg_cnt
  .unreq pte_val
  .unreq lo_attr
  .unreq up_attr
  .unreq end_oa
  ret


.globl _armv8_mmu_map_pte_l2
_armv8_mmu_map_pte_l2:
  // BaseAddr[PAMax-1:x]:IA[y:21]:0b000; if 34 < TnSZ <= 39 x = 46 - TnSZ else = 12
  // PAMax = 48; TnSZ = 16; x = 12; y = x + 17 = 29
  // pte_l2 = pte_l1[47:12]:IA[29:21]:0b000


.globl _armv8_mmu_map_pte_l1
_armv8_mmu_map_pte_l1:
  // BaseAddr[PAMax-1:x]:IA[y:30]:0b000; if 25 <= TnSZ <= 33 x = 37 - TnSZ else = 12
  // PAMax = 48; TnSZ = 16; x = 12; y = x + 26 = 38
  // ptr_l1 = pte_l0[47:12]:IA[38:30]:0b000


// x0 - base address to an l0 page table
// x1 - starting physical address of a page to map
// x2 - number of contignous pages to map
// x3 - lower attributes
// x4 - upper attributes
.globl _armv8_mmu_map_pte_l0
_armv8_mmu_map_pte_l0:
  // BaseAddr[PAMax-1:x]:IA[y:39]:0b000
  // if z<= TnSZ <= 24, then x = (28 - TnSZ)
  // y = (x + 35); z = 16
  // For PAMax = 48 TnSZ = 16, x = 28-16=12; y = 37; z = 16
  // pte_l0 = TTBR0[47:12]:IA[47:39]:0b000
  ret
  stp x5, x6 , [sp, #-16]!
  stp x7, x8 , [sp, #-16]!
  stp x9, x10, [sp, #-16]!
  pages_per_pte_l1 .req x4
  mov   pages_per_pte_l1, #1
  mov    pages_per_pte_l1,  pages_per_pte_l1, lsl #(9 * 3)
1:
  bl   _armv8_mmu_map_pte_l1  
  bne  1b

  ldp x9, x10, [sp], #16
  ldp x7, x8, [sp], #16
  ldp x5, x6, [sp], #16

  .unreq pages_per_pte_l1
  ret


// Map pages 
//.globl _armv8_mmu_init
//_armv8_mmu_init:
//  stp x5, x6 , [sp, #-16]!
//  stp x7, x8 , [sp, #-16]!
//  stp x9, x10, [sp, #-16]!
//
//  pt_l0_addr       .req x4
//  max_pages        .req x5
//  pages_per_pte_l0 .req x6
//
//  mov pt_l0_addr, x0
//  mov max_pages , x1
//  mov   pages_per_pte_l0, #1
//  mov   pages_per_pte_l0, pages_per_pte_l0, lsl #(9 * 4)
//1:
//  mov   x0, pt0_addr
//  mov   x0, pages_per_pte_l0
//  bl    _armv8_mmu_map_pte_l0
//  subs  max_pages, max_pages, pages_per_pte_l0
//  add   pt0_addr, pt0_addr, #8
//  bgt   1b
//
//  .unreq pt_l0_addr
//  .unreq max_pages
//
//  ldp x9, x10, [sp], #16
//  ldp x7, x8, [sp], #16
//  ldp x5, x6, [sp], #16
//  ret
  

// Chapter D5.2.8
// Table D5-25. Translation table entry addresses when using 4KB
// translation granule.

l0_pte_idx:
  // BaseAddr[39:x]:IA[y:39]:0b000; x = 12; y = 47
  // BaseAddr[39:12]:IA[47:39]:0b000

l1_pte_idx:
  // BaseAddr[39:x]:IA[y:30]:0b000; x = 12; y = 38
  // BaseAddr[39:12]:IA[38:30]:0b000

l2_pte_idx:
  // BaseAddr[39:x]:IA[y:21]:0b000; x = 12; y = 29
  // BaseAddr[39:12]:IA[29:21]:0b000

l3_pte_idx:
  // BaseAddr[39:x]:IA[y:12]:0b000; x = 12; y = 20
  // BaseAddr[39:12]:IA[20:12]:0b000
