#include <mbox/mbox.h>
#include <compiler.h>
#include <gpio.h>
#include <common.h>
#include <spinlock.h>
#include <cpu.h>
#include <error.h>

static aligned(64) uint64_t mbox_lock;

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

int mbox_prop_call_no_lock()
{
  int ret;

#ifdef CONFIG_DEBUG_LED_MBOX
  blink_led(1, 10);
#endif
  ret = mbox_call_blocking(MBOX_CH_PROP);
  /* Here I leave return value so other code could check if
   * ret val is 0 on success or non-zero on failer common logic,
   * but tight now any non-zero returns should be immediately paniced
   * Later, some ret codes could be non-zero and still not paniced
   * and got to the caller.
   */

  if (ret)
    kernel_panic("mbox_call_blocking failed");
#ifdef CONFIG_DEBUG_LED_MBOX
  blink_led(1, 10);
#endif
  return ret;
}

int mbox_prop_call()
{
  int ret, should_lock;
  should_lock = spinlocks_enabled && is_irq_enabled();
  if (should_lock)
    spinlock_lock(&mbox_lock);

  ret = mbox_prop_call_no_lock();

  if (should_lock)
    spinlock_unlock(&mbox_lock);
  return ret;
}
