#include <mbox/mbox.h>
#include <compiler.h>
#include <gpio.h>
#include <common.h>

volatile aligned(16) uint32_t mbox_buffer[36];

#define VIDEOCORE_MBOX (PERIPHERAL_BASE_PHY + 0xb880)
#define MBOX_READ   ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x00))
#define MBOX_POLL   ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x10))
#define MBOX_SENDER ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x14))
#define MBOX_STATUS ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x18))
#define MBOX_CONFIG ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x1c))
#define MBOX_WRITE  ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x20))
#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL     0x80000000
#define MBOX_EMPTY    0x40000000

#ifdef CONFIG_DEBUG_MBOX
static void print_mbox()
{
  int i;
  printf("mbox addr = 0x%08x\n", (uint32_t)mbox_addr);
  for (i = 0; i < 8; ++i)
    printf("mbox[%d] = 0x%08x\n", i, mbox[i]);
  puts("--------\n");
}
#endif // CONFIG_DEBUG_MBOX

void flush_dcache_range(uint64_t start, uint64_t stop)
{
  asm volatile (
  "mrs  x3, ctr_el0\n" // Cache Type Register
  "lsr  x3, x3, #16\n" // 
  "and  x3, x3, #0xf\n"// x3 = (x3 >> 16) & 0xf - DminLine - Log2 of number of words in the smallest cache line
  "mov  x2, #4\n"      // x2 = 4
  "lsl  x2, x2, x3\n"  /* cache line size */
  /* x2 <- minimal cache line size in cache system */
  "sub x3, x2, #1\n"   
                       // x0 = aligned(start)
                       // align address to cache line offset
  "bic x0, x0, x3\n"   // x0 = start & ~(4^(DminLine) - 1)
  "1:\n"
  "dc  civac, x0\n"    // clean & invalidate data or unified cache
  "add x0, x0, x2\n"   // x0 += DminLine
  "cmp x0, x1\n"       // check end reached
  "b.lo  1b\n"      
  "dsb sy\n"
  );
}

int mbox_call(unsigned char ch)
{
  uint32_t regval;
  uint64_t mbox_addr = (uint64_t)mbox_buffer;
  regval = (uint32_t)mbox_addr | (ch & 0xf);

  // wait until we can write to mailbox
  while(read_reg(MBOX_STATUS) & MBOX_FULL);

  flush_dcache_range(mbox_addr, mbox_addr);
  // write address of out message
  write_reg(MBOX_WRITE, regval);

  while(1) {
    while (read_reg(MBOX_STATUS) & MBOX_EMPTY);
    if (read_reg(MBOX_READ) == regval) {
      flush_dcache_range(mbox_addr, mbox_addr);
      return mbox_buffer[1] == MBOX_RESPONSE;
    }
  }
  return 0;
}
