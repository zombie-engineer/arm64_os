#pragma once

// SPI operation mode poll
#define SPI_TYPE_POLL 0

// SPI operation mode interrupts 
#define SPI_TYPE_INT  1

// SPI operation mode DMA
#define SPI_TYPE_DMA  2

// spi_enable success
#define SPI_ENA_ERR_OK             0

// spi_enable error arguments invalid
#define SPI_ENA_ERR_INVALID       -1

// spi_enable error not implemented
#define SPI_ENA_ERR_UNIMPLEMENTED -2

int spi_enable(int type);
