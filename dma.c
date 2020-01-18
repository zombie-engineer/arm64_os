#include <dma.h>
#include <memory.h>
#include <error.h>
#include <common.h>
#include <reg_access.h>
#include <delays.h>


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


static __attribute__((aligned(32))) dma_cb_t dma_channels[16];

int dma_set_transfer_width(int channel, int width) 
{
  int ti = *DMA_TI(channel);
  if (width == DMA_TRANSFER_WIDTH_32BIT) {
    ti &= ~DMA_TI_SRC_WIDTH;
    ti &= ~DMA_TI_DEST_WIDTH;
  } else if (width == DMA_TRANSFER_WIDTH_128BIT) {
    ti |= DMA_TI_SRC_WIDTH;
    ti |= DMA_TI_DEST_WIDTH;
  } else 
    return ERR_INVAL_ARG;

  *DMA_TI(channel) = ti;
  return ERR_OK;
}

int dma_set_control_block(int channel, dma_cb_t *cb)
{
  *DMA_CONBLK_AD(channel) = RAM_PHY_TO_BUS_UNCACHED(cb);
  return ERR_OK;
}

int dma_set_active(int channel)
{
  int cs;
  cs = *DMA_CS(channel);
  cs |= DMA_CS_ACTIVE;
  cs |= DMA_CS_WAIT_FOR_OUTSTANDING_WRITES;
  *DMA_CS(channel) = cs;
  return ERR_OK;
}

//static void cache_clean_and_inval(uint64_t address, uint64_t length)
//{
//  length += 64;
//  while(1) {
//    asm volatile ("dc civac, %0" : : "r" (address) : "memory");
//    if (length < 64)
//      break;
//    address += 64;
//    length -= 64;
//  }
//  asm volatile ("dsb sy" ::: "memory");
//}

int dma_enable(int channel)
{
  *DMA_ENABLE |= (1 << channel);
  return ERR_OK;
}

int dma_reset(int channel)
{
  *DMA_CS(channel) |= DMA_CS_RESET;
  return ERR_OK;
}

int dma_setup(dma_ch_opts_t *o) 
{
  dma_cb_t *cb = &dma_channels[o->channel];

  cb->ti = DMA_TI_PERMAP(((o->src_dreq) | (o->dst_dreq)))
         | (o->src_inc  ? DMA_TI_SRC_INC   : 0)
         | (o->src_ign  ? DMA_TI_DEST_IGNORE : 0)
         | (o->src_dreq != DMA_DREQ_NONE ? DMA_TI_SRC_DREQ : 0)
         | (o->dst_inc  ? DMA_TI_DEST_INC  : 0)
         | (o->dst_ign  ? DMA_TI_DEST_IGNORE : 0)
         | (o->dst_dreq != DMA_DREQ_NONE ? DMA_TI_DEST_DREQ : 0)
         | DMA_TI_NO_WIDE_BURSTS
         | DMA_TI_WAIT_RESP;

  cb->src_addr        = o->src;
  cb->dst_addr        = o->dst;
  cb->transfer_length = o->len;
  cb->stride          = 0;
  cb->dma_cb_next     = 0;
  cb->res0            = 0;
  cb->res1            = 0;

  dma_set_control_block(o->channel, cb);
  return ERR_OK;
}

int dma_clear_end_flag(int channel)
{
  int dma_cs;
  dma_cs = *DMA_CS(channel);
  dma_cs |= DMA_CS_END;
  *DMA_CS(channel) = dma_cs;
  return ERR_OK;
}

void dma_print_debug_info(int channel)
{
  int cs, ti, source_ad, dest_ad, conblk_ad, txfr_len, debug, enable;
  cs = *DMA_CS(channel);
  ti = *DMA_TI(channel);
  source_ad = *DMA_SOURCE_AD(channel);
  dest_ad = *DMA_DEST_AD(channel);
  conblk_ad = *DMA_CONBLK_AD(channel);
  txfr_len = *DMA_TXFR_LEN(channel);
  debug = *DMA_DEBUG(channel);
  enable = *DMA_ENABLE;

  printf("DMA_CS#%d         %08x\n", channel, cs);
  printf("DMA_TI#%d         %08x\n", channel, ti);
  printf("DMA_CONBLK_AD#%d  %08x\n", channel, conblk_ad);
  printf("DMA_SOURCE_AD#%d  %08x\n", channel, source_ad);
  printf("DMA_DEST_AD#%d    %08x\n", channel, dest_ad);
  printf("DMA_TXFR_LEN#%d   %08x\n", channel, txfr_len);
  printf("DMA_DEBUG#%d      %08x\n", channel, debug);
  printf("DMA_ENABLE%c      %08x\n", ' ', enable);
}

int dma_transfer_is_done(int channel)
{
  // printf("dma_transfer_is_done: DMA_CS: %08x\n", *DMA_CS(channel));
  return *DMA_CS(channel) & DMA_CS_END;
}

int dma_transfer_error(int channel)
{
  return *DMA_CS(channel) & DMA_CS_ERROR;
}

int dma_clear_transfer(int channel)
{
  int dma_cs;
  dma_cs = *DMA_CS(channel);
  if (dma_cs & DMA_CS_END)
    dma_cs |= DMA_CS_END;
  if (dma_cs & DMA_CS_INT)
    dma_cs |= DMA_CS_INT;
  *DMA_CS(channel) = dma_cs;
  return ERR_OK;
}
