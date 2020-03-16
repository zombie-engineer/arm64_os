#pragma once


#define BSC_SLAVE_MODE_SPI 0
#define BSC_SLAVE_MODE_I2C 1

int bsc_slave_init(int mode, char addr);
int bsc_slave_debug();
