#include <intr_ctl.h>
#include <common.h>
#include <arch/armv8/armv8.h>
#include <bits_api.h>
#include <reg_access.h>
#include <board/bcm2835/bcm2835_irq_ctrl.h>

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
  if (irq_num > INTR_CTL_IRQ_GPU_MAX) \
    return ERR_INVAL_ARG;             \
  dst = r;                            \
  if (irq_num > 31) {                 \
    irq_num -= 32;                    \
    dst++;                            \
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


/*
 * For details see BCM2835 ARM Peripherals, page 113
 * there is an ARM Peripherals interrupts table, that
 * shows 64 possible IRQs, for gpio looks like we only 
 * need gpio_int[0], which is bit 49 IRQ#49.
 * To access bits 32-64, we need to go to 
 * IRQ_PENDING_2/ENABLE_IRQS_2/DISABLE_IRQS_2 (second
 * half of this table) so IRQ 49 will be bit number 
 * 49-32=17
 */
#define IRQ_BIT_GPIO1 17


void intr_ctl_enable_gpio_irq(int gpio_num)
{
  write_reg(BCM2835_IC_ENABLE_GPU_2, 1 << IRQ_BIT_GPIO1);
}

void intr_ctl_disable_gpio_irq(int gpio_num)
{
  write_reg(BCM2835_IC_DISABLE_GPU_2, 1 << IRQ_BIT_GPIO1);
}

#define CHECK_PENDING_GPU(i, gpu_i) \
  if (basic_pending & (1<<i)) {\
    gpu_pending |= (((uint64_t)1)<<gpu_i);\
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
    if (gpu_pending & (1ull << i)) {
      CHECKED_RUN_CB(irq_callbacks[i]);
    }
  }
}
