#include <gpio.h>
#include <bits_api.h>
#include <mmu.h>
#include <mbox/mbox_props.h>
#include <uart/uart.h>
#include <common.h>
#include <types.h>
#include <arch/armv8/armv8.h>
#include "armv8_tcr.h"
#include "armv8_sctlr.h"
#include "mmu_memattr.h"
#include "mmu_lpdesc.h"
#include <stringlib.h>
#include <memory/dma_area.h>

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

#define MEMATTR_IDX_NORMAL    0
#define MEMATTR_IDX_DEV_NGNRE 1
#define MEMATTR_IDX_NORMAL2   2

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

#define PTE_OUT_ADDR(phys_page_idx) BITWIDTH64(BITS_AT_POS(phys_page_idx, 12, 0xffffffffff))
#define PTE_LO_ATTR(lo_attr)   BITS_AT_POS(lo_attr,  2,  0x3ff)
#define PTE_UP_ATTR(up_attr)   BITS_AT_POS(up_attr, 51, 0x1fff)

#define MAKE_PAGE_PTE_VALID_PAGE_4KB(up_attr, phys_page_idx, lo_attr) (PTE_UP_ATTR(up_attr) | PTE_OUT_ADDR(phys_page_idx) | PTE_LO_ATTR(lo_attr) | 0b11)

#define MAKE_PAGE_PTE(up_attr, phys_page_idx, lo_attr) MAKE_PAGE_PTE_VALID_PAGE_4KB(up_attr, phys_page_idx, lo_attr)

#define MAKE_PAGE_PTE_LO_ATTR(attr_idx, ns, ap, sh, af, ng) \
  BITS_AT_POS(attr_idx, 0, 0b111) | BIT_AT_POS(ns, 3) | BITS_AT_POS(ap, 4, 0b11) \
  | BITS_AT_POS(sh, 6, 0b11) | BIT_AT_POS(af, 8) | BIT_AT_POS(ng, 9) 

#define MAKE_TABLE_PTE_VALID_TABLE_4KB(ns_table, ap_table, xn_table, pxn_table, next_lvl_table_page) \
  BIT_AT_POS(ns_table, 63) | BITS_AT_POS(ap_table, 61, 0b11) | BIT_AT_POS(xn_table, 60) | BIT_AT_POS(pxn_table, 59) | \
  PTE_OUT_ADDR(next_lvl_table_page) | 0b11
  
#define MAKE_TABLE_PTE(ns_table, ap_table, xn_table, pxn_table, next_lvl_table_page) \
  MAKE_TABLE_PTE_VALID_TABLE_4KB(ns_table, ap_table, xn_table, pxn_table, next_lvl_table_page)

typedef uint64_t mmu_pte_t;

// MMU capabilities
typedef struct mmu_caps {
  int max_pa_bits;
} mmu_caps_t;

// Single range of physical pages, grouped by
// same memory attributes
typedef struct pt_mem_range {
  // Index of a first physical page in range
  uint64_t pa_start_page;
  // Index of a first virtual page in range
  uint64_t va_start_page;
  // Number of pages in range
  uint64_t num_pages;
  // Index of memory attribute in MAIR register
  int mem_attr_idx;
} pt_mem_range_t;


// Page table configuration
typedef struct pt_config {
  uint64_t base_address;
  // Size of a signle mapped page (page granule size)
  int page_size;
  // Number of entries (ptes) in a single page table
  int num_entries_per_pt;
  // [pstart - pa_end] is the range of physical addresses,
  // covered by this page table
  uint64_t pa_start;
  uint64_t pa_end;

  // Memory ranges to map
  int num_ranges;
  pt_mem_range_t mem_ranges[64];
} pt_config_t;

void pt_config_print(pt_config_t *p)
{
  int i;
  uint64_t total_l3_ptes = 0;
  for (i = 0; i < p->num_ranges; ++i) {
    pt_mem_range_t *r = &p->mem_ranges[i];
    total_l3_ptes += r->num_pages;
    printf("mmu range: [0x%016llx:0x%016llx]-[0x%08x:0x%08x], attr_idx:%d"__endline,
      r->va_start_page * MMU_PAGE_GRANULE,
      (r->va_start_page  + r->num_pages) * MMU_PAGE_GRANULE,
      (uint32_t)(r->pa_start_page * MMU_PAGE_GRANULE),
      (uint32_t)((r->pa_start_page  + r->num_pages) * MMU_PAGE_GRANULE),
      r->mem_attr_idx);
  }
  printf("mmu_table at: [0x%08x:0x%08x] (%d ptes)" __endline,
    p->base_address,
    p->base_address + total_l3_ptes * 8, total_l3_ptes);
}

