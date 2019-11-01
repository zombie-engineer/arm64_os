#pragma once
#include <types.h>

// SPI operation mode poll
#define SPI_TYPE_POLL 0

// SPI operation mode interrupts 
#define SPI_TYPE_INT  1

// SPI operation mode DMA
#define SPI_TYPE_DMA  2

// spi_enable success
#define SPI_ERR_OK               0

// spi_enable error arguments invalid
#define SPI_ERR_INVALID         -1

// spi_enable error not implemented
#define SPI_ERR_UNIMPLEMENTED   -2

// spi not initialized
#define SPI_ERR_NOT_INITIALIZED -3

// spi0 controller
#define SPI_TYPE_SPI0         0

// spi1 controller
#define SPI_TYPE_SPI1         1

// emulated spi with manual bit banging of gpio pins.
#define SPI_TYPE_SPI_EMULATED 2

typedef struct spi_dev {
  int (*xmit)(char* bytes, uint32_t len);
} spi_dev_t;

int spi0_init(int type);

int spi_emulated_init(
  int sclk_pin, 
  int mosi_pin, 
  int miso_pin, 
  int ce0_pin,
  int ce1_pin);


void spi_emulated_print_info();

spi_dev_t *spi_emulated_get_dev();

spi_dev_t *spi0_get_dev();

spi_dev_t *spi1_get_dev();

spi_dev_t *spi2_get_dev();
