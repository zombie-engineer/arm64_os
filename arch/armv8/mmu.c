#include <gpio.h>
#include <mmu.h>
#include <mbox/mbox_props.h>
#include <uart/uart.h>
#include <common.h>
#include <types.h>
#include "armv8_tcr.h"
#include "armv8_sctlr.h"
#include "mmu_memattr.h"
#include "mmu_lpdesc.h"

/* We setup 4096 bytes GRANULE.
 *
 * - Any long-format translation table entry always has 8 bytes size.
 * - Any translation table should fit into page of GRANULE size (4096)
 * - For each table level next 9 bits of VA are taken from the right to
 *   get index of table entry (one of 512 entries in table)
 * - Last table points
 * bit math: 
 * 4096 addresses from 0 to 4095 are covered by 12 bits
 * 512  addresses from 0 to 511  are covered by 9  bits
 * 
 *              L0      L1       L2       L3                           
 * VA [63 : 48][47 : 39][38 : 30][29 : 21][20 : 12][11 : 0]
 *             |        |        |        |                \
 *             |        |        |        |                 \_
 *             |        |        |        |                   \_
 *             |        |        |        |                     \_
 *             |        |        |        |                       \_
 *             |        |        |        |                         \
 *             |        |        |     (table 3)[x]->(page address)+([11:0]offset)
 *             |        |        |
 *             |        |       (table 2)[x]->(table 3)[x]
 *             |        |
 *             |      (table 1)[x]->(table 2)[x]
 *             |
 *          (table 0)[x] -> (table 1)[x]
 *
 *
 *
 * num entries in table = 2**(n - 3) = 2 ** 9 = 512 (3bits - are 8byte alignement)
 * 4096 - 2**12
 * OA[11:0] = IA[11:0]
 * 48 - 11 = 37
 * IA[47:12] 
 * 8-bytes sized descriptor may be addressed by index 
 * in page address   resolves IA[(n-1):0]        = IA[11:0]
 * last lookup level resolves IA[(2n-4):n]       = IA[20:12]
 * prev lookup level resolves IA[(3n-7):(2n-3)]  = IA[29:21]
 * prev lookup level resolves IA[(4n-10):(3n-6)] = IA[38:30]
 * prev lookup level resolves IA[(5n-13):(4n-9)] = IA[47:39]
 */


#define TTBR_CNP    1

#define MAX_TABLE_ENTRIES_FOR_4K_GRANULE  512
#define MAX_TABLE_ENTRIES_FOR_16K_GRANULE 2048
#define MAX_TABLE_ENTRIES_FOR_64K_GRANULE 8192

#define ADDR_BITS_RESOLV_AT_SINGLE_LOOKUP_4K   9
#define ADDR_BITS_RESOLV_AT_SINGLE_LOOKUP_16K 11
#define ADDR_BITS_RESOLV_AT_SINGLE_LOOKUP_64K 13

#define PAGE_SIZE_4K_GRANULE  4096
#define PAGE_SIZE_16K_GRANULE 16384
#define PAGE_SIZE_64K_GRANULE 65536

#define LOWATTR_SH_NORMAL_NON_SHAREABLE 0
#define LOWATTR_SH_NORMAL_OUT_SHAREABLE 2
#define LOWATTR_SH_NORMAL_IN_SHAREABLE  3

#define TABLE_ENTRY_TYPE_BLOCK 1
#define TABLE_ENTRY_TYPE_TABLE 3

  // MAP a range of 32 megabytes
  // map range 32Mb
  // 32Mb = 32 * 1024 * 1024 = 8 * 4 * 1024 * 1024 = 8 * 1024 * 4096 
  // = 8192 x 4K pages 
  // = 8192 l3 pte's
  // 8192 l3 pte's = 16 * 512 
  // = 16 l2 pte's
  // =  1 l1 pte's
  // =  1 l0 pte's

  // MAP a range of 256 megabytes
  // 256MB = 256 * 1024 * 1024 = 64 * 1024 * 4K
  // = 65536 l3 pte's
  // = 128   l2 pte's
  // = 1     l1 pte
  // = 1     l0 pte

  // MAP a range of 1 gigabytes
  // 1024Mb = 1024 * 1024 * 1024 = 256 * 1024 * 4K
  // = 262144 l3 pte's
  // = 512    l2 pte's
  // = 1      l1 pte
  // = 1      l0 pte

