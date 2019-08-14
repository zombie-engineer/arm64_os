#pragma once

#define MMIO_BASE       0x3F000000
#define GPIO_BASE       (unsigned long)(MMIO_BASE + 0x00200000)

#define GPFSEL0         *(volatile unsigned int*)(GPIO_BASE + 0x00)
#define GPFSEL1         *(volatile unsigned int*)(GPIO_BASE + 0x04)
#define GPFSEL2         *(volatile unsigned int*)(GPIO_BASE + 0x08)
#define GPFSEL3         *(volatile unsigned int*)(GPIO_BASE + 0x0C)
#define GPFSEL4         *(volatile unsigned int*)(GPIO_BASE + 0x10)
#define GPFSEL5         *(volatile unsigned int*)(GPIO_BASE + 0x14)
#define GPSET0          *(volatile unsigned int*)(GPIO_BASE + 0x1C)
#define GPSET1          *(volatile unsigned int*)(GPIO_BASE + 0x20)
#define GPCLR0          *(volatile unsigned int*)(GPIO_BASE + 0x28)
#define GPLEV0          *(volatile unsigned int*)(GPIO_BASE + 0x34)
#define GPLEV1          *(volatile unsigned int*)(GPIO_BASE + 0x38)
#define GPEDS0          *(volatile unsigned int*)(GPIO_BASE + 0x40)
#define GPEDS1          *(volatile unsigned int*)(GPIO_BASE + 0x44)
#define GPHEN0          *(volatile unsigned int*)(GPIO_BASE + 0x64)
#define GPHEN1          *(volatile unsigned int*)(GPIO_BASE + 0x68)
#define GPLEN0          *(volatile unsigned int*)(GPIO_BASE + 0x70)
#define GPLEN1          *(volatile unsigned int*)(GPIO_BASE + 0x74)
#define GPPAFEN0        *(volatile unsigned int*)(GPIO_BASE + 0x88)
#define GPPAFEN1        *(volatile unsigned int*)(GPIO_BASE + 0x8c)
#define GPPUD           *(volatile unsigned int*)(GPIO_BASE + 0x94)
#define GPPUDCLK0       *(volatile unsigned int*)(GPIO_BASE + 0x98)
#define GPPUDCLK1       *(volatile unsigned int*)(GPIO_BASE + 0x9C)

#define GPIO_FUNC_IN  0
#define GPIO_FUNC_OUT 1

static inline volatile unsigned int * gpio_pin_select_reg(int idx)
{
  return (volatile unsigned int*)(GPIO_BASE + 4 * (idx / 10));
}

static inline unsigned int gpio_pin_select_bit(int idx)
{
  return (idx % 10) * 3;
}

static inline volatile unsigned int * gpio_pin_set_reg(int idx)
{
  return (volatile unsigned int*)(GPIO_BASE + 0x1c * (idx / 32));
}

static inline unsigned int gpio_pin_set_bit(int idx)
{
  return idx % 32;
}

static inline volatile unsigned int * gpio_pin_clear_reg(int idx)
{
  return (volatile unsigned int*)(GPIO_BASE + 0x28 * (idx / 32));
}

static inline unsigned int gpio_pin_clear_bit(int idx)
{
  return idx % 32;
}

int gpio_set_function(int gpio_num, int func);

int gpio_set_on(int gpio_num);

int gpio_set_off(int gpio_num);

int gpio_set_detect_high(int gpio_num);

void gpio_dump_select_regs(const char* tag);
