#pragma once
#include <types.h>

#define DMA_DREQ_1              0
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

typedef struct {
  // transfer information
  uint32_t ti;
  // 
  uint32_t src_addr;
  // 
  uint32_t dst_addr;
  uint32_t transfer_length;
  uint32_t stride;
  // next block
  uint32_t dma_cb_next;
  uint32_t res0;
  uint32_t res1;
} dma_cb_t;

void dma_channel_setup(int channel, void *src, void *dst, int len);

#define DMA_TRANSFER_WIDTH_32BIT  0
#define DMA_TRANSFER_WIDTH_128BIT 1

int dma_enable(int channel);
int dma_reset(int channel);
int dma_set_active(int channel);

typedef struct {
  uint32_t channel;
  uint32_t src;
  uint32_t src_inc;
  uint32_t src_dreq;
  uint32_t dst;
  uint32_t dst_inc;
  uint32_t dst_dreq;
  uint32_t len;
  uint32_t width_bits;
} dma_ch_opts_t;

int dma_setup(dma_ch_opts_t *o);

void dma_print_debug_info(int channel);
