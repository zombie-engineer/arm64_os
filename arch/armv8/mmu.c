#include <gpio.h>
#include <mmu.h>
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


// ?? why alignment is 16384
static uint64_t __attribute__((aligned(16384))) 
mmu_table_l1[MAX_TABLE_ENTRIES_FOR_4K_GRANULE] = { 0 };

static uint64_t __attribute__((aligned(16384))) 
mmu_table_l2[MAX_TABLE_ENTRIES_FOR_4K_GRANULE] = { 0 };

void mmu_init()
{
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
  uint64_t t;
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