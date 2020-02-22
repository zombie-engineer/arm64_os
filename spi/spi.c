#include <spi.h>
#include <memory.h>
#include <reg_access.h>
#include <types.h>
#include <common.h>
#include <stringlib.h>
#include <gpio.h>
#include <delays.h>
#include <dma.h>


#define SPI_BASE ((unsigned long long)PERIPHERAL_BASE_PHY + 0x00204000)

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

#define SPI_CS ((reg32_t)SPI_BASE)

#define SPI_FIFO ((reg32_t)(SPI_BASE + 0x4))

typedef struct {
  uint32_t CDIV : 16; // clock divider
  uint32_t RES  : 16; // reserved
} __attribute__ ((packed)) spi_reg_clk_t;

#define SPI_CLK ((reg32_t)(SPI_BASE + 0x8))

#define SPI_DLEN ((reg32_t)(SPI_BASE + 0xc))

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


#define DMA_CHANNEL_SPI_TX 0

#define DMA_CHANNEL_SPI_RX 1

static gpio_role_spi0_t gpio_spi0_role;

static int gpio_roles_initialized = 0;

static spi_spi0_dev_t spi0_dev;
static int spi0_dev_initialized = 0;

static spi_spi1_dev_t spi1_dev;
static int spi1_dev_initialized = 0;

static spi_spi2_dev_t spi2_dev;
static int spi2_dev_initialized = 0;

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
  return ERR_OK;
}

static int spi0_init_dma()
{
  int err;
  RET_IF_ERR(dma_reset, DMA_CHANNEL_SPI_TX);
  RET_IF_ERR(dma_reset, DMA_CHANNEL_SPI_RX);
  RET_IF_ERR(dma_enable, DMA_CHANNEL_SPI_TX);
  RET_IF_ERR(dma_enable, DMA_CHANNEL_SPI_RX);
  return ERR_OK;
}


static int spi0_xmit_dma(const void *data_out, void *data_in, uint32_t bytelen)
{
  uint32_t stub_in, stub_out;
  // printf("spi0_xmit_dma: data_out: %08x\n", data_out);
  dma_reset(DMA_CHANNEL_SPI_TX);
  dma_reset(DMA_CHANNEL_SPI_RX);
  dma_enable(DMA_CHANNEL_SPI_TX);
  dma_enable(DMA_CHANNEL_SPI_RX);

  __sync_synchronize();
  if (data_out == 0 && data_in == 0) {
    puts("spi0_xmit_dma: in and out data buffers are both zero\n");
    return ERR_INVAL_ARG;
  }

  *SPI_CS |= (SPI_CS_CLEAR | SPI_CS_DMAEN | SPI_CS_ADCS);
  stub_out = 0;

  dma_ch_opts_t o = { 0 };

  o.width_bits = DMA_TRANSFER_WIDTH_32BIT;
  o.dst        = PERIPHERAL_PHY_TO_BUS(SPI_FIFO);

  o.channel    = DMA_CHANNEL_SPI_TX;
  o.src_dreq   = DMA_DREQ_NONE;
  o.dst_inc    = 0;
  o.dst_dreq   = DMA_DREQ_SPI_TX;

  if (data_out) {
    // TODO: this is a problem, as the buffer is passed here by
    // a const pointer, so we shouldn't modify it
    ((uint32_t*)data_out)[0] = bytelen << 16 | SPI_CS_TA;
    o.src        = RAM_PHY_TO_BUS_UNCACHED(data_out);
    o.src_inc    = 1;
    o.len        = bytelen + 4;
  } else {
    o.src        = NARROW_PTR(RAM_PHY_TO_BUS_UNCACHED(&stub_out));
    o.src_inc    = 0;
  }


  dma_setup(&o);

  o.channel    = DMA_CHANNEL_SPI_RX;
  o.src        = PERIPHERAL_PHY_TO_BUS(SPI_FIFO);
  o.src_inc    = 0;
  o.len        = bytelen;
  o.src_dreq   = DMA_DREQ_SPI_RX;
  o.dst_dreq   = DMA_DREQ_NONE;

  if (data_in) {
    o.dst        = NARROW_PTR(RAM_PHY_TO_BUS_UNCACHED(data_in));
    o.dst_inc    = 1;
  } else {
    o.dst        = NARROW_PTR(RAM_PHY_TO_BUS_UNCACHED(&stub_in));
    o.dst_inc    = 0;
  }

  dma_setup(&o);
  __sync_synchronize();

  dma_set_active(DMA_CHANNEL_SPI_TX);
  dma_set_active(DMA_CHANNEL_SPI_RX);

  while(!dma_transfer_is_done(DMA_CHANNEL_SPI_TX));
  while(!dma_transfer_is_done(DMA_CHANNEL_SPI_RX));
  dma_clear_transfer(DMA_CHANNEL_SPI_TX);
  dma_clear_transfer(DMA_CHANNEL_SPI_RX);

  // *SPI_CS = 0;
  return ERR_OK;
}