#define MMU_PAGE_GRANULE 4096
#define MMU_PTES_PER_LEVEL 512
#define MMU_PT_ALIGNMENT 4096
#define ALIGN_UP(v, to) ((((v) + (to) - 1) / to) * to)

#define LONGLONG(x) ((uint64_t)x)

#define BITS_AT_POS(bits, pos, mask) ((LONGLONG(bits) & mask) << pos)
#define BIT_AT_POS(bit, pos) BITS_AT_POS(bit, pos, 1)

#define PTE_OUT_ADDR(out_addr) LONGLONG(out_addr & (0xffffffffffff - 0xfff))
#define PTE_LO_ATTR(lo_attr)   BITS_AT_POS(lo_attr,  2,  0x3ff)
#define PTE_UP_ATTR(up_attr)   BITS_AT_POS(up_attr, 51, 0x1fff)

#define MAKE_Lx_PTE_VALID_TABLE_4KB(ns_table, ap_table, xn_table, pxn_table, next_lvl_table_addr) \
  BIT_AT_POS(ns_table, 63) | BITS_AT_POS(ap_table, 61, 0b11) | BIT_AT_POS(xn_table, 60) | BIT_AT_POS(pxn_table, 59) | \
  PTE_OUT_ADDR(next_lvl_table_addr) | 0b11
  
#define MAKE_L3_PTE_VALID_PAGE_4KB(up_attr, out_addr, lo_attr) (PTE_UP_ATTR(up_attr) | PTE_OUT_ADDR(out_addr) | PTE_LO_ATTR(lo_attr) | 0b11)

  

#define MAKE_L3_PTE(up_attr, out_addr, lo_attr) MAKE_L3_PTE_VALID_PAGE_4KB(up_attr, out_addr, lo_attr)

#define MAKE_Lx_PTE(ns_table, ap_table, xn_table, pxn_table, next_lvl_table_addr) \
  MAKE_Lx_PTE_VALID_TABLE_4KB(ns_table, ap_table, xn_table, pxn_table, next_lvl_table_addr)

#define MAKE_L3_PTE_LO_ATTR(attr_idx, ns, ap, sh, af, ng) \
  BITS_AT_POS(attr_idx, 0, 0b111) | BIT_AT_POS(ns, 3) | BITS_AT_POS(ap, 4, 0b11) \
  | BITS_AT_POS(sh, 6, 0b11) | BIT_AT_POS(af, 8) | BIT_AT_POS(ng, 9) 
void map_l3_pt(uint64_t* l3_pt_start, int l3_pte_count, uint64_t start_page_pa)
{
  int i;
  uint64_t pa;
  uint64_t pte;
  uint64_t *ppte = l3_pt_start;
  uint64_t lo_attr, up_attr;
  lo_attr = up_attr = 0;

  lo_attr = MAKE_L3_PTE_LO_ATTR(
    0 /* attr_idx */, 
    1 /* ns */, 
    0 /* ap */, 
    0 /* sh */, 
    0, //1 /* af */, 
    0 /* ng */);

  pa = start_page_pa;
  for (i = 0; i < l3_pte_count; ++i) {
    pte = MAKE_L3_PTE(up_attr, pa, lo_attr);
    *(ppte++) = pte;
    pa += 4096;
  }
}

void map_l0l1l2_pt(uint64_t pt_start, int pte_count, uint64_t start_pte)
{
  int i;
  uint64_t *ppte = pt_start;
  uint64_t next_lvl_table_addr;
  uint64_t pte;
  uint64_t ns, ap, xn, pxn;
  ns = ap = xn = pxn = 0;

  next_lvl_table_addr = start_pte;
  for (i = 0; i < pte_count; ++i) {
    pte = MAKE_Lx_PTE(ns, ap, xn, pxn, next_lvl_table_addr);
    *(ppte++) = pte;
    next_lvl_table_addr += MMU_PTES_PER_LEVEL * sizeof(pte);
  }
}

