#pragma once
#include <types.h>

typedef struct spi_dev spi_dev_t;

int atmega8a_init(spi_dev_t *spidev, int gpio_pin_reset);

int atmega8a_get_flash_size();

int atmega8a_get_eeprom_size();

int atmega8a_read_flash_memory(void *buf, int sz, int byte_addr);

int atmega8a_read_eeprom_memory(void *buf, int sz, int byte_addr);

int atmega8a_read_fuse_bits(char *out_fuse_low, char *out_fuse_high);

int atmega8a_write_eeprom(const void *buf, int sz, int addr);

int atmega8a_write_flash(const void *buf, int sz, int from_page);

int atmega8a_chip_erase();

int atmega8a_read_lock_bits(char *out_lock_bits);

int atmega8a_write_lock_bits(char lock_bits);

/*
 * Give string description to lock bits
 * buf: write string to this buffer.
 * bufsz: do not exceed this size of buffer.
 * lock_bits: lock bits to describe
 */
int atmega8a_lock_bits_describe(char *buf, int bufsz, char lock_bits);
