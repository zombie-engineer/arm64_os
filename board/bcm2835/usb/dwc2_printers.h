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

static inline int dwc2_print_intsts(uint32_t v, char *buf, int bufsz)
{
  int n = 0;
  int first = 1;
#define __PRINT_BIT(__bit) \
  if (USB_GINTSTS_GET_ ## __bit(v)) {\
    n += snprintf(buf + n, bufsz - n, "%s" #__bit, first ? "" : ",");\
    first = 0;\
  }
  __PRINT_BIT(CURMODE_HOST);
  __PRINT_BIT(MODEMIS);
  __PRINT_BIT(OTGINT);
  __PRINT_BIT(SOF);
  __PRINT_BIT(RXFLVL);
  __PRINT_BIT(NPTXFEMP);
  __PRINT_BIT(GINNAKEFF);
  __PRINT_BIT(GOUTNAKEFF);
  __PRINT_BIT(ULPI_CK_INT);
  __PRINT_BIT(I2CINT);
  __PRINT_BIT(ERLYSUSP);
  __PRINT_BIT(USBSUSP);
  __PRINT_BIT(USBRST);
  __PRINT_BIT(ENUMDONE);
  __PRINT_BIT(ISOUTDROP);
  __PRINT_BIT(EOPF);
  __PRINT_BIT(RESTOREDONE);
  __PRINT_BIT(EPMIS);
  __PRINT_BIT(IEPINT);
  __PRINT_BIT(OEPINT);
  __PRINT_BIT(INCOMPL_SOIN);
  __PRINT_BIT(INCOMPL_SOOUT);
  __PRINT_BIT(FET_SUSP);
  __PRINT_BIT(RESETDET);
  __PRINT_BIT(PRTINT);
  __PRINT_BIT(HCHINT);
  __PRINT_BIT(PTXFEMP);
  __PRINT_BIT(LPMTRANRCVD);
  __PRINT_BIT(CONIDSTSCHNG);
  __PRINT_BIT(DISCONNINT);
  __PRINT_BIT(SESSREQINT);
  __PRINT_BIT(WKUPINT);
#undef __PRINT_BIT
 return n;
}

static inline int dwc2_print_otgint(uint32_t v, char *buf, int bufsz)
{
  int n = 0;
  int first = 1;
#define __PRINT_BIT(__bit) \
  if (USB_GOTGINT_GET_ ## __bit(v)) {\
    n += snprintf(buf + n, bufsz - n, "%s"#__bit, first ? "" : ",");\
    first = 0;\
  }
  __PRINT_BIT(DBNCE_DONE);
  __PRINT_BIT(A_DEV_TOUT_CHG);
  __PRINT_BIT(HST_NEG_DET);
  __PRINT_BIT(HST_NEG_SUC_STS_CHNG);
  __PRINT_BIT(SES_REQ_SUC_STS_CHNG);
  __PRINT_BIT(SES_END_DET);
#undef __PRINT_BIT
  return n;
}

static inline int dwc2_print_port_int(uint32_t v, char *buf, int bufsz)
{
  int n = 0;
  int first = 1;
#define __PRINT_BIT(__bit) \
  if (USB_HOST_INTR_GET_ ## __bit(v)) {\
    n += snprintf(buf + n, bufsz - n, "%s"#__bit, first ? "" : ",");\
    first = 0;\
  }
  __PRINT_BIT(XFER_COMPLETE);
  __PRINT_BIT(HALT);
  __PRINT_BIT(AHB_ERR);
  __PRINT_BIT(STALL);
  __PRINT_BIT(NAK);
  __PRINT_BIT(ACK);
  __PRINT_BIT(NYET);
  __PRINT_BIT(TRNSERR);
  __PRINT_BIT(BABBLERR);
  __PRINT_BIT(FRMOVRN);
  __PRINT_BIT(DATTGGLERR);
  __PRINT_BIT(BUFNOTAVAIL);
  __PRINT_BIT(EXCESSXMIT);
  __PRINT_BIT(FRMLISTROLL);
#undef __PRINT_BIT
  return n;
}