void mmu_map_linear_range(uint64_t l0_pt_start, uint64_t start_pa, uint64_t linear_size, uint64_t start_va)
{
  uint64_t l3_pte_count, l2_pte_count, l1_pte_count, l0_pte_count;
  uint64_t l3_pt_start, l2_pt_start, l1_pt_start;

  l3_pte_count = max(linear_size   / MMU_PAGE_GRANULE  , 1);
  l2_pte_count = max(l3_pte_count  / MMU_PTES_PER_LEVEL, 1);
  l1_pte_count = max(l2_pte_count  / MMU_PTES_PER_LEVEL, 1);
  l0_pte_count = max(l1_pte_count  / MMU_PTES_PER_LEVEL, 1);
  l1_pt_start = ALIGN_UP(l0_pt_start + l0_pte_count, MMU_PT_ALIGNMENT);
  l2_pt_start = ALIGN_UP(l1_pt_start + l1_pte_count, MMU_PT_ALIGNMENT);
  l3_pt_start = ALIGN_UP(l2_pt_start + l2_pte_count, MMU_PT_ALIGNMENT);
  map_l3_pt    (l3_pt_start, l3_pte_count, start_pa);
  map_l0l1l2_pt(l2_pt_start, l2_pte_count, l3_pt_start);
  map_l0l1l2_pt(l1_pt_start, l1_pte_count, l2_pt_start);
  map_l0l1l2_pt(l0_pt_start, l0_pte_count, l1_pt_start);
}

void enable_mmu_tables(uint64_t ttbr0, uint64_t ttbr1)
{
  uint64_t sctlr_el1_val = 0;
  uint64_t mair_reg = 0;
  uint64_t tcr_el1_val = 0;
  char mair_attr0 = MAIR_normal(MAIR_NormReadonly(1), MAIR_NormReadonly(0)); // MAIR_dd_nGnRnE;
  char mair_attr1 = MAIR_dd_nGnRE;
  char mair_attr2 = MAIR_dd_GRE;
  char mair_attr3 = MAIR_normal(MAIR_Norm_NonCacheable(1), MAIR_Norm_NonCacheable(0));
  char mair_attr4 = MAIR_normal(MAIR_Norm(1), MAIR_Norm(0));

  mair_reg |= ((uint64_t)mair_attr0) << (8 * 0);
  mair_reg |= ((uint64_t)mair_attr1) << (8 * 1);
  mair_reg |= ((uint64_t)mair_attr2) << (8 * 2);
  mair_reg |= ((uint64_t)mair_attr3) << (8 * 3);
  mair_reg |= ((uint64_t)mair_attr4) << (8 * 4);
  
  asm volatile("dsb sy");
  asm volatile("msr mair_el1, %0"  :: "r"(mair_reg));
  asm volatile("msr ttbr0_el1, %0" :: "r"(ttbr0));
  asm volatile("msr ttbr1_el1, %0" :: "r"(ttbr1));
  asm volatile("isb");

  // Set translate control register
  // for both ttbr0_el1, ttbr1_el1:
  // - enable table walk
  // - set page granule 4kb
  // - set region size 512Gb
  // - set region caching Normal Mem, Write-Back Read-Allocate Write-Allocate Cacheable
  tcr_el1_t *tcr = (tcr_el1_t*)&tcr_el1_val;
  tcr->IPS   = TCR_IPS_32_BITS;
  tcr->TG1   = TCR_TG_4K;
  tcr->SH1   = TCR_SH_INNER_SHAREABLE;
  tcr->ORGN1 = TCR_RGN_WbRaWaC;
  tcr->IRGN1 = TCR_RGN_WbRaWaC;
  tcr->EPD1  = TCR_EPD_WALK_ENABLED;
  tcr->T1SZ  = 25; // 0x19 - Region size is (2 ** (64 - 25) -> 2 ** 39 
                  // = 0x8 000 000 000 = 512G
  tcr->TG0   = TCR_TG_4K;
  tcr->SH0   = TCR_SH_INNER_SHAREABLE;
  tcr->ORGN0 = TCR_RGN_WbRaWaC;
  tcr->IRGN0 = TCR_RGN_WbRaWaC;
  tcr->EPD0  = TCR_EPD_WALK_ENABLED;
  tcr->T0SZ  = 25; // 0x19 Region Size is 512G
  asm volatile("msr tcr_el1, %0" :: "r"(tcr_el1_val));

  // Set system control register
  sctlr_el1_t *sctlr = (sctlr_el1_t*)&sctlr_el1_val;
  sctlr->I   = 1; // instruction cache enable
  sctlr->SA0 = 1; // stack align check enable
  sctlr->SA  = 1; // stack align check enable
  sctlr->C   = 1; // data cache enable
  sctlr->A   = 1; // alignment check enable
  sctlr->M   = 1; // mmu enable
  // 0xC00800
 
  sctlr_el1_val |= 0x30d00800;
  asm volatile("mov x0, #0; at S1E1R, x0");

  asm volatile("msr sctlr_el1, %0; isb" :: "r"(sctlr_el1_val));

  sctlr->M   = 0; // mmu enable

  asm volatile(
    "msr sctlr_el1, %0;"
    "isb"
      :: "r"(sctlr_el1_val));
}


