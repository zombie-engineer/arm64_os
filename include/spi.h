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

// spi2 controller
#define SPI_TYPE_SPI2         2

// emulated spi with manual bit banging of gpio pins.
#define SPI_TYPE_EMULATED     3

// for error signalling
#define SPI_TYPE_UNKNOWN   0xff

#define SPIDEV_MAXNAMELEN 15
typedef struct spi_dev {
  int (*xmit)(struct spi_dev *d, const char* bytes_in, char *bytes_out, uint32_t len);
  int (*xmit_byte)(struct spi_dev *d, char byte_in, char *byte_out);
  int (*xmit_dma)(struct spi_dev *d, const void *data_out, void *data_in, uint32_t len);
  char name[SPIDEV_MAXNAMELEN + 1];
} spi_dev_t;

int spi0_init();

#define SPI_EMU_MODE_MASTER 0
#define SPI_EMU_MODE_SLAVE 1

spi_dev_t *spi_allocate_emulated(
  const char *name,
  int sclk_pin, 
  int mosi_pin, 
  int miso_pin, 
  int ce0_pin,
  int ce1_pin,
  int mode);


int spi_deallocate_emulated(spi_dev_t *s);

void spi_emulated_init(void);

void spi_emulated_set_clk(spi_dev_t *s, int val);

void spi_emulated_set_log_level(int val);

void spi_emulated_print_info();

int spi_type_from_string(const char *string, int len);

spi_dev_t *spi_get_dev(int type);