void map_page_ptes(mmu_pte_t *page_pte, uint64_t phys_page_idx, uint64_t num_pages, int mem_attr_idx)
{
  int i;
  uint64_t lo_attr, up_attr;

  up_attr = 0;
  lo_attr = MAKE_PAGE_PTE_LO_ATTR(
    mem_attr_idx,
    1 /* ns */,
    0 /* ap */,
    3 /* sh */,
    1 /* af */,
    0 /* ng */);

  for (i = 0; i < num_pages; ++i) {
    uint64_t pte = MAKE_PAGE_PTE(up_attr, phys_page_idx, lo_attr);
    // printf("map_page_ptes: %p = %llx"__endline, page_pte, pte);
    *(page_pte++) = pte;
    phys_page_idx++;
  }
}

void map_table_ptes(mmu_pte_t *pt, int pte_count, mmu_pte_t *next_level_pte, pt_config_t *pt_config)
{
  int i;
  uint64_t ns, ap, xn, pxn;
  uint64_t next_level_pt_page = (uint64_t)next_level_pte / pt_config->page_size;
  ns = ap = xn = pxn = 0;

  for (i = 0; i < pte_count; ++i) {
    pt[i] = MAKE_TABLE_PTE(ns, ap, xn, pxn, next_level_pt_page);
    next_level_pt_page++;
  }
}

void map_linear_range(uint64_t start_va, mmu_caps_t *mmu_caps, pt_config_t *pt_config)
{
  int i;
  int l3_pte_count;
  int l2_pte_count;
  int l1_pte_count;
  int l0_pte_count;
  pt_mem_range_t *range;

  mmu_pte_t *l3_pt;
  mmu_pte_t *l2_pt;
  mmu_pte_t *l1_pt;
  mmu_pte_t *l0_pt;

#define div_round_up(val, by) ((val + by - 1) / by)
  BUG(pt_config->pa_end % pt_config->page_size, "Unaligned memory translation range");
  l3_pte_count = max((pt_config->pa_end - pt_config->pa_start) / pt_config->page_size, 1);
  l2_pte_count = max(div_round_up(l3_pte_count, pt_config->num_entries_per_pt), 1);
  l1_pte_count = max(div_round_up(l2_pte_count, pt_config->num_entries_per_pt), 1);
  l0_pte_count = max(div_round_up(l1_pte_count, pt_config->num_entries_per_pt), 1);
  printf("num_ptes:l0:%d,l1:%d,l2:%d,l3:%d"__endline,
    l0_pte_count,
    l1_pte_count,
    l2_pte_count,
    l3_pte_count);

  l0_pt = (mmu_pte_t *)(pt_config->base_address);
#define round_up_to(val, to) (((val + to - 1) / to) * to)
  l1_pt = l0_pt + round_up_to(l0_pte_count, pt_config->num_entries_per_pt);
  l2_pt = l1_pt + round_up_to(l1_pte_count, pt_config->num_entries_per_pt);
  l3_pt = l2_pt + round_up_to(l2_pte_count, pt_config->num_entries_per_pt);
#define print_page_table_l(__l)\
  printf("page_table: l"#__l": [0x%08x:0x%08x] (%d entries)" __endline,\
    l## __l ## _pt,\
    l## __l ## _pt + l0_pte_count * 8,\
    l## __l ## _pte_count)

  print_page_table_l(0);
  print_page_table_l(1);
  print_page_table_l(2);
  print_page_table_l(3);

  /*
   * Map level 0 page table entries to level 1 page tables
   */
  map_table_ptes(l0_pt, l0_pte_count, l1_pt, pt_config);

  /*
   * Map level 1 page table entries to level 2 page tables
   */
  map_table_ptes(l2_pt, l2_pte_count, l3_pt, pt_config);

  /*
   * Map level 2 page table entries to level 3 page tables
   */
  map_table_ptes(l1_pt, l1_pte_count, l2_pt, pt_config);

  /*
   * Map level 3 page table entries to actual pages
   */
  for (i = 0; i < pt_config->num_ranges; ++i) {
    range = &pt_config->mem_ranges[i];
    map_page_ptes(l3_pt + range->va_start_page, range->pa_start_page, range->num_pages, range->mem_attr_idx);
  }
}

extern void __armv8_enable_mmu(uint64_t, uint64_t);
extern char __mmu_table_base;

void mmu_self_test(void)
{
  {
#define check_translation(addr) {\
    uint64_t from = addr;\
    uint64_t to = 0xffffffff;\
    asm volatile ("at s1e1r, %1\nmrs %0, PAR_EL1" : "=r"(to) : "r"(from));\
    printf("----%llx -> %llx" __endline, from, to);\
}
    check_translation(0);
    check_translation(0x80000);
    check_translation(0x3f000000);
    check_translation(0x3fffffff);
    check_translation(0x40000000);
    check_translation(0x40000040);
  }
}

