#pragma once
#include <types.h>

#define ATMEGA8A_FUSE_CPU_FREQ_MASK 0xf
#define ATMEGA8A_FUSE_CPU_FREQ_1MHZ 0x1
#define ATMEGA8A_FUSE_CPU_FREQ_2MHZ 0x2
#define ATMEGA8A_FUSE_CPU_FREQ_4MHZ 0x3
#define ATMEGA8A_FUSE_CPU_FREQ_8MHZ 0x4


typedef struct spi_dev spi_dev_t;

int atmega8a_init(spi_dev_t *spidev, int gpio_pin_reset);

int atmega8a_drop_spi();

int atmega8a_deinit();

int atmega8a_reset();

int atmega8a_get_flash_size();

int atmega8a_get_eeprom_size();

int atmega8a_read_flash_memory(void *buf, int sz, int byte_addr);

int atmega8a_read_eeprom_memory(void *buf, int sz, int byte_addr);

int atmega8a_read_fuse_bits(char *out_fuse_low, char *out_fuse_high);

int atmega8a_write_eeprom(const void *buf, int sz, int addr);

int atmega8a_write_flash(const void *buf, int sz, int from_page);

int atmega8a_write_fuse_bits_low(char fuse_low);

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

int atmega8a_spi_master_test();
