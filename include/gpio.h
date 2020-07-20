#pragma once
#include <memory.h>
#include <reg_access.h>
#include <types.h>

/*
 * GPIO OUT outputs 3.3 volts
 */

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

void gpio_read_and_set_64(reg32_t addr, uint32_t gpio_num);
void gpio_read_and_set_3(reg32_t addr, uint32_t gpio_num, uint32_t mode);
bool gpio_is_set(int gpio_num);
uint32_t gpio_read_level_reg(int gpio_num);
