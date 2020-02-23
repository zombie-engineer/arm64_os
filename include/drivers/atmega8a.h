#pragma once
#include <types.h>

typedef struct spi_dev spi_dev_t;

int atmega8a_init(spi_dev_t *spidev, int gpio_pin_reset);

int atmega8a_read_program_memory(void *buf, uint32_t addr, size_t sz);
