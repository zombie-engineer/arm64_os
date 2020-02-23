#pragma once
#include <types.h>

int atmega8a_init(int gpio_pin_miso, int gpio_pin_mosi, int gpio_pin_sclk, int gpio_pin_reset);

int atmega8a_read_program_memory(void *buf, uint32_t addr, size_t sz);
