#include "gpio.h"
#include "common.h"

#define GPIO_REG_32PIN_REG(gpio_num, reg0) ((reg32_t)reg0 + (gpio_num / 32))

#define GPIO_REG_32PIN_BIT(gpio_num) (gpio_num % 32)

#define GPIO_32PIN_REG_SET_PIN(gpio_num, reg0) (*GPIO_REG_32PIN_REG(gpio_num, reg0) |= (1 << GPIO_REG_32PIN_BIT(gpio_num)));

#define GPIO_32PIN_REG_GET(gpio_num) (*GPIO_REG_32PIN_REG(gpio_num, GPIO_REG_GPLEV0) & (1 << GPIO_REG_32PIN_BIT(gpio_num)));

#define GPIO_32PIN_REG_CLR_PIN(gpio_num, reg0) (*GPIO_REG_32PIN_REG(gpio_num, reg0) &= ~(1 << GPIO_REG_32PIN_BIT(gpio_num)));

#define GPIO_CHECK_GPIO_NUM(gpio_num) \
  if (gpio_num > GPIO_MAX_PIN_IDX) \
    return -1

#define GPIO_32PIN_SET_CHEKCED(gpio_num, reg0) \
  GPIO_CHECK_GPIO_NUM(gpio_num);\
  GPIO_32PIN_REG_SET_PIN(gpio_num, reg0);\
  return 0;

#define GPIO_PIN_SELECT_REG(gpio_num) ((reg32_t)(GPIO_REG_GPFSEL0 + 4 * (gpio_num / 10)))

#define GPIO_PIN_SELECT_BIT(gpio_num) ((gpio_num % 10) * 3)

int gpio_set_function(int gpio_num, int func)
{
  unsigned int regval, bitpos;
  reg32_t gpio_sel_reg;
  GPIO_CHECK_GPIO_NUM(gpio_num);

  gpio_sel_reg = GPIO_PIN_SELECT_REG(gpio_num);
  bitpos = GPIO_PIN_SELECT_BIT(gpio_num);
  regval = *gpio_sel_reg;
  regval &= ~(7 << bitpos);

  if (func != GPIO_FUNC_IN)
  {
    regval |= (func & 7) << bitpos;
    *gpio_sel_reg = regval;
  }
  return 0;
}

int gpio_is_set(int gpio_num)
{
  GPIO_CHECK_GPIO_NUM(gpio_num);
  return GPIO_REG_32PIN_REG(gpio_num, GPIO_REG_GPLEV0) == 0 ? 0 : 1;
}

int gpio_set_on(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPSET0);
}

int gpio_set_off(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPCLR0);
}

int gpio_set_detect_high(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPHEN0);
}

int gpio_set_detect_low(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPLEN0);
}

int gpio_set_detect_rising_edge(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPREN0);
}

int gpio_set_detect_falling_edge(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPFEN0);
}

int gpio_set_detect_async_rising_edge(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPAREN0);
}

int gpio_set_detect_async_falling_edge(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPAFEN0);
}

int gpio_set_gppudclk(int gpio_num)
{
  GPIO_32PIN_SET_CHEKCED(gpio_num, GPIO_REG_GPPUDCLK0);
}

int gpio_set_pullupdown(int gpio_num, int pullupdown)
{
  register unsigned int r;
  GPIO_CHECK_GPIO_NUM(gpio_num);
  *(reg32_t)GPIO_REG_GPPUD = 0;
  r = 150;
  while(r--) { asm volatile("nop"); } 
  gpio_set_gppudclk(gpio_num);
  r = 150;
  while(r--) { asm volatile("nop"); } 
  *(reg32_t)GPIO_REG_GPPUDCLK0 = 0;
  return 0;
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
  *(reg32_t)GPIO_REG_GPFSEL0 = 0;
  *(reg32_t)GPIO_REG_GPFSEL1 = 0;
  *(reg32_t)GPIO_REG_GPFSEL2 = 0;
  *(reg32_t)GPIO_REG_GPFSEL3 = 0;
  *(reg32_t)GPIO_REG_GPFSEL4 = 0;
  *(reg32_t)GPIO_REG_GPFSEL5 = 0;
  *(reg32_t)GPIO_REG_GPPUD = 0;
  r = 150;
  while(r--) { asm volatile("nop"); } 
  // flush GPIO setup
  *(reg32_t)GPIO_REG_GPPUDCLK0 = 0xffffffff;
  *(reg32_t)GPIO_REG_GPPUDCLK1 = 0xffffffff;
  r = 150;
  while(r--) { asm volatile("nop"); } 
  *(reg32_t)GPIO_REG_GPPUDCLK0 = 0;
}

