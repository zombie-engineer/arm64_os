#include <intr_ctl.h>
#include <common.h>
#include <arch/armv8/armv8.h>
#include <bits_api.h>

#define BCM2835_IC_BASE          (PERIPHERAL_BASE_PHY  + 0xb200)
#define BCM2835_IC_PENDING_BASIC (reg32_t)(BCM2835_IC_BASE + 0x00)
#define BCM2835_IC_PENDING_GPU_1 (reg32_t)(BCM2835_IC_BASE + 0x04)
#define BCM2835_IC_PENDING_GPU_2 (reg32_t)(BCM2835_IC_BASE + 0x08)
#define BCM2835_IC_FIQ_CONTROL   (reg32_t)(BCM2835_IC_BASE + 0x0c)
#define BCM2835_IC_ENABLE_GPU_1  (reg32_t)(BCM2835_IC_BASE + 0x10)
#define BCM2835_IC_ENABLE_GPU_2  (reg32_t)(BCM2835_IC_BASE + 0x14)
#define BCM2835_IC_ENABLE_BASIC  (reg32_t)(BCM2835_IC_BASE + 0x18)
#define BCM2835_IC_DISABLE_GPU_1 (reg32_t)(BCM2835_IC_BASE + 0x1c)
#define BCM2835_IC_DISABLE_GPU_2 (reg32_t)(BCM2835_IC_BASE + 0x20)
#define BCM2835_IC_DISABLE_BASIC (reg32_t)(BCM2835_IC_BASE + 0x24)

#define BASIC_IRQ_TIMER             (1 << 0)
#define BASIC_IRQ_MAILBOX           (1 << 1)
#define BASIC_IRQ_DOORBELL_0        (1 << 2)
#define BASIC_IRQ_DOORBELL_1        (1 << 3)
#define BASIC_IRQ_GPU_0_HALTED      (1 << 4)
#define BASIC_IRQ_GPU_1_HALTED      (1 << 5)
#define BASIC_IRQ_ACCESS_ERR_TYPE_0 (1 << 6)
#define BASIC_IRQ_ACCESS_ERR_TYPE_1 (1 << 7)

static intr_ctl_irq_cb irq_callbacks[64 + 8];

int intr_ctl_set_cb(int irq_type, int irq_num, intr_ctl_irq_cb cb)
{
  if (irq_num + irq_type > ARRAY_SIZE(irq_callbacks))
    return ERR_INVAL_ARG;

  irq_callbacks[irq_num + irq_type] = cb;
  return ERR_OK;
}

void intr_ctl_dump_regs(const char* tag)
{
  printf("ARM_TIMER_REGS: tag: %s\n", tag);
  print_reg32(BCM2835_IC_PENDING_BASIC);
  print_reg32(BCM2835_IC_PENDING_GPU_1);
  print_reg32(BCM2835_IC_PENDING_GPU_2);
  print_reg32(BCM2835_IC_FIQ_CONTROL);
  print_reg32(BCM2835_IC_ENABLE_GPU_1);
  print_reg32(BCM2835_IC_ENABLE_GPU_2);
  print_reg32(BCM2835_IC_ENABLE_BASIC);
  print_reg32(BCM2835_IC_DISABLE_GPU_1);
  print_reg32(BCM2835_IC_DISABLE_GPU_2);
  print_reg32(BCM2835_IC_DISABLE_BASIC);
  printf("---------\n");
}


uint32_t intr_ctl_read_pending_gpu_1()
{
  return read_reg(BCM2835_IC_PENDING_GPU_1);
}


int intr_ctl_arm_irq_enable(int irq_num)
{
  if (irq_num > INTR_CTL_IRQ_ARM_MAX)
    return ERR_INVAL_ARG;

  write_reg(BCM2835_IC_ENABLE_BASIC, (1 << irq_num));
  return ERR_OK;
}


int intr_ctl_arm_irq_disable(int irq_num)
{
  if (irq_num > INTR_CTL_IRQ_ARM_MAX)
    return ERR_INVAL_ARG;

  write_reg(BCM2835_IC_DISABLE_BASIC, (1 << irq_num));
  return ERR_OK;
}


#define ACCESS_GPU_IRQ(r, irq) \
  reg32_t dst;                        \
  if (irq_num > INTR_CTL_IRQ_ARM_MAX) \
    return ERR_INVAL_ARG;             \
  dst = r;                            \
  if (irq_num > 31) {                 \
    irq_num -= 32;                    \
    dst += 4;                         \
  }                                   \
  write_reg(dst, (1 << irq_num));     \
  return ERR_OK;


int intr_ctl_gpu_irq_enable(int irq_num)
{
  ACCESS_GPU_IRQ(BCM2835_IC_ENABLE_GPU_1, irq_num);
}


int intr_ctl_gpu_irq_disable(int irq_num)
{
  ACCESS_GPU_IRQ(BCM2835_IC_DISABLE_GPU_1, irq_num);
}


void intr_ctl_enable_gpio_irq(int gpio_num)
{
  uint32_t irq_enable_reg;

  irq_enable_reg = 0;
  irq_enable_reg |= 1 << (49 - 32);
  irq_enable_reg |= 1 << (50 - 32);
  irq_enable_reg |= 1 << (51 - 32);
  irq_enable_reg |= 1 << (52 - 32);
  write_reg(BCM2835_IC_ENABLE_GPU_2, irq_enable_reg);
  write_reg(BCM2835_IC_ENABLE_BASIC, 1 << gpio_num);
}

#define CHECK_PENDING_GPU(i, gpu_i) \
  if (basic_pending & (1<<i)) {\
    gpu_pending |= gpu_i;\
  }

#define CHECKED_RUN_CB(cb) \
  if ((cb)) cb()

void intr_ctl_handle_irq(void)
{
  int i;
  uint32_t basic_pending;
  uint64_t gpu_pending;

  basic_pending = read_reg(BCM2835_IC_PENDING_BASIC);

  // if we are here some interrupt has fired.
  // the check is not needed 99% 
  // if (!basic_pending)
  //   return;

  gpu_pending = 0;

  CHECK_PENDING_GPU(10, 7);
  CHECK_PENDING_GPU(11, 9);
  CHECK_PENDING_GPU(12, 10);
  CHECK_PENDING_GPU(13, 18);
  CHECK_PENDING_GPU(14, 19);
  CHECK_PENDING_GPU(15, 53);
  CHECK_PENDING_GPU(16, 54);
  CHECK_PENDING_GPU(17, 55);
  CHECK_PENDING_GPU(18, 56);
  CHECK_PENDING_GPU(19, 57);
  CHECK_PENDING_GPU(20, 62);

  if (basic_pending & (1 << 8))
    gpu_pending |= read_reg(BCM2835_IC_PENDING_GPU_1);
  if (basic_pending & (1 << 9))
    gpu_pending |= ((uint64_t)(read_reg(BCM2835_IC_PENDING_GPU_2))) << 32;

  // start irq handling
  if (basic_pending & 0xff) {
    // run arm irq callbacks
    for (i = 0; i < 8; ++i) {
      if (basic_pending & (1 << i)) {
        CHECKED_RUN_CB(irq_callbacks[INTR_CTL_IRQ_TYPE_GPU + i]);
      }
    }
  }

  if (!gpu_pending)
    return;

  // run gpu irq callbacks
  for (i = 0; i < 64; ++i) {
    if (gpu_pending & (1 << i)) {
      CHECKED_RUN_CB(irq_callbacks[i]);
    }
  }
}
