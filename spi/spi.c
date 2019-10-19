#include <spi.h>
#include <memory.h>
#include <reg_access.h>
#include <types.h>
#include <common.h>

#define SPI_BASE ((unsigned long long)MMIO_BASE + 0x00204000)


typedef struct {
  char CS       : 2; // 1:0 Chip Select (00, 01, 10 - chips 0, 1, 2. 11 -reserved)
  char CPHA     : 1; // 2   Clock Phase
  char CPOL     : 1; // 3   Clock Polarity
  char CLEAR    : 2; // 5:4 CLEAR FIFO clear
  char CSPOL    : 1; // 6   Chip Select Polarity
  char TA       : 1; // 7   Transfer active
  char DMAEN    : 1; // 8   DMAEN DMA Enable
  char INTD     : 1; // 9   INTD Interrupt on Done
  char INTR     : 1; // 10  INTR Interrupt on RXR
  char ADCS     : 1; // 11  ADCS Auto Deassert Chip select
  char REN      : 1; // 12  REN Read Enable
  char LEN      : 1; // 13  LEN LoSSI enable
  char LMONO    : 1; // 14  Unused
  char TE_EN    : 1; // 15  Unused
  char DONE     : 1; // 16  Done transfer Done
  char RXD      : 1; // 17  RX FIFO contains Data
  char TXD      : 1; // 18  TX FIFO can accept Data
  char RXR      : 1; // 19  RX FIFO needs Reading (full)
  char RXF      : 1; // 20  FX FIFO Full
  char CSPOL0   : 1; // 21  Chip Select 0 Polarity
  char CSPOL1   : 1; // 22  Chip Select 1 Polarity
  char CSPOL2   : 1; // 23  Chip Select 2 Polarity
  char DMA_LEN  : 1; // 24  Enable DMA mode in Lossi mode
  char LEN_LONG : 1; // 25  Enable Long data word in Lossi mode if DMA_LEN set
  char RES      : 6; // 31:26 Write as 0
} __attribute__ ((packed)) spi_reg_cs_t;

#define SPI_CS ((spi_reg_cs_t *)SPI_BASE)

typedef struct {
  uint32_t DATA; // data register
} __attribute__ ((packed)) spi_reg_fifo_t;

#define SPI_FIFO ((spi_reg_fifo_t*)(SPI_BASE + 0x4))

typedef struct {
  uint32_t CDIV : 16; // clock divider
  uint32_t RES  : 16; // reserved
} __attribute__ ((packed)) spi_reg_clk_t;

#define SPI_CLK ((spi_reg_clk_t*)(SPI_BASE + 0x8))

typedef struct {
  
} __attribute__ ((packed)) spi_reg_dlen_t;

typedef struct {
  
} __attribute__ ((packed)) spi_reg_ltoh_t;

typedef struct {
  
} __attribute__ ((packed)) spi_reg_dc_t;


int spi_enable_dma()
{
  return SPI_ERR_UNIMPLEMENTED;
}


int spi_enable_int()
{
  return SPI_ERR_UNIMPLEMENTED;
}


int spi_enable_poll()
{
  int i;
  SPI_CS->CPOL = 0;
  SPI_CS->CPHA = 0;
  SPI_CS->TA = 1;

  printf("<%08x\n>", *(reg32_t)SPI_CS); 
  for (i = 0; i < 8; ++i) {
    if (SPI_CS->TXD) {
      SPI_FIFO->DATA = 0xffffffff;
    }
  }

  return SPI_ERR_OK;
}


int spi_enable(int type)
{
  switch (type) {
    case SPI_TYPE_POLL : return spi_enable_poll();
    case SPI_TYPE_INT  : return spi_enable_int();
    case SPI_TYPE_DMA  : return spi_enable_dma();
  }
  return SPI_ERR_INVALID;
}

int spi0_get_dev() 
{
  return 0;
}

int spi1_get_dev() 
{
  return 0;
}