#define DECL_RANGE(__start, __num, __memattr) \
  pt_config.mem_ranges[num_ranges].pa_start_page = __start;\
  pt_config.mem_ranges[num_ranges].va_start_page = __start;\
  pt_config.mem_ranges[num_ranges].num_pages     = __num;\
  pt_config.mem_ranges[num_ranges].mem_attr_idx  = MEMATTR_IDX_ ## __memattr;\
  num_ranges++

void mmu_init(void)
{
  uint64_t va_start;
  mmu_caps_t mmu_caps;
  pt_config_t pt_config;
  mair_repr_64_t mair_repr;
  int num_ranges = 0;

  int pg_norm_off = 0;
  int pg_norm_num = dma_area_get_start_addr() / MMU_PAGE_GRANULE;
  int pg_dma_off  = pg_norm_num;
  int pg_dma_num  = dma_area_get_end_addr() / MMU_PAGE_GRANULE - pg_dma_off;
  int pg_shar_off = pg_dma_off + pg_norm_num;
  int pg_shar_num = PERIPHERAL_ADDR_RANGE_START / MMU_PAGE_GRANULE - pg_shar_off;
  int pg_peri_off = PERIPHERAL_ADDR_RANGE_START / MMU_PAGE_GRANULE;
  int pg_peri_num = (PERIPHERAL_ADDR_RANGE_END - PERIPHERAL_ADDR_RANGE_START) / MMU_PAGE_GRANULE;
  int pg_locl_off = LOCAL_PERIPH_ADDR_START / MMU_PAGE_GRANULE;
  int pg_locl_num = (LOCAL_PERIPH_ADDR_END - LOCAL_PERIPH_ADDR_START) / MMU_PAGE_GRANULE;

  pt_config.base_address = (uint64_t)&__mmu_table_base;
  pt_config.page_size = MMU_PAGE_GRANULE;
  pt_config.num_entries_per_pt = MMU_PTES_PER_LEVEL;
  // Cover from 0 to 1GB of physical address space
  pt_config.pa_start = 0;
  pt_config.pa_end = LOCAL_PERIPH_ADDR_END;//(uint64_t)2 * 1024 * 1024 * 1024;

  DECL_RANGE(pg_norm_off, pg_norm_num, NORMAL);
  DECL_RANGE(pg_dma_off , pg_dma_num , DEV_NGNRE);
  DECL_RANGE(pg_shar_off, pg_shar_num, NORMAL2);
  DECL_RANGE(pg_peri_off, pg_peri_num, DEV_NGNRE);
  DECL_RANGE(pg_locl_off, pg_locl_num, DEV_NGNRE);

  pt_config.num_ranges = num_ranges;

  pt_config_print(&pt_config);

  memset(&mair_repr, 0, sizeof(mair_repr));

  // memory region attributes of 0xff enable stxr / ldxr operations
  mair_repr.memattrs[MEMATTR_IDX_NORMAL]   = MAKE_MEMATTR_NORMAL(
      MEMATTR_WRITEBACK_NONTRANS(MEMATTR_RA, MEMATTR_WA),
      MEMATTR_WRITEBACK_NONTRANS(MEMATTR_RA, MEMATTR_WA));
  mair_repr.memattrs[MEMATTR_IDX_DEV_NGNRE] = MEMATTR_DEVICE_NGNRE;
  mair_repr.memattrs[MEMATTR_IDX_NORMAL2]   = MAKE_MEMATTR_NORMAL(
      MEMATTR_WRITEBACK_NONTRANS(MEMATTR_RA, MEMATTR_WA),
      MEMATTR_WRITEBACK_NONTRANS(MEMATTR_RA, MEMATTR_WA));

  armv8_set_mair_el1(mair_repr_64_to_value(&mair_repr));

  // Number of entries (ptes) in a single page table
  mmu_caps.max_pa_bits = mem_model_max_pa_bits();
  va_start = 0;
  map_linear_range(va_start, &mmu_caps, &pt_config);
  __armv8_enable_mmu(pt_config.base_address, pt_config.base_address);
  mmu_self_test();
}
