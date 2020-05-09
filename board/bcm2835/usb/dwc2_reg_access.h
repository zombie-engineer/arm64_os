#include <reg_access.h>

static inline uint32_t __read_ch_reg(reg32_t reg, int chan)
{
  return read_reg(reg + chan * 8);
}

static inline void __write_ch_reg(reg32_t reg, int chan, uint32_t val)
{
  write_reg(reg + chan * 8, val);
}

#define CLEAR_INTR()    __write_ch_reg(USB_HCINT0   , ch, 0xffffffff)
#define CLEAR_INTRMSK() __write_ch_reg(USB_HCINTMSK0, ch, 0x00000000)
#define SET_SPLT()      __write_ch_reg(USB_HCSPLT0  , ch, splt); DWCDEBUG2("transfer:splt:ch:%d,%08x->%p", ch, splt, USB_HCSPLT0 + ch * 8);
#define SET_CHAR()      __write_ch_reg(USB_HCCHAR0  , ch, chr);  DWCDEBUG2("transfer:char:ch:%d,%08x->%p", ch, chr , USB_HCCHAR0 + ch * 8);
#define SET_SIZ()       __write_ch_reg(USB_HCTSIZ0  , ch, siz);  DWCDEBUG2("transfer:size:ch:%d,%08x->%p", ch, siz , USB_HCTSIZ0 + ch * 8);
#define SET_DMA()       __write_ch_reg(USB_HCDMA0   , ch, dma);  DWCDEBUG2("transfer:dma :ch:%d,%08x->%p", ch, dma , USB_HCDMA0  + ch * 8);

#define GET_CHAR()      chr     = __read_ch_reg(USB_HCCHAR0  , ch); DWCDEBUG2("transfer:read_chac :%d,%p->%08x", ch, USB_HCCHAR0 + ch * 8, chr);
#define GET_SIZ()       siz     = __read_ch_reg(USB_HCTSIZ0  , ch)
#define GET_SPLT()      splt    = __read_ch_reg(USB_HCSPLT0  , ch); DWCDEBUG2("transfer:read_splt:%d,%p->%08x", ch, USB_HCSPLT0 + ch * 8, splt);
#define GET_INTR()      intr    = __read_ch_reg(USB_HCINT0   , ch); DWCDEBUG2("transfer:read_intr:%p->%08x", USB_HCINT0 + ch * 8, intr);
#define GET_INTRMSK()   intrmsk = __read_ch_reg(USB_HCINTMSK0, ch)
#define GET_DMA()       dma     = __read_ch_reg(USB_HCDMA0   , ch)

