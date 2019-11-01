#include <spi.h>
#include <memory.h>
#include <reg_access.h>
#include <types.h>
#include <common.h>
#include <gpio.h>
#include <delays.h>


#define SPI_BASE ((unsigned long long)MMIO_BASE + 0x00204000)

typedef struct {
  spi_dev_t spidev;
} spi_spi0_dev_t;

typedef struct {
  spi_dev_t spidev;
} spi_spi1_dev_t;

typedef struct {
  spi_dev_t spidev;
} spi_spi2_dev_t;

// Description of SPI0 CS reg at SPI0_BASE + 0x00
// 1:0 Chip Select (00, 01, 10 - chips 0, 1, 2. 11 -reserved)
// 2   Clock Phase
// 3   Clock Polarity
// 5:4 CLEAR FIFO clear
// 6   Chip Select Polarity
// 7   Transfer active
// 8   DMAEN DMA Enable
// 9   INTD Interrupt on Done
// 10  INTR Interrupt on RXR
// 11  ADCS Auto Deassert Chip select
// 12  REN Read Enable
// 13  LEN LoSSI enable
// 14  Unused
// 15  Unused
// 16  Done transfer Done
// 17  RX FIFO contains Data
// 18  TX FIFO can accept Data
// 19  RX FIFO needs Reading (full)
// 20  FX FIFO Full
// 21  Chip Select 0 Polarity
// 22  Chip Select 1 Polarity
// 23  Chip Select 2 Polarity
// 24  Enable DMA mode in Lossi mode
// 25  Enable Long data word in Lossi mode if DMA_LEN set
// 31:26 Write as 0

#define SPI_CS_CS       (3 << 0)
#define SPI_CS_CPHA     (1 << 2)
#define SPI_CS_CPOL     (1 << 3)
#define SPI_CS_CLEAR    (3 << 4)
#define SPI_CS_CSPOL    (1 << 6)
#define SPI_CS_TA       (1 << 7)
#define SPI_CS_DMAEN    (1 << 8)
#define SPI_CS_INTD     (1 << 9)
#define SPI_CS_INTR     (1 << 10)
#define SPI_CS_ADCS     (1 << 11)
#define SPI_CS_REN      (1 << 12)
#define SPI_CS_LEN      (1 << 13)
#define SPI_CS_DONE     (1 << 16)
#define SPI_CS_RXD      (1 << 17)
#define SPI_CS_TXD      (1 << 18)
#define SPI_CS_RXR      (1 << 19)
#define SPI_CS_RXF      (1 << 20)
#define SPI_CS_CSPOL0   (1 << 21)
#define SPI_CS_CSPOL1   (1 << 22)
#define SPI_CS_CSPOL2   (1 << 23)
#define SPI_CS_DMA_LEN  (1 << 24)
#define SPI_CS_LEN_LONG (1 << 25)

#define SPI_CS (*(reg32_t)SPI_BASE)

#define SPI_FIFO (*(reg32_t)(SPI_BASE + 0x4))

typedef struct {
  uint32_t CDIV : 16; // clock divider
  uint32_t RES  : 16; // reserved
} __attribute__ ((packed)) spi_reg_clk_t;

#define SPI_CLK (*(reg32_t)(SPI_BASE + 0x8))

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

static spi_spi2_dev_t spi2_dev;
static int spi2_dev_initialized = 0;

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
//  gpio_set_function(11, GPIO_FUNC_ALT_0);
//  gpio_set_function(10, GPIO_FUNC_ALT_0);
//  gpio_set_function(9 , GPIO_FUNC_ALT_0);
//  gpio_set_function(8 , GPIO_FUNC_ALT_0);
//  gpio_set_function(7 , GPIO_FUNC_ALT_0);
//  return 0;
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
  int i, rx_data;

  puts("spi0_xmit started\n");

   
  SPI_CS = SPI_CS_CLEAR;
  SPI_CS = SPI_CS_TA;
  printf("SPI0 CS: 0x%08x\n", SPI_CS);

  for (i = 0; i < len; ++i) {
    while((SPI_CS & SPI_CS_TXD) == 0) {
      printf("SPI_CS->TXD not yet, CS: 0x%08x, TXD: %d\n", SPI_CS, SPI_CS & SPI_CS_TXD);
    }

    while(SPI_CS & SPI_CS_RXD) {
      rx_data = SPI_FIFO;
      printf("RX data: 0x%08x\n", rx_data);
    }

    printf("spi0_xmit: transmitting: %08x\n", bytes[i]); 
    SPI_FIFO = bytes[i];
    while((SPI_CS & SPI_CS_DONE) == 0)
      puts("SPI_CS->DONE not yet\n");
  } 
  SPI_CS = 0;
  puts("spi0_xmit complete\n");
  return 0;
}

static int spi0_init_dev()
{
  spi0_dev.spidev.xmit      = spi0_xmit;
  spi0_dev.spidev.push_bit  = spi0_push_bit;
  spi0_dev.spidev.ce0_set   = spi0_ce0_set;
  spi0_dev.spidev.ce0_clear = spi0_ce0_clear;
  spi0_dev_initialized = 1;
  return 0;
}

static int spi0_init_poll()
{
  puts("spi0_init_poll\n");
  spi_init_gpio();
  spi0_init_dev();
  SPI_CS = SPI_CS_CLEAR;
  printf("spi0_init_poll completed: SPI_CS: %08x\n>", SPI_CS); 
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

spi_dev_t *spi2_get_dev() 
{
  return spi2_dev_initialized ? &spi2_dev.spidev : 0;
}
