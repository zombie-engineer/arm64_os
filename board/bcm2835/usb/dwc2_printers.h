#pragma once
#include <reg_access.h>
#include <stringlib.h>
#include "dwc2_regs.h"
#include "dwc2_regs_bits.h"

static inline void dwc2_get_core_reg_description(char *buf, int bufsz)
{
  int n = 0;
#define PRINT_REG(__r) \
  n += snprintf(buf + n, bufsz - n, "  "#__r":%08x"__endline, read_reg(__r))
  PRINT_REG(USB_GOTGCTL);
  PRINT_REG(USB_GOTGINT);
  PRINT_REG(USB_GAHBCFG);
  PRINT_REG(USB_GUSBCFG);
  PRINT_REG(USB_GRSTCTL);
  PRINT_REG(USB_GINTSTS);
  PRINT_REG(USB_GINTMSK);
  PRINT_REG(USB_GRXSTSR);
  PRINT_REG(USB_GRXSTSP);
  PRINT_REG(USB_GRXFSIZ);
  PRINT_REG(USB_GNPTXFSIZ);
  PRINT_REG(USB_GNPTXSTS);
  PRINT_REG(USB_GI2CCTL);
  PRINT_REG(USB_GPVNDCTL);
  PRINT_REG(USB_GGPIO);
  PRINT_REG(USB_GUID);
  PRINT_REG(USB_GSNPSID);
  PRINT_REG(USB_GHWCFG1);
  PRINT_REG(USB_GHWCFG2);
  PRINT_REG(USB_GHWCFG3);
  PRINT_REG(USB_GHWCFG4);
  PRINT_REG(USB_GLPMCFG);
  PRINT_REG(USB_GAXIDEV);
  PRINT_REG(USB_GMDIOCSR);
  PRINT_REG(USB_GMDIOGEN);
  PRINT_REG(USB_GVBUSDRV);
  PRINT_REG(USB_HPTXFSIZ);
}
