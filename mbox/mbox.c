#include <mbox/mbox.h>
#include <compiler.h>
#include <gpio.h>
#include <common.h>
#include <spinlock.h>
#include <cpu.h>
#include <error.h>
#include <config.h>
#include <debug.h>

static DECL_SPINLOCK(mbox_lock);

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
static void debug_mbox()
{
  int i;
  int dcache_width = (int)dcache_line_width();
  printf("mbox addr = %p, dcache_line_sz: %d, dcache_addr: %p\n",
    mbox_buffer, dcache_width,
    (uint64_t)mbox_buffer & ~(dcache_width - 1)
  );
  for (i = 0; i < 8; ++i)
    printf("mbox[%d] = 0x%08x\n", i, mbox_buffer[i]);
}
#endif // CONFIG_DEBUG_MBOX

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

#define mbox_flush_dcache()\
  dcache_flush(mbox_buffer, sizeof(mbox_buffer));

int mbox_prop_call()
{
  int ret, should_lock;

#ifdef CONFIG_DEBUG_MBOX
  debug_mbox();
#endif

  should_lock = spinlocks_enabled && is_irq_enabled();
  if (should_lock)
    spinlock_lock(&mbox_lock);

  mbox_flush_dcache();
  ret = mbox_prop_call_no_lock();
  mbox_flush_dcache();

  if (should_lock)
    spinlock_unlock(&mbox_lock);

#ifdef CONFIG_DEBUG_MBOX
  debug_mbox();
#endif
  return ret;
}
