#include "mmu.h"
#include "gpio.h"
#include "uart.h"

#define PAGE_SIZE  4096

// accessibility
#define PT_PAGE    0x3
#define PT_BLOCK   0x1
#define PT_KERNEL  (0<<6)    // privelidged, supervisor EL1 access only
#define PT_USER    (1<<6)    // unprivelidged, EL0 access allowed
#define PT_RW      (0<<7)    // read-write
#define PT_RO      (1<<7)    // read-only
#define PT_AF      (1<<10)   // accessed flag
#define PT_NX      (1UL<<54) // no execute

// sharebility
#define PT_OSH     (2<<8)    // outter shareable
#define PT_ISH     (3<<8)    // inner shareable
// defined in MIAR register
#define PT_MEM     (0<<2)    // normal memory
#define PT_DEV     (1<<2)    // device memory
#define PT_NC      (2<<2)    // non-cachable

#define TTBR_CNP   1

// get addresses from linker
extern volatile unsigned char _data;
extern volatile unsigned char _end;

/**
 * Set up page translation tables and enable virtual memory
 */
void mmu_init()
{
  unsigned long ttbr0_el1 = (unsigned long)0x100000;
  unsigned long ttbr1_el1 = (unsigned long)0x100008;
  unsigned long tcr_el1 = 0x0;
  asm volatile("msr ttbr0_el1, %0\n"::"r"(ttbr0_el1));
  asm volatile("msr ttbr1_el1, %0\n"::"r"(ttbr1_el1));
  asm volatile("msr tcr_el1, %0\n"::"r"(tcr_el1));
  asm volatile("isb\n");
  asm volatile("mrs x0, sctlr_el1\n");
  asm volatile("orr x0, x0, #1\n");
  asm volatile("msr sctlr_el1, x0\n");
  asm volatile("isb\n");
  return;
  unsigned long data_page = (unsigned long)&_data / PAGE_SIZE;
  unsigned long r, b, *paging = (unsigned long*)&_end;

  // TTBR0, identity L1
  paging[0] = (unsigned long)((unsigned long*)&_end + 2 * PAGE_SIZE) // phy addr
    | PT_PAGE // it has the "Present" flag, which must be set, area in it mapped by pages
    | PT_AF   // accessed flag. Without this we're going to have a Data Abort exception
    | PT_USER // non-privelidged
    | PT_ISH  // inner shareable
    | PT_MEM; // normal memory

  // identity L2, first 2M block
  paging[2 * 512] = (unsigned long)((unsigned char*)&_end + 3 * PAGE_SIZE) // physical address
    | PT_PAGE // we have area in it mapped by pages
    | PT_AF   // accessed flag
    | PT_USER // non-privileged
    | PT_ISH  // inner shareable
    | PT_MEM; // normal memory

  // identity L2 2M blocks
  b = MMIO_BASE >> 21;
  for (r = 1; r < 512; r++)
  paging[2 * 512 + r]=(unsigned long)((r << 21)) // physical address
    | PT_BLOCK // map 2M block
    | PT_AF    // accessed flag
    | PT_NX    // no execute
    | PT_USER  // non-privileged
    | (r >= b ? PT_OSH|PT_DEV : PT_ISH|PT_MEM); // different attributes for device memory


  // identity L3
  for (r = 0; r < 512; r++)
    paging[3 * 512 + r]=(unsigned long)(r * PAGE_SIZE) // physical address
      | PT_PAGE // map 4k
      | PT_AF   // accessed flag
      | PT_USER // non-privileged
      | PT_ISH  // inner shareable
      | ((r<0x80 || r >= data_page) ? PT_RW|PT_NX : PT_RO); // different for code and data

 
  // TTBR1, kernel L1
  paging[512 + 511] = (unsigned long)((unsigned char*)&_end + 4 * PAGE_SIZE) // phy address
    | PT_PAGE   // we have area in it mapped by pages
    | PT_AF     // accessed flag
    | PT_KERNEL // privileged
    | PT_ISH    // inner shareable
    | PT_MEM;   // normal memory

  // kernel L2
  paging[4 * 512 + 511]=(unsigned long)((unsigned char*)&_end + 5 * PAGE_SIZE) // phy address
    | PT_PAGE   // we have area in it mapped by pages
    | PT_AF     // accessed flag
    | PT_KERNEL // privileged
    | PT_ISH    // inner shareable
    | PT_MEM;   // normal memory

  // kernel L3
  paging[5*512]=(unsigned long)(MMIO_BASE + 0x00201000) // physical address
    | PT_PAGE   // map 4k
    | PT_AF     // accessed flag
    | PT_NX     // no execute
    | PT_KERNEL // privileged
    | PT_OSH    // outter shareable
    | PT_DEV;   // device memory

  /* okay, now we have to set system registers to enable MMU */
  asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
  b = r & 0xf;
  if (r & (0xf << 28) /* 4k */ || b < 1 /* 36 bits */) {
    uart_puts("ERROR: 4k granule or 36 bit address space not supported\n");
    return;
  }
  // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
  r =  (0xFF << 0) |    // AttrIdx=0: normal, IWBWA, OWBWA, NTR
       (0x04 << 8) |    // AttrIdx=1: device, nGnRE (must be OSH too)
       (0x44 <<16);     // AttrIdx=2: non cacheable
  asm volatile ("msr mair_el1, %0" : : "r" (r));

  // next, specify mapping characteristics in translate control register
  r = (0b00LL << 37) // TBI = 0, no tagging
    | (b      << 32) // IPS = autodetected
    | (0b10LL << 30) // TG1 = 4k
    | (0b11LL << 28) // SH1 = 3 inner
    | (0b01LL << 26) // ORGN1 = 1 write back
    | (0b01LL << 24) // IRGN1 = 1 write back
    | (0b0LL  << 23) // EPD1 enable higher half
    | (25LL   << 16) // T1SZ = 24, 3 levels (512G)
    | (0b00LL << 14) // TG0 = 4k
    | (0b11LL << 12) // SH0 = 3 innter
    | (0b01LL << 10) // ORGN0 = 1 write back
    | (0b01LL << 8 ) // IRGN0 = 1 write back
    | (0b0LL  << 7 ) // EPD0 enable lower half
    | (25LL   << 0 );// TOSZ = 24, 3 levels (512G)

  asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));
  // tell the MMU where our translation tables are. TTBR_CNP bit not documented, but required
  // lower half, user space
  asm volatile ("msr ttbr0_el1, %0" :: "r" ((unsigned long)&_end + TTBR_CNP));
  // upper half, kernel space
  asm volatile ("msr ttbr1_el1, %0" :: "r" ((unsigned long)&_end + TTBR_CNP + PAGE_SIZE));

  // finally, toggle some bits in system control register to enable page translation
  asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
  r |= 0xc00800; // set mandatory reserved bits
  r &=~((1<<25)  // clear EE, little endian translation tables
       |(1<<24)  // clear E0E
       |(1<<19)  // clear WXN
       |(1<<12)  // clear I, no instruction cache
       |(1<<4 )  // clear SA0
       |(1<<3 )  // clear SA
       |(1<<2 )  // clear C, no cache at all
       |(1<<1 ));// clear A, no alignemt chack
  r |= (1<<0);   // set M, enable MMU
  asm volatile ("msr sctlr_el1, %0; isb" :: "r" (r));
}
