#pragma once
#include <types.h>

#define DMA_DREQ_NONE           0
#define DMA_DREQ_DSI            1
#define DMA_DREQ_PCM_TX         2
#define DMA_DREQ_PCM_RX         3
#define DMA_DREQ_SMI            4
#define DMA_DREQ_PWM            5
#define DMA_DREQ_SPI_TX         6
#define DMA_DREQ_SPI_RX         7
#define DMA_DREQ_BSC_SLAVE_TX   8
#define DMA_DREQ_BSC_SLAVE_RX   9
#define DMA_DREQ_UNUSED        10
#define DMA_DREQ_EMMC          11
#define DMA_DREQ_UART_TX       12
#define DMA_DREQ_SD_HOST       13
#define DMA_DREQ_UART_RX       14
#define DMA_DREQ_DSI_B         15
#define DMA_DREQ_SLIMBUS_MCTX  16
#define DMA_DREQ_HDMI          17
#define DMA_DREQ_SLIMBUS_MCRX  18
#define DMA_DREQ_SLIMBUS_DC0   19
#define DMA_DREQ_SLIMBUS_DC1   20
#define DMA_DREQ_SLIMBUS_DC2   21
#define DMA_DREQ_SLIMBUS_DC3   22
#define DMA_DREQ_SLIMBUS_DC4   23
#define DMA_DREQ_SCALER_FIFO_0 24
#define DMA_DREQ_SCALER_FIFO_1 25
#define DMA_DREQ_SCALER_FIFO_2 26
#define DMA_DREQ_SLIMBUS_DC5   27
#define DMA_DREQ_SLIMBUS_DC6   28
#define DMA_DREQ_SLIMBUS_DC7   29
#define DMA_DREQ_SLIMBUS_DC8   30
#define DMA_DREQ_SLIMBUS_DC9   31

/* DMA control block.
 * This is the exact memory structure of control block
 * that will be loaded into BCM DMA controller by writing
 * to DMA_CONBLK_AD register.
 */
struct dma_control_block {
  /* BCM DMA_TI - transfer information */
  uint32_t ti;
  /* source address */
  uint32_t src_addr;
  /* destination address */
  uint32_t dst_addr;
  /* size in bytes of transfer */
  uint32_t transfer_length;
  /* stride for 2d mode */
  uint32_t stride;
  /*
   * address of next dma control block to execute.
   * must be physical address
   */
  uint32_t dma_cb_next;
  /* reserved and should be zero */
  uint32_t res0;
  uint32_t res1;
};

void dma_channel_setup(int channel, void *src, void *dst, int len);

#define DMA_TRANSFER_WIDTH_32BIT  0
#define DMA_TRANSFER_WIDTH_128BIT 1

//#define DMA_TI_INTEN           (1<<0)
//#define DMA_TI_WAIT_RESP       (1<<3)
//#define DMA_TI_DEST_INC        (1<<4)
//#define DMA_TI_DEST_WIDTH_128  (1<<5)
//#define DMA_TI_DEST_DREQ       (1<<6)
//#define DMA_TI_DEST_IGNORE     (1<<7)
//#define DMA_TI_SRC_INC         (1<<8)
//#define DMA_TI_SRC_WIDTH_128   (1<<9)
//#define DMA_TI_SRC_DREQ        (1<<10)
//#define DMA_TI_SRC_IGNORE      (1<<11)
//#define DMA_TI_BURST_LENGTH(x) ((x & 0xf) << 12)
//#define DMA_TI_PERMAP(x)       ((x & 0x1f) << 16)
//#define DMA_TI_WAITS(x)        ((x & 0x1f) << 21)

int dma_enable(int channel);
int dma_reset(int channel);
int dma_set_active(int channel);

typedef struct {
  uint32_t channel;
  uint32_t src;
  uint32_t src_inc;
  uint32_t src_ign;
  uint32_t src_dreq;
  uint32_t dst;
  uint32_t dst_inc;
  uint32_t dst_ign;
  uint32_t dst_dreq;
  uint32_t len;
  uint32_t width_bits;
} dma_ch_opts_t;

int dma_setup(dma_ch_opts_t *o);

void dma_print_debug_info(int channel);

int dma_transfer_is_done(int channel);

int dma_transfer_error(int channel);

int dma_clear_transfer(int channel);

int dma_clear_end_flag(int channel);
