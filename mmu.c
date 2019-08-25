#include "gpio.h"
#include "uart.h"
#include "common.h"
#include "types.h"
#include "armv8_tcr.h"
#include "armv8_sctlr.h"

/* We setup 4096 bytes GRANULE.
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
 */

#define PAGESIZE    4096

// granularity
#define PT_PAGE     0b11        // 4k granule
#define PT_BLOCK    0b01        // 2M granule
// accessibility
#define PT_KERNEL   (0<<6)      // privileged, supervisor EL1 access only
#define PT_USER     (1<<6)      // unprivileged, EL0 access allowed
#define PT_RW       (0<<7)      // read-write
#define PT_RO       (1<<7)      // read-only
#define PT_AF       (1<<10)     // accessed flag
#define PT_NX       (1UL<<54)   // no execute
// shareability
#define PT_OSH      (2<<8)      // outter shareable
#define PT_ISH      (3<<8)      // inner shareable
// defined in MAIR register
#define PT_MEM      (0<<2)      // normal memory
#define PT_DEV      (1<<2)      // device MMIO
#define PT_NC       (2<<2)      // non-cachable

#define TTBR_CNP    1

// get addresses from linker
extern volatile unsigned char _data;
extern volatile unsigned char _end;

/**
 * Set up page translation tables and enable virtual memory
 */
void mmu_init1()
{
    unsigned long data_page = (unsigned long)&_data/PAGESIZE;
    unsigned long r, b, *paging=(unsigned long*)&_end;
    printf("data page: %d, paging at 0x%016lx\n", (int)data_page, paging);

    /* create MMU translation tables at _end */

    // TTBR0, identity L1
    paging[0]=(unsigned long)((unsigned char*)&_end+2*PAGESIZE) |    // physical address
        PT_PAGE |     // it has the "Present" flag, which must be set, and we have area in it mapped by pages
        PT_AF |       // accessed flag. Without this we're going to have a Data Abort exception
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // identity L2, first 2M block
    paging[2*512]=(unsigned long)((unsigned char*)&_end+3*PAGESIZE) | // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // identity L2 2M blocks
    b=MMIO_BASE>>21;
    // skip 0th, as we're about to map it by L3
    for(r=1;r<512;r++)
        paging[2*512+r]=(unsigned long)((r<<21)) |  // physical address
        PT_BLOCK |    // map 2M block
        PT_AF |       // accessed flag
        PT_NX |       // no execute
        PT_USER |     // non-privileged
        (r>=b? PT_OSH|PT_DEV : PT_ISH|PT_MEM); // different attributes for device memory

    // identity L3
    for(r=0;r<512;r++)
        paging[3*512+r]=(unsigned long)(r*PAGESIZE) |   // physical address
        PT_PAGE |     // map 4k
        PT_AF |       // accessed flag
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        ((r<0x80||r>=data_page)? PT_RW|PT_NX : PT_RO); // different for code and data

    // TTBR1, kernel L1
    paging[512+511]=(unsigned long)((unsigned char*)&_end+4*PAGESIZE) | // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_KERNEL |   // privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // kernel L2
    paging[4*512+511]=(unsigned long)((unsigned char*)&_end+5*PAGESIZE) |   // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_KERNEL |   // privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // kernel L3
    paging[5*512]=(unsigned long)(MMIO_BASE+0x00201000) |   // physical address
        PT_PAGE |     // map 4k
        PT_AF |       // accessed flag
        PT_NX |       // no execute
        PT_KERNEL |   // privileged
        PT_OSH |      // outter shareable
        PT_DEV;       // device memory

    /* okay, now we have to set system registers to enable MMU */

    // check for 4k granule and at least 36 bits physical address bus */
    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
    b=r&0xF;
    if(r&(0xF<<28)/*4k*/ || b<1/*36 bits*/) {
        uart_puts("ERROR: 4k granule or 36 bit address space not supported\n");
        return;
    }

    // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
    r=  (0xFF << 0) |    // AttrIdx=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) |    // AttrIdx=1: device, nGnRE (must be OSH too)
        (0x44 <<16);     // AttrIdx=2: non cacheable
    asm volatile ("msr mair_el1, %0" : : "r" (r));

    // next, specify mapping characteristics in translate control register
    r=  (0b00LL << 37) | // TBI=0, no tagging
        (b << 32) |      // IPS=autodetected
        (0b10LL << 30) | // TG1=4k
        (0b11LL << 28) | // SH1=3 inner
        (0b01LL << 26) | // ORGN1=1 write back
        (0b01LL << 24) | // IRGN1=1 write back
        (0b0LL  << 23) | // EPD1 enable higher half
        (25LL   << 16) | // T1SZ=25, 3 levels (512G)
        (0b00LL << 14) | // TG0=4k
        (0b11LL << 12) | // SH0=3 inner
        (0b01LL << 10) | // ORGN0=1 write back
        (0b01LL << 8) |  // IRGN0=1 write back
        (0b0LL  << 7) |  // EPD0 enable lower half
        (25LL   << 0);   // T0SZ=25, 3 levels (512G)
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));

    // tell the MMU where our translation tables are. TTBR_CNP bit not documented, but required
    // lower half, user space
    asm volatile ("msr ttbr0_el1, %0" : : "r" ((unsigned long)&_end + TTBR_CNP));
    // upper half, kernel space
    asm volatile ("msr ttbr1_el1, %0" : : "r" ((unsigned long)&_end + TTBR_CNP + PAGESIZE));

    // finally, toggle some bits in system control register to enable page translation
    asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
    r|=0xC00800;     // set mandatory reserved bits
    r&=~((1<<25) |   // clear EE, little endian translation tables
         (1<<24) |   // clear E0E
         (1<<19) |   // clear WXN
         (1<<12) |   // clear I, no instruction cache
         (1<<4) |    // clear SA0
         (1<<3) |    // clear SA
         (1<<2) |    // clear C, no cache at all
         (1<<1));    // clear A, no aligment check
    r|=  (1<<0);     // set M, enable MMU
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));
}