static int spi0_xmit_byte(char byte_in, char *byte_out)
{
  int rx;
  *SPI_CS = SPI_CS_CLEAR;
  *SPI_CS = SPI_CS_TA;
  while(!(*SPI_CS & SPI_CS_TXD));
  while(*SPI_CS & SPI_CS_RXD) rx = *SPI_FIFO;
  if (rx);
  *SPI_FIFO = byte_in;
  while(!(*SPI_CS & SPI_CS_DONE));
  *SPI_CS = 0;
  return ERR_OK;
}

static int spi0_xmit(const char* bytes_in, char *bytes_out, uint32_t len)
{
  int i, rx_data;

  // puts("spi0_xmit started\n");
  *SPI_CS = SPI_CS_CLEAR;
  *SPI_CS = SPI_CS_TA;
  // printf("SPI0 CS: 0x%08x\n", SPI_CS);

  for (i = 0; i < len; ++i) {
    while((*SPI_CS & SPI_CS_TXD) == 0) {
      // printf("SPI_CS->TXD not yet, CS: 0x%08x, TXD: %d\n", SPI_CS, SPI_CS & SPI_CS_TXD);
    }

    while(*SPI_CS & SPI_CS_RXD) {
      rx_data = *SPI_FIFO;
      if (rx_data);
      // printf("RX data: 0x%08x\n", rx_data);
    }

    // printf("spi0_xmit: transmitting: %08x\n", bytes[i]); 
    *SPI_FIFO = bytes_in[i];
    while((*SPI_CS & SPI_CS_DONE) == 0);
    //  puts("SPI_CS->DONE not yet\n");
  } 
  *SPI_CS = 0;
  // puts("spi0_xmit complete\n");
  return ERR_OK;
}

static int spi0_init_dev()
{
  spi0_dev.spidev.xmit      = spi0_xmit;
  spi0_dev.spidev.xmit_byte = spi0_xmit_byte;
  spi0_dev.spidev.xmit_dma  = spi0_xmit_dma;
  spi0_dev_initialized = 1;
  return ERR_OK;
}

int spi0_init()
{
  int err;
  RET_IF_ERR(spi_init_gpio);
  RET_IF_ERR(spi0_init_dev);
  *SPI_CLK = 256;
  *SPI_CS = SPI_CS_CLEAR;
  printf("spi0_init_poll completed: SPI_CS: %08x\n>", *SPI_CS); 
  RET_IF_ERR(spi0_init_dma);
  return ERR_OK;
}


static spi_dev_t *spi0_get_dev() 
{
  return spi0_dev_initialized ? &spi0_dev.spidev : 0;
}


static spi_dev_t *spi1_get_dev() 
{
  return spi1_dev_initialized ? &spi1_dev.spidev : 0;
}


static spi_dev_t *spi2_get_dev() 
{
  return spi2_dev_initialized ? &spi2_dev.spidev : 0;
}


spi_dev_t *spi_get_dev(int type)
{
  switch(type) {
    case SPI_TYPE_SPI0:     return spi0_get_dev();
    case SPI_TYPE_SPI1:     return spi1_get_dev();
    case SPI_TYPE_SPI2:     return spi2_get_dev();
    case SPI_TYPE_EMULATED: return spi_emulated_get_dev();
  }

  return 0;
}


int spi_type_from_string(const char *string, int len)
{
  if (!strncmp("spi0", string, len))
    return SPI_TYPE_SPI0;
  if (!strncmp("spi1", string, len))
    return SPI_TYPE_SPI1;
  if (!strncmp("spi2", string, len))
    return SPI_TYPE_SPI2;
  if (!strncmp("spi_emulated", string, len) || !strncmp("emulated", string, len))
    return SPI_TYPE_EMULATED;
  return SPI_TYPE_UNKNOWN;
}
