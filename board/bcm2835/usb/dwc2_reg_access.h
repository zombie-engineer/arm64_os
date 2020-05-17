#include <reg_access.h>
#include "dwc2_log.h"

static inline const char *reg_to_string(reg32_t reg)
{
  switch(reg - USB_HCCHAR0) {
    case USB_HCINT0    - USB_HCCHAR0 : return "INTR";
    case USB_HCINTMSK0 - USB_HCCHAR0 : return "INMS";
    case USB_HCSPLT0   - USB_HCCHAR0 : return "SPLT";
    case USB_HCCHAR0   - USB_HCCHAR0 : return "CHAR";
    case USB_HCTSIZ0   - USB_HCCHAR0 : return "SIZE";
    case USB_HCDMA0    - USB_HCCHAR0 : return "DMA ";
    default            : return "UNKN";
  }
}

static inline uint32_t __read_ch_reg(reg32_t reg, int chan)
{
  uint32_t val = read_reg(reg + chan * 8);
  DWCDEBUG2("__xfer:rd:%s:%08x:%08x", reg_to_string(reg), reg, val);
  return val;
}

static inline void __write_ch_reg(reg32_t reg, int chan, uint32_t val)
{
  write_reg(reg + chan * 8, val);
  DWCDEBUG2("__xfer:wr:%s:%08x:%08x", reg_to_string(reg), reg, chan);
}

#define CLEAR_INTR()    __write_ch_reg(USB_HCINT0   , ch, 0xffffffff)
#define CLEAR_INTRMSK() __write_ch_reg(USB_HCINTMSK0, ch, 0x00000000)
#define SET_SPLT()      __write_ch_reg(USB_HCSPLT0  , ch, splt)
#define SET_CHAR()      __write_ch_reg(USB_HCCHAR0  , ch, chr)
#define SET_SIZ()       __write_ch_reg(USB_HCTSIZ0  , ch, siz)
#define SET_DMA()       __write_ch_reg(USB_HCDMA0   , ch, dma)

#define GET_CHAR()      chr     = __read_ch_reg(USB_HCCHAR0  , ch)
#define GET_SIZ()       siz     = __read_ch_reg(USB_HCTSIZ0  , ch)
#define GET_SPLT()      splt    = __read_ch_reg(USB_HCSPLT0  , ch)
#define GET_INTR()      intr    = __read_ch_reg(USB_HCINT0   , ch)
#define GET_INTRMSK()   intrmsk = __read_ch_reg(USB_HCINTMSK0, ch)
#define GET_DMA()       dma     = __read_ch_reg(USB_HCDMA0   , ch)

