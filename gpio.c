#include "gpio.h"

int gpio_set_function(int gpio_num, int func)
{
  unsigned int regval, off;
  unsigned int volatile *gpio_sel_reg;
  if (gpio_num > 53)
    return -1;
  gpio_sel_reg = GPFSEL0 + gpio_num / 10;
  off = (gpio_num % 10) * 3;
  regval = *gpio_sel_reg;
  regval &= ~(7 << off);
  regval |= (func & 7) << off;
  *gpio_sel_reg = regval;
  return 0;
}

int gpio_set_on(int gpio_num)
{
  unsigned int volatile *gpio_set_reg;
  if (gpio_num > 53)
    return -1;
  gpio_set_reg = GPSET0 + gpio_num / 32;
  *gpio_set_reg |= (1 << (gpio_num % 32));
  return 0;
}

int gpio_set_off(int gpio_num)
{
  unsigned int volatile *gpio_set_reg;
  if (gpio_num > 53)
    return -1;
  gpio_set_reg = GPCLR0 + gpio_num / 32;
  *gpio_set_reg |= (1 << (gpio_num % 32));
  return 0;
}
