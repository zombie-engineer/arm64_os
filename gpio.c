#include "gpio.h"
#include "common.h"

int gpio_set_function(int gpio_num, int func)
{
  unsigned int regval, bitpos;
  unsigned int volatile *gpio_sel_reg;
  if (gpio_num > 53)
    return -1;
  gpio_sel_reg = *gpio_pin_select_reg(gpio_num);
  bitpos = gpio_pin_select_bit(gpio_num);
  regval = *gpio_sel_reg;
  regval &= ~(7 << bitpos);
  regval |= (func & 7) << bitpos;
  *gpio_sel_reg = regval;
  return 0;
}

int gpio_set_on(int gpio_num)
{
  unsigned int volatile *gpio_set_reg;
  if (gpio_num > 53)
    return -1;
  gpio_set_reg = gpio_pin_set_reg(gpio_num);
  *gpio_set_reg |= 1 << gpio_pin_set_bit(gpio_num);
  return 0;
}

int gpio_set_off(int gpio_num)
{
  unsigned int volatile *gpio_clear_reg;
  if (gpio_num > 53)
    return -1;
  gpio_clear_reg = gpio_pin_clear_reg(gpio_num);
  *gpio_clear_reg |= 1 << gpio_pin_clear_bit(gpio_num);
  return 0;
}

int gpio_set_detect_high(int gpio_num)
{
  if (gpio_num > 53)
    return -1;

  if (gpio_num < 32)
    GPHEN0 |= 1 << gpio_num;
  else
    GPHEN0 |= 1 << (gpio_num - 32);
  return 0;
}

void gpio_dump_select_regs(const char* tag)
{
  printf("GPFSEL regs: tag: %s\n", tag);
  print_reg32(GPFSEL0);
  print_reg32(GPFSEL1);
  print_reg32(GPFSEL2);
  print_reg32(GPFSEL3);
  print_reg32(GPFSEL4);
  print_reg32(GPFSEL5);
  printf("---------\n");
}

