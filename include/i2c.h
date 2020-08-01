#pragma once

int i2c_init(void);

int i2c_write(uint8_t i2c_addr, const char *buf, int bufsz);

int i2c_read(uint8_t i2c_addr, const char *buf, int bufsz);