#define MAX_TABLE_ENTRIES_FOR_4K_GRANULE  512
#define MAX_TABLE_ENTRIES_FOR_16K_GRANULE 2048
#define MAX_TABLE_ENTRIES_FOR_64K_GRANULE 8192

#define ADDR_BITS_RESOLV_AT_SINGLE_LOOKUP_4K   9
#define ADDR_BITS_RESOLV_AT_SINGLE_LOOKUP_16K 11
#define ADDR_BITS_RESOLV_AT_SINGLE_LOOKUP_64K 13

#define PAGE_SIZE_4K_GRANULE  4096
#define PAGE_SIZE_16K_GRANULE 16384
#define PAGE_SIZE_64K_GRANULE 65536
// 0x00000000 00000000
// 0xffff0000 00000000
// 0x0000ffff ffffffff - bits of address
// n - granule, n = 12
// to resolve = 48 - n = 36
// num entries in table = 2**(n - 3) = 2 ** 9 = 512 (3bits - are 8byte alignement)
// 4096 - 2**12
// OA[11:0] = IA[11:0]
// 48 - 11 = 37
// IA[47:12] 
// 8-bytes sized descriptor may be addressed by index 
// in page address   resolves IA[(n-1):0]        = IA[11:0]
// last lookup level resolves IA[(2n-4):n]       = IA[20:12]
// prev lookup level resolves IA[(3n-7):(2n-3)]  = IA[29:21]
// prev lookup level resolves IA[(4n-10):(3n-6)] = IA[38:30]
// prev lookup level resolves IA[(5n-13):(4n-9)] = IA[47:39]

