#include <spi.h>
#include <memory.h>
#include <reg_access.h>
#include <types.h>
#include <common.h>
#include <gpio.h>


#define SPI_BASE ((unsigned long long)MMIO_BASE + 0x00204000)

typedef struct {
  spi_dev_t spidev;
} spi_spi0_dev_t;

typedef struct {
  spi_dev_t spidev;
} spi_spi1_dev_t;

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


typedef struct {
  struct {
    int num;
    int fun;
  } pin[5];
} gpio_role_spi0_t;

static gpio_role_spi0_t gpio_spi0_role;

static int gpio_roles_initialized = 0;

static spi_spi0_dev_t spi0_dev;
static int spi0_dev_initialized = 0;

static spi_spi1_dev_t spi1_dev;
static int spi1_dev_initialized = 0;


static int spi0_init_dma()
{
  return SPI_ERR_UNIMPLEMENTED;
}


static int spi0_init_int()
{
  return SPI_ERR_UNIMPLEMENTED;
}


static gpio_role_spi0_t *gpio_get_role_spi0()
{
  if (!gpio_roles_initialized) {
    gpio_spi0_role.pin[0].num = 7;
    gpio_spi0_role.pin[0].fun = GPIO_FUNC_ALT_0;
    gpio_spi0_role.pin[1].num = 8;
    gpio_spi0_role.pin[1].fun = GPIO_FUNC_ALT_0;
    gpio_spi0_role.pin[2].num = 9;
    gpio_spi0_role.pin[2].fun = GPIO_FUNC_ALT_0;
    gpio_spi0_role.pin[3].num = 10;
    gpio_spi0_role.pin[3].fun = GPIO_FUNC_ALT_0;
    gpio_spi0_role.pin[4].num = 11;
    gpio_spi0_role.pin[4].fun = GPIO_FUNC_ALT_0;

    gpio_roles_initialized = 1;
  }
  return &gpio_spi0_role;
}


static int spi_init_gpio()
{
  int i;
  gpio_role_spi0_t *role;
  role = gpio_get_role_spi0();
  for (i = 0; i < 5; ++i)
    gpio_set_function(role->pin[i].num, role->pin[i].fun);
  return 0;
}


static int spi0_push_bit(uint8_t value)
{
  return 0;
}


static int spi0_ce0_set()
{
  return 0;
}


static int spi0_ce0_clear()
{
  return 0;
}

static int spi0_xmit(char* bytes, uint32_t len)
{
  puts("spi0_xmit started\n");
  SPI_CS->TA = 1;
  while(len--) {
    while(!SPI_CS->TXD)
      puts("SPI_CS->TXD\n");
    printf("spi0_xmit: transmitting: %02x\n", *bytes);
    SPI_FIFO->DATA = *(bytes++);
  } 
  SPI_CS->TA = 0;
  puts("spi0_xmit complete\n");
}

static int spi0_init_dev()
{
  spi0_dev.spidev.xmit      = spi0_xmit;
  spi0_dev.spidev.push_bit  = spi0_push_bit;
  spi0_dev.spidev.ce0_set   = spi0_ce0_set;
  spi0_dev.spidev.ce0_clear = spi0_ce0_clear;
  spi0_dev_initialized = 1;
}

static int spi0_init_poll()
{
  spi_init_gpio();

  SPI_CS->CPOL = 0;
  SPI_CS->CPHA = 0;
  SPI_CS->TA = 1;

  printf("<%08x\n>", *(reg32_t)SPI_CS); 
  return SPI_ERR_OK;
}


int spi0_init(int type)
{
  switch (type) {
    case SPI_TYPE_POLL : return spi0_init_poll();
    case SPI_TYPE_INT  : return spi0_init_int();
    case SPI_TYPE_DMA  : return spi0_init_dma();
  }
  return SPI_ERR_INVALID;
}


spi_dev_t *spi0_get_dev() 
{
  return spi0_dev_initialized ? &spi0_dev.spidev : 0;
}


spi_dev_t *spi1_get_dev() 
{
  return spi1_dev_initialized ? &spi1_dev.spidev : 0;
}
