#pragma once
#include <reg_access.h>

#define DMA_BASE (unsigned long long)(PERIPHERAL_BASE_PHY + 0x00007000)
#define DMA_CH_BASE(channel)               (DMA_BASE + channel * 0x100)
#define DMA_CS(channel)          (reg32_t)(DMA_CH_BASE(channel) + 0x00)
#define DMA_CONBLK_AD(channel)   (reg32_t)(DMA_CH_BASE(channel) + 0x04)
#define DMA_TI(channel)          (reg32_t)(DMA_CH_BASE(channel) + 0x08)
#define DMA_SOURCE_AD(channel)   (reg32_t)(DMA_CH_BASE(channel) + 0x0c)
#define DMA_DEST_AD(channel)     (reg32_t)(DMA_CH_BASE(channel) + 0x10)
#define DMA_TXFR_LEN(channel)    (reg32_t)(DMA_CH_BASE(channel) + 0x14)
#define DMA_STRIDE(channel)      (reg32_t)(DMA_CH_BASE(channel) + 0x18)
#define DMA_NEXT_CONBLK(channel) (reg32_t)(DMA_CH_BASE(channel) + 0x1c)
#define DMA_DEBUG(channel)       (reg32_t)(DMA_CH_BASE(channel) + 0x20)
#define DMA_ENABLE               (reg32_t)(DMA_BASE + 0xff0)


#define DMA_CS_ACTIVE                         (1 <<  0)
#define DMA_CS_END                            (1 <<  1)
#define DMA_CS_INT                            (1 <<  2)
#define DMA_CS_DREQ                           (1 <<  3)
#define DMA_CS_PAUSED                         (1 <<  4)
#define DMA_CS_DREQ_STOPS_DMA                 (1 <<  5)
#define DMA_CS_WAITING_FOR_OUTSTANDING_WRITES (1 <<  6)
#define DMA_CS_ERROR                          (1 <<  8)
#define DMA_CS_PRIORITY(x)            ((x & 0xf) << 16)
#define DMA_CS_PANIC_PRIORITY(x)      ((x & 0xf) << 20)
#define DMA_CS_WAIT_FOR_OUTSTANDING_WRITES    (1 << 28)
#define DMA_CS_DISDEBUG                       (1 << 29)
#define DMA_CS_ABORT                          (1 << 30)
#define DMA_CS_RESET                          (1 << 31)


#define DMA_TI_INTEN                   (1 <<  0)
#define DMA_TI_TD_MODE                 (1 <<  1)
#define DMA_TI_WAIT_RESP               (1 <<  3)
#define DMA_TI_DEST_INC                (1 <<  4)
#define DMA_TI_DEST_WIDTH              (1 <<  5)
#define DMA_TI_DEST_DREQ               (1 <<  6)
#define DMA_TI_DEST_IGNORE             (1 <<  7)
#define DMA_TI_SRC_INC                 (1 <<  8)
#define DMA_TI_SRC_WIDTH               (1 <<  9)
#define DMA_TI_SRC_DREQ                (1 << 10)
#define DMA_TI_SRC_IGNORE              (1 << 11)
#define DMA_TI_BURST_LENGTH   ((x & 0x0f) << 12)
#define DMA_TI_PERMAP(x)      ((x & 0x1f) << 16)
#define DMA_TI_WAITS(x)       ((x & 0x1f) << 25)
#define DMA_TI_NO_WIDE_BURSTS          (1 << 26)


#define DMA_TXFR_LEN_XLENGTH(x) ((x & 0xffff) <<  0)
#define DMA_TXFR_LEN_YLENGTH(x) ((x & 0x3fff) << 16)


#define DMA_STRIDE_D_STRIDE(x) ((x & 0xffff) << 16)
#define DMA_STRIDE_S_STRIDE(x) ((x & 0xffff) <<  0)


#define DMA_DEBUG_READ_LAST_NOT_SET_ERROR (1 <<  0)
#define DMA_DEBUG_FIFO_ERROR              (1 <<  1)
#define DMA_DEBUG_READ_ERROR              (1 <<  2)
#define DMA_DEBUG_OUTSTANDING_WRITES    (0xf <<  4)
#define DMA_DEBUG_DMA_ID            (0xffff  <<  8)
#define DMA_DEBUG_DMA_STATE         (0x1ffff << 16)
#define DMA_DEBUG_VERSION                 (7 << 27)
#define DMA_DEBUG_LITE                    (1 << 28)


typedef enum {
  DMA_TI_DREQ_T_NONE = 0,
  DMA_TI_DREQ_T_SRC,
  DMA_TI_DREQ_T_DST
} dma_ti_dreq_type_t;

typedef enum {
  DMA_TI_ADDR_TYPE_IGNOR = 0,
  DMA_TI_ADDR_TYPE_INC_Y,
  DMA_TI_ADDR_TYPE_INC_N
} dma_addr_type_t;

static inline uint32_t dma_make_ti_value(
  int dreq,
  dma_ti_dreq_type_t dreq_type,
  dma_addr_type_t src_addr_type,
  dma_addr_type_t dst_addr_type,
  bool wait_resp)
{
  uint32_t value = 0;

  switch(src_addr_type) {
    case DMA_TI_ADDR_TYPE_IGNOR: value |= DMA_TI_SRC_IGNORE; break;
    case DMA_TI_ADDR_TYPE_INC_Y: value |= DMA_TI_SRC_INC   ; break;
    default                    : break;
  }

  switch(dst_addr_type) {
    case DMA_TI_ADDR_TYPE_IGNOR: value |= DMA_TI_DEST_IGNORE; break;
    case DMA_TI_ADDR_TYPE_INC_Y: value |= DMA_TI_DEST_INC   ; break;
    default                    : break;
  }
  
  switch (dreq_type) {
    case DMA_TI_DREQ_T_SRC : value |= DMA_TI_SRC_DREQ; break;
    case DMA_TI_DREQ_T_DST: value |= DMA_TI_DEST_DREQ; break;
    default                : break;                   
  }

  value |= DMA_TI_PERMAP(dreq);
  value |= DMA_TI_NO_WIDE_BURSTS;
  if (wait_resp)
    value |= DMA_TI_WAIT_RESP;

  return value;
}

