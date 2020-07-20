#include <reg_access.h>
#include "dwc2_log.h"
#include <stringlib.h>

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
typedef int (*reg_printer)(char *, int, uint32_t);

static inline void reg_printer_simple(char *buf, int bufsz, uint32_t val)
{
  snprintf(buf, bufsz, "%08x", val);
}

static inline void print_reg(reg32_t reg, uint32_t val, reg_printer p, int is_write_op)
{
  if (dwc2_get_log_level() > 1) {
    char val_string[256];
    if (p)
      p(val_string, sizeof(val_string), val);
    else
      reg_printer_simple(val_string, sizeof(val_string), val);

    DWCDEBUG2("__xfer:%s:%s:%08x:%s",
      is_write_op ? "wr" : "rd",
      reg_to_string(reg),
      reg,
      val_string);
  }
}

static inline uint32_t __read_ch_reg(reg32_t reg, int chan, reg_printer p)
{
  uint32_t val = read_reg(reg + chan * 8);
  print_reg(reg, val, p, 0);
  return val;
}

static inline void __write_ch_reg(reg32_t reg, int chan, uint32_t val, reg_printer p)
{
  write_reg(reg + chan * 8, val);
  print_reg(reg, val, p, 1);
}

#define CLEAR_INTR()    __write_ch_reg(USB_HCINT0   , ch_id, 0xffffffff, usb_host_intr_to_string)
#define CLEAR_INTRMSK() __write_ch_reg(USB_HCINTMSK0, ch_id, 0x00000000, usb_host_intr_to_string)
#define SET_INTR()      __write_ch_reg(USB_HCINT0   , ch_id ,intr, usb_host_intr_to_string)
#define SET_INTRMSK()   __write_ch_reg(USB_HCINTMSK0, ch_id ,intrmsk, usb_host_intr_to_string)
#define SET_SPLT()      __write_ch_reg(USB_HCSPLT0  , ch_id, splt, usb_host_splt_to_string)
#define SET_CHAR()      __write_ch_reg(USB_HCCHAR0  , ch_id, chr, usb_host_char_to_string)
#define SET_SIZ()       __write_ch_reg(USB_HCTSIZ0  , ch_id, siz, usb_host_size_to_string)
#define SET_DMA()       __write_ch_reg(USB_HCDMA0   , ch_id, dma, NULL)

#define GET_CHAR()      chr     = __read_ch_reg(USB_HCCHAR0  , ch_id, usb_host_char_to_string)
#define GET_SIZ()       siz     = __read_ch_reg(USB_HCTSIZ0  , ch_id, usb_host_size_to_string)
#define GET_SPLT()      splt    = __read_ch_reg(USB_HCSPLT0  , ch_id, usb_host_splt_to_string)
#define GET_INTR()      intr    = __read_ch_reg(USB_HCINT0   , ch_id, usb_host_intr_to_string)
#define GET_INTRMSK()   intrmsk = __read_ch_reg(USB_HCINTMSK0, ch_id, usb_host_intr_to_string)
#define GET_DMA()       dma     = __read_ch_reg(USB_HCDMA0   , ch_id, NULL)

