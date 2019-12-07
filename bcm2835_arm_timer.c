#include <timer.h>
#include <reg_access.h>
#include <mbox/mbox.h>
#include <common.h>
#include <exception.h>

#define SEC_TO_USEC(x) (x * 1000 * 1000)

#define ARM_TIMER_BASE                     (uint64_t)(PERIPHERAL_BASE_PHY + 0xb400)
#define ARM_TIMER_LOAD_REG                 (reg32_t)(ARM_TIMER_BASE)
#define ARM_TIMER_VALUE_REG                (reg32_t)(ARM_TIMER_BASE + 4)
#define ARM_TIMER_CONTROL_REG              (reg32_t)(ARM_TIMER_BASE + 8)
#define ARM_TIMER_IRQ_CLEAR_ACK_REG        (reg32_t)(ARM_TIMER_BASE + 12)
#define ARM_TIMER_RAW_IRQ_REG              (reg32_t)(ARM_TIMER_BASE + 16)
#define ARM_TIMER_MASKED_IRQ_REG           (reg32_t)(ARM_TIMER_BASE + 20)
#define ARM_TIMER_RELOAD_REG               (reg32_t)(ARM_TIMER_BASE + 24)
#define ARM_TIMER_PRE_DIVIDER_REG          (reg32_t)(ARM_TIMER_BASE + 28)
#define ARM_TIMER_FREE_RUNNING_COUNTER_REG (reg32_t)(ARM_TIMER_BASE + 32)

#define ARM_TIMER_CONTROL_REG_WIDTH       (1 << 1)
#define ARM_TIMER_CONTROL_REG_ENABLE_IRQ  (1 << 5)
#define ARM_TIMER_CONTROL_REG_ENABLE      (1 << 7)
#define ARM_TIMER_CONTROL_REG_ENABLE_FREE (1 << 9)

typedef enum {
  Clkdiv1      = 0b00,
  Clkdiv16     = 0b01,
  Clkdiv256    = 0b10,
  Clkdiv_undef = 0b11,
} TIMER_PRESCALE;

static void bcm2835_arm_timer_enable_freerunning()
{
  uint32_t control_reg; 
  control_reg = read_reg(ARM_TIMER_CONTROL_REG);
  control_reg |= ARM_TIMER_CONTROL_REG_ENABLE_FREE;
  write_reg(ARM_TIMER_CONTROL_REG, control_reg);
}

static void bcm2835_arm_timer_disable_freerunning()
{
  uint32_t control_reg; 
  control_reg = read_reg(ARM_TIMER_CONTROL_REG);
  control_reg &= ~ARM_TIMER_CONTROL_REG_ENABLE_FREE;
  write_reg(ARM_TIMER_CONTROL_REG, control_reg);
}

void bcm2835_arm_timer_init()
{
  bcm2835_arm_timer_disable_freerunning();
}

void bcm2835_arm_timer_dump_regs(const char* tag)
{
  printf("ARM_TIMER_REGS: tag: %s\n", tag);
  print_reg32(ARM_TIMER_LOAD_REG);
  print_reg32(ARM_TIMER_VALUE_REG);
  print_reg32(ARM_TIMER_CONTROL_REG);
  print_reg32(ARM_TIMER_IRQ_CLEAR_ACK_REG);
  print_reg32(ARM_TIMER_RAW_IRQ_REG);
  print_reg32(ARM_TIMER_MASKED_IRQ_REG);
  print_reg32(ARM_TIMER_RELOAD_REG);
  print_reg32(ARM_TIMER_PRE_DIVIDER_REG);
  print_reg32(ARM_TIMER_FREE_RUNNING_COUNTER_REG);
  printf("---------\n");
}

static timer_callback timer_cb = 0;
static void * timer_cb_arg     = 0;

void timer_irq_callback()
{
  // bcm2835_arm_timer_dump_regs("timer_irq_callback");
  if (read_reg(ARM_TIMER_RAW_IRQ_REG))
    write_reg(ARM_TIMER_IRQ_CLEAR_ACK_REG, 1);
  if (timer_cb)
    timer_cb(timer_cb_arg);
}

void bcm2835_arm_timer_set(uint32_t usec, timer_callback cb, void *cb_arg)
{
  uint32_t core_clock_rate, arm_clock_rate;
  uint32_t control_reg; 
  uint32_t predivider_clock;
  // clock rate - is HZ : number of clocks per sec
  if (mbox_get_clock_rate(MBOX_CLOCK_ID_CORE, &core_clock_rate))
     generate_exception();
  // 250 000 000 Hz
  bcm2835_arm_timer_dump_regs("timer_irq_callback");

  predivider_clock = read_reg(ARM_TIMER_PRE_DIVIDER_REG) & 0x3ff;
  if (mbox_get_clock_rate(MBOX_CLOCK_ID_ARM, &arm_clock_rate))
     generate_exception();
  // 600 000 000 Hz
  //
  // 300 microseconds = 1 / 300 of a second = 0.3333 = (600 000 000 / 1000 0000) * 300

  // printf("core_clock_rate: %d, arm_clock_rate: %d\n", core_clock_rate, arm_clock_rate);
  // return;
  timer_cb = cb;
  timer_cb_arg = cb_arg;

  control_reg = read_reg(ARM_TIMER_CONTROL_REG);
  // write_reg(ARM_TIMER_LOAD_REG, microsec * (core_clock_rate / MICROSECONDS_PER_SECOND));
  write_reg(ARM_TIMER_LOAD_REG, core_clock_rate / (predivider_clock + 1) / SEC_TO_USEC(1) * usec);

  control_reg &= ~ARM_TIMER_CONTROL_REG_ENABLE;
  write_reg(ARM_TIMER_CONTROL_REG, control_reg);
  control_reg = ARM_TIMER_CONTROL_REG_WIDTH | ARM_TIMER_CONTROL_REG_ENABLE_IRQ | ARM_TIMER_CONTROL_REG_ENABLE;
  write_reg(ARM_TIMER_CONTROL_REG, control_reg);

  write_reg(ARM_TIMER_IRQ_CLEAR_ACK_REG, 0xffffffff);
}
