#include <timer.h>
#include <reg_access.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <common.h>
#include <intr_ctl.h>
#include <exception.h>
#include <error.h>

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

typedef struct {
  timer_callback_t cb;
  void *cb_arg;
  int is_oneshot;
} bcm2835_timer_t;

static bcm2835_timer_t bcm2835_arm_timer;

static void bcm2835_arm_timer_enable_freerunning()
{
  uint32_t control_reg; 
  control_reg = read_reg(ARM_TIMER_CONTROL_REG);
  control_reg |= ARM_TIMER_CONTROL_REG_ENABLE_FREE;
  write_reg(ARM_TIMER_CONTROL_REG, control_reg);
}

static inline 
void bcm2835_arm_timer_disable()
{
  uint32_t control_reg; 
  control_reg = read_reg(ARM_TIMER_CONTROL_REG);
  control_reg &= ~ARM_TIMER_CONTROL_REG_ENABLE;
  write_reg(ARM_TIMER_CONTROL_REG, control_reg);
}
 
static void bcm2835_arm_timer_disable_freerunning()
{
  uint32_t control_reg; 
  control_reg = read_reg(ARM_TIMER_CONTROL_REG);
  control_reg &= ~ARM_TIMER_CONTROL_REG_ENABLE_FREE;
  write_reg(ARM_TIMER_CONTROL_REG, control_reg);
}

int bcm2835_arm_timer_init()
{
  bcm2835_arm_timer.cb = 0;
  bcm2835_arm_timer.cb_arg = 0;
  bcm2835_arm_timer.is_oneshot = 0;
  bcm2835_arm_timer_disable();
  bcm2835_arm_timer_disable_freerunning();
  return ERR_OK;
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

void timer_irq_callback()
{
  if (read_reg(ARM_TIMER_RAW_IRQ_REG))
    write_reg(ARM_TIMER_IRQ_CLEAR_ACK_REG, 1);
  if (bcm2835_arm_timer.is_oneshot)
    bcm2835_arm_timer_disable();
  if (bcm2835_arm_timer.cb)
    bcm2835_arm_timer.cb(bcm2835_arm_timer.cb_arg);
}

static uint32_t get_core_clock_rate()
{
  uint32_t result;
  // clock rate - is HZ : number of clocks per sec
  if (mbox_get_clock_rate(MBOX_CLOCK_ID_CORE, &result))
     generate_exception();
  // 250 000 000 Hz

  // if (mbox_get_clock_rate(MBOX_CLOCK_ID_ARM, &arm_clock_rate))
  //   generate_exception();
  // // 600 000 000 Hz
  return result;
}

static inline
int bcm2835_arm_timer_set_and_enable(uint32_t load_reg_value)
{
  int ret;
  // set counter 
  write_reg(ARM_TIMER_LOAD_REG, load_reg_value);
  write_reg(ARM_TIMER_CONTROL_REG, ARM_TIMER_CONTROL_REG_WIDTH | ARM_TIMER_CONTROL_REG_ENABLE_IRQ | ARM_TIMER_CONTROL_REG_ENABLE);

  // clear interrupt ack
  write_reg(ARM_TIMER_IRQ_CLEAR_ACK_REG, 0xffffffff);

  // unmask basic interrupt in interrupt controller
  ret = intr_ctl_arm_irq_enable(INTR_CTL_IRQ_ARM_TIMER);
  return ret;
}

#define GET_COUNTER_VALUE(clock_rate, predivider_clock, usec) \
  (clock_rate / (predivider_clock + 1) / SEC_TO_USEC(1) * usec)

int bcm2835_arm_timer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  uint32_t core_clock_rate, predivider_clock;

  bcm2835_arm_timer_disable();

  core_clock_rate = get_core_clock_rate();
  predivider_clock = read_reg(ARM_TIMER_PRE_DIVIDER_REG) & 0x3ff;

  bcm2835_arm_timer.cb = cb;
  bcm2835_arm_timer.cb_arg = cb_arg;
  bcm2835_arm_timer.is_oneshot = 0;
  return bcm2835_arm_timer_set_and_enable(GET_COUNTER_VALUE(core_clock_rate, predivider_clock, usec));
}

int bcm2835_arm_timer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  uint32_t core_clock_rate, predivider_clock;

  bcm2835_arm_timer_disable();

  core_clock_rate = get_core_clock_rate();
  predivider_clock = read_reg(ARM_TIMER_PRE_DIVIDER_REG) & 0x3ff;

  bcm2835_arm_timer.cb = cb;
  bcm2835_arm_timer.cb_arg = cb_arg;
  bcm2835_arm_timer.is_oneshot = 0;
  return bcm2835_arm_timer_set_and_enable(GET_COUNTER_VALUE(core_clock_rate, predivider_clock, usec));
}

