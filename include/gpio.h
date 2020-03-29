#pragma once
#include <memory.h>
#include <reg_access.h>
#include <types.h>

/* 
 * GPIO OUT outputs 3.3 volts
 */

#define GPIO_BASE       (uint64_t)(PERIPHERAL_BASE_PHY + 0x200000)

#define GPIO_REG_GPFSEL0   (GPIO_BASE + 0x00)
#define GPIO_REG_GPFSEL1   (GPIO_BASE + 0x04)
#define GPIO_REG_GPFSEL2   (GPIO_BASE + 0x08)
#define GPIO_REG_GPFSEL3   (GPIO_BASE + 0x0C)
#define GPIO_REG_GPFSEL4   (GPIO_BASE + 0x10)
#define GPIO_REG_GPFSEL5   (GPIO_BASE + 0x14)

#define GPIO_REG_GPSET0    (GPIO_BASE + 0x1C)
#define GPIO_REG_GPSET1    (GPIO_BASE + 0x20)

#define GPIO_REG_GPCLR0    (GPIO_BASE + 0x28)
#define GPIO_REG_GPCLR1    (GPIO_BASE + 0x2c)

#define GPIO_REG_GPLEV0    (GPIO_BASE + 0x34)
#define GPIO_REG_GPLEV1    (GPIO_BASE + 0x38)

#define GPIO_REG_GPEDS0    (GPIO_BASE + 0x40)
#define GPIO_REG_GPEDS1    (GPIO_BASE + 0x44)

#define GPIO_REG_GPREN0    (GPIO_BASE + 0x4c)
#define GPIO_REG_GPREN1    (GPIO_BASE + 0x50)

#define GPIO_REG_GPFEN0    (GPIO_BASE + 0x58)
#define GPIO_REG_GPFEN1    (GPIO_BASE + 0x5c)

#define GPIO_REG_GPHEN0    (GPIO_BASE + 0x64)
#define GPIO_REG_GPHEN1    (GPIO_BASE + 0x68)

#define GPIO_REG_GPLEN0    (GPIO_BASE + 0x70)
#define GPIO_REG_GPLEN1    (GPIO_BASE + 0x74)

#define GPIO_REG_GPAREN0  (GPIO_BASE + 0x7c)
#define GPIO_REG_GPAREN1  (GPIO_BASE + 0x80)

#define GPIO_REG_GPAFEN0  (GPIO_BASE + 0x88)
#define GPIO_REG_GPAFEN1  (GPIO_BASE + 0x8c)

#define GPIO_REG_GPPUD     (GPIO_BASE + 0x94)
#define GPIO_REG_GPPUDCLK0 (GPIO_BASE + 0x98)
#define GPIO_REG_GPPUDCLK1 (GPIO_BASE + 0x9C)

#define GPIO_MAX_PIN_IDX 53

#define GPIO_FUNC_IN     0b000
#define GPIO_FUNC_OUT    0b001
#define GPIO_FUNC_ALT_0  0b100 
#define GPIO_FUNC_ALT_1  0b101 
#define GPIO_FUNC_ALT_2  0b110 
#define GPIO_FUNC_ALT_3  0b111 
#define GPIO_FUNC_ALT_4  0b011 
#define GPIO_FUNC_ALT_5  0b010 

#define GPIO_PULLUPDOWN_NO_PULLUPDOWN 0b00
#define GPIO_PULLUPDOWN_EN_PULLDOWN   0b01
#define GPIO_PULLUPDOWN_EN_PULLUP     0b10

int gpio_is_set(uint32_t gpio_num);

int gpio_set_function(uint32_t gpio_num, int func);

int gpio_set_on(uint32_t gpio_num);

int gpio_set_off(uint32_t gpio_num);

int gpio_set_detect_high(uint32_t gpio_num);

int gpio_set_detect_low(uint32_t gpio_num);

int gpio_set_detect_rising_edge(uint32_t gpio_num);

int gpio_set_detect_falling_edge(uint32_t gpio_num);

int gpio_set_detect_async_rising_edge(uint32_t gpio_num);

int gpio_set_detect_async_falling_edge(uint32_t gpio_num);

int gpio_pin_status_triggered(uint32_t gpio_num);

int gpio_pin_status_clear(uint32_t gpio_num);

int gpio_set_gppudclk(uint32_t gpio_num);

int gpio_set_pullupdown(uint32_t gpio_num, int pullupdown);

void gpio_dump_select_regs(const char* tag);

void gpio_power_off(void);

void gpio_read_and_set_64(uint32_t *addr, uint32_t gpio_num);
void gpio_read_and_set_3(uint32_t *addr, uint32_t gpio_num, uint32_t mode);