extern void _armv8_mmu_init(void *pt_l0, uint64_t max_pages);

// ?? why alignment is 16384
static uint64_t __attribute__((aligned(16384))) 
mmu_table_l1[MAX_TABLE_ENTRIES_FOR_4K_GRANULE] = { 0 };

static uint64_t __attribute__((aligned(16384))) 
mmu_table_l2[MAX_TABLE_ENTRIES_FOR_4K_GRANULE] = { 0 };

void mmu_set_ttbr0(uint64_t ttbr0_table)
{
}

void mmu_init()
{
  int max_pa_bits;
  uint64_t pt_l0;
  uint64_t pa_start, pa_range, va_start;
  uint64_t max_pages;
  pt_l0 = 0x900000;
  pa_start = 0;
  va_start = 0;
  pa_range = 1024 * 1024 * 1024;
  max_pa_bits = mem_model_max_pa_bits();
  mmu_map_linear_range(pt_l0, pa_start, pa_range, va_start);
  __enable_mmu(pt_l0, pt_l0);
  //mmu_set_ttbr0(pt_l0);
  return;


  _armv8_mmu_init(pt_l0, max_pages);
  _Static_assert(sizeof(tcr_el1_t)              == 8, "size not 8"); 
  _Static_assert(sizeof(sctlr_el1_t)            == 8, "size not 8");
  _Static_assert(sizeof(vmsav8_64_block_dsc_t)  == 8, "size not 8");
  
  mmu_init_table();
  enable_mmu_tables((uint64_t)&mmu_table_l1[0], (uint64_t)&mmu_table_l2[0]);
}

void mmu_init_table()
{
  uint64_t val = 0;
  vmsav8_64_block_dsc_t* t1 = (vmsav8_64_block_dsc_t*)&val;
  t1->SH = 1;
  t1->NS = 1;
  t1->AF = 1;
  t1->PXNTable = 1;
  t1->XNTable = 1;

  mmu_table_l1[0] = (0x8000000000000000) | val | TABLE_ENTRY_TYPE_BLOCK;
  return;

  int vcmem_base = 0, vcmem_size = 0;
  int block_size = (1<<21);
  int nblocks = 0;
  if (mbox_get_vc_memory(&vcmem_base, &vcmem_size)) {
    printf("failed to get arm memory\n");
    return;
  } else {
    nblocks  = vcmem_size / block_size;
    printf("vc memory base:  %08x,\n size: %08x,\n block_size: %08x,\n nblocks: %d\n",
      vcmem_base,
      vcmem_size,
      block_size, 
      nblocks);
  }
}