// typedef struct {
//   char attr_indx : 3;
//   char ns        : 1;
//   char ap        : 2;
//   char sh        : 2;
//   char af        : 1;
//   char ng        : 1;
// } __attribute__((packed)) lower_block_attributes;
// 
// typedef struct { 
//   char valid : 1;
//   enum {
//     DESCRIPTOR_TYPE_BLOCK = 0,
//     DESCRIPTOR_TYPE_TABLE = 1,
//   } descriptor_type : 1;
//   union {
//     struct {
//       lower_block_attributes low_attrs;
//       char oa        : 4;
//       char nt        : 1;
//     } block;
//   } 
// } __attribute__((packed)) armv8_mmu_descriptor;


#define MAIR_dd_nGnRnE (0 << 2)
#define MAIR_dd_nGnRE  (1 << 2)
#define MAIR_dd_nGRE   (2 << 2)
#define MAIR_dd_GRE    (3 << 2)

#define MAIR_Norm_RW_NoAllocate 0
#define MAIR_Norm_RW_Allocate   1


#define MAIR_WriteThrough 0
#define MAIR_WriteBack    1

#define MAIR_Transient    0
#define MAIR_NonTransient 1

#define MAIR_Norm_RW(R,W) ((R&1)|((W&1)<<1))
#define MAIR_Norm_WrTyp(WrTyp) ((WrTyp)<<2)
#define MAIR_Norm_TrTyp(TrTyp) ((TrTyp)<<3)

#define MAIR_Norm_Cacheable(in, R, W, WrTyp, TrnsTyp) \
  (((MAIR_Norm_RW(R,W)) & 2)|\
  (MAIR_Norm_WrTyp(WrTyp) & 1)|\
  (MAIR_Norm_TrTyp(TrnsTyp) & 1))

#define MAIR_Norm_NonCacheable(in) (0b0100 << 4 * in)

#define MAIR_Norm(in) MAIR_Norm_Cacheable(in, \
  MAIR_Norm_RW_Allocate,\
  MAIR_Norm_RW_Allocate,\
  MAIR_WriteBack,\
  MAIR_NonTransient) << 4 * in

#define MAIR_NormReadonly(in) MAIR_Norm_Cacheable(in, \
  MAIR_Norm_RW_Allocate,\
  MAIR_Norm_RW_NoAllocate,\
  MAIR_WriteBack,\
  MAIR_NonTransient) << 4 * in

#define MAIR_normal(iiii, oooo) (((iiii)&0xf)<<4|((oooo)&0xf))




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

typedef struct {
  char     valid_bit      : 1;
  char     page_block_bit : 1;
  // lower attributes
  char AttrIndx  : 3;
  char NS        : 1;
  char AP        : 2;
  char SH        : 2;
  char AF        : 1;
  char nG        : 1;

  uint64_t address        : 36;
  char     res0_48        : 3;
  char     ign_52         : 7;
  char     PXNTable       : 1;
  char     XNTable        : 1;
  char     APTable        : 2;
  char     NSTable        : 1;

} __attribute__((packed)) vmsav8_64_block_dsc_t;


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

#define LOWATTR_SH_NORMAL_NON_SHAREABLE 0
#define LOWATTR_SH_NORMAL_OUT_SHAREABLE 2
#define LOWATTR_SH_NORMAL_IN_SHAREABLE  3

#define TABLE_ENTRY_TYPE_BLOCK 1
#define TABLE_ENTRY_TYPE_TABLE 3

void mmu_init_table()
{
  uint64_t val = 0;
  vmsav8_64_block_dsc_t* t1 = (vmsav8_64_block_dsc_t*)&val;
  // t1->AP = 1;
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

  // for (i = 0; i < nblocks; ++i) {
  //   mvsav8_64_block_dsc_t* d = (mvsav8_64_block_dsc_t*)&mmu_table_l2[i];
  //   t = TABLE_ENTRY_TYPE_BLOCK;
  //   t |= xxxx << 21
  //   mmu_table_l2[i] = t;
  // }
  // mmu_table_l1[0] = (0x8000000000000000) | &mmu_table_l2[0] | TABLE_ENTRY_TYPE_TABLE;
}
