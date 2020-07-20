#include <gpio.h>
#include <common.h>
#include <compiler.h>
#include "bcm2835_gpio_reg.h"


#define GPIO_CHECK_GPIO_NUM(gpio_num) \
  if (gpio_num > GPIO_MAX_PIN_IDX) \
    return -1

#define GPIO_32PIN_SET_CHEKCED(gpio_num, reg0) \
  GPIO_CHECK_GPIO_NUM(gpio_num); \
  write_reg(reg0 + (gpio_num / 32), (1 << (gpio_num % 32))); \
  return ERR_OK;

#define GPIO_PIN_SELECT_REG(gpio_num) ((GPIO_REG_GPFSEL0 + 4 * (gpio_num / 10)))

#define GPIO_PIN_SELECT_BIT(gpio_num) ((gpio_num % 10) * 3)

int OPTIMIZED gpio_set_function(uint32_t gpio_num, int func)
{
  GPIO_CHECK_GPIO_NUM(gpio_num);
  gpio_read_and_set_3(GPIO_REG_GPFSEL0, gpio_num, func);
  return ERR_OK;
}

int OPTIMIZED gpio_set_on(uint32_t gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPSET0);
}

int OPTIMIZED gpio_set_off(uint32_t gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPCLR0);
}

int OPTIMIZED gpio_set_detect_high(uint32_t gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPHEN0);
}

int OPTIMIZED gpio_set_detect_low(uint32_t gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPLEN0);
}

int OPTIMIZED gpio_set_detect_rising_edge(uint32_t gpio_num)
{
  GPIO_CHECK_GPIO_NUM(gpio_num);
  gpio_read_and_set_64(GPIO_REG_GPREN0, gpio_num);
  return 0;
}

int OPTIMIZED gpio_set_detect_falling_edge(uint32_t gpio_num)
{
  GPIO_CHECK_GPIO_NUM(gpio_num);
  gpio_read_and_set_64(GPIO_REG_GPFEN0, gpio_num);
  return 0;
}

int OPTIMIZED gpio_set_detect_async_rising_edge(uint32_t gpio_num)
{
  GPIO_CHECK_GPIO_NUM(gpio_num);
  gpio_read_and_set_64(GPIO_REG_GPAREN0, gpio_num);
  return 0;
}

int OPTIMIZED gpio_set_detect_async_falling_edge(uint32_t gpio_num)
{
  GPIO_CHECK_GPIO_NUM(gpio_num);
  gpio_read_and_set_64(GPIO_REG_GPAFEN0, gpio_num);
  return 0;
}

#define GPIO_REG_ADDR(reg, pin) (reg + (pin >> 5))

#define GPIO_REG_BIT(pin) (pin % 32)
int gpio_pin_status_triggered(uint32_t gpio_num)
{
  reg32_t regaddr = GPIO_REG_ADDR(GPIO_REG_GPEDS0, gpio_num);
  if (*regaddr & (1 << GPIO_REG_BIT(gpio_num)))
     return 1;
  return 0;
}

int gpio_pin_status_clear(uint32_t gpio_num)
{
  reg32_t regaddr = GPIO_REG_ADDR(GPIO_REG_GPEDS0, gpio_num);
  write_reg(regaddr, 1 << GPIO_REG_BIT(gpio_num));
  return ERR_OK;
}

int OPTIMIZED gpio_set_gppudclk(uint32_t gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPPUDCLK0);
}

int OPTIMIZED gpio_set_pullupdown(uint32_t gpio_num, int pullupdown)
{
  /*
   * According to datasheet BCM2835 ARM Peripherals, page 101
   * 1. Write to GPPUD to set the required control signal (i.e.
   * Pull-up or Pull-Down or neither
   * to remove the current Pull-up/down)
   * 2. Wait 150 cycles – this provides the required set-up time
   * for the control signal
   * 3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
   * modify – NOTE only the pads which receive a clock will be modified, all others will
   * retain their previous state.
   * 4. Wait 150 cycles – this provides the required hold time for the control signal
   * 5. Write to GPPUD to remove the control signal
   * 6. Write to GPPUDCLK0/1 to remove the clock
   *
   */
  register unsigned int r;

  GPIO_CHECK_GPIO_NUM(gpio_num);
  write_reg(GPIO_REG_GPPUD, pullupdown);
  r = 150;
  while(r--) { asm volatile("nop"); }
  gpio_set_gppudclk(gpio_num);
  r = 150;
  while(r--) { asm volatile("nop"); }
  write_reg(GPIO_REG_GPPUD, 0);

  write_reg(GPIO_REG_GPPUDCLK0, 0);
  write_reg(GPIO_REG_GPPUDCLK1, 0);
  return ERR_OK;
}

void gpio_dump_select_regs(const char* tag)
{
  printf("GPFSEL regs: tag: %s\n", tag);
  print_reg32_at(GPIO_REG_GPFSEL0);
  print_reg32_at(GPIO_REG_GPFSEL1);
  print_reg32_at(GPIO_REG_GPFSEL2);
  print_reg32_at(GPIO_REG_GPFSEL3);
  print_reg32_at(GPIO_REG_GPFSEL4);
  print_reg32_at(GPIO_REG_GPFSEL5);
  printf("---------\n");
}

void gpio_power_off(void)
{
  register unsigned long r;
  // power off gpio pins
  write_reg(GPIO_REG_GPFSEL0, 0);
  write_reg(GPIO_REG_GPFSEL1, 0);
  write_reg(GPIO_REG_GPFSEL2, 0);
  write_reg(GPIO_REG_GPFSEL3, 0);
  write_reg(GPIO_REG_GPFSEL4, 0);
  write_reg(GPIO_REG_GPFSEL5, 0);
  write_reg(GPIO_REG_GPPUD, 0);
  r = 150;
  while(r--) { asm volatile("nop"); }
  // flush GPIO setup
  write_reg(GPIO_REG_GPPUDCLK0, 0xffffffff);
  write_reg(GPIO_REG_GPPUDCLK1, 0xffffffff);
  r = 150;
  while(r--) { asm volatile("nop"); }
  write_reg(GPIO_REG_GPPUDCLK0, 0);
}

