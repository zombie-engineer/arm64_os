#include "dwc2.h"
#include <types.h>
#include <reg_access.h>
#include <stringlib.h>
#include <bits_api.h>
#include "dwc2_regs.h"
#include "dwc2_regs_bits.h"
#include "dwc2_reg_access.h"
#include "dwc2_printers.h"

void dwc2_start_vbus(void)
{
  uint32_t ctl;
  ctl  = read_reg(USB_GUSBCFG);
  BIT_CLEAR_U32(ctl, USB_GUSBCFG_ULPI_EXT_VBUS_DRV);
  BIT_CLEAR_U32(ctl, USB_GUSBCFG_TERM_SEL_DL_PULSE);
  write_reg(USB_GUSBCFG, ctl);
}

void dwc2_reset(void)
{
  uint32_t rst;
  do {
    rst = read_reg(USB_GRSTCTL);
  } while(!USB_GRSTCTL_GET_AHB_IDLE(rst));

  USB_GRSTCTL_CLR_SET_C_SFT_RST(rst, 1)
  write_reg(USB_GRSTCTL, rst);

  do {
    rst = read_reg(USB_GRSTCTL);
  } while(!USB_GRSTCTL_GET_AHB_IDLE(rst) || USB_GRSTCTL_GET_C_SFT_RST(rst));
  DWCINFO("reset done.");
}


void dwc2_set_ulpi_no_phy(void)
{
  uint32_t ctl = read_reg(USB_GUSBCFG);
  /* Set mode UTMI */
  BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_UTMI_SEL);
  /* Disable PHY */
  BIT_CLEAR_U32(ctl, USB_GUSBCFG_PHY_IF);
  write_reg(USB_GUSBCFG, ctl);
}

uint32_t dwc2_get_vendor_id(void)
{
 return read_reg(USB_GSNPSID);
}

uint32_t dwc2_get_user_id(void)
{
  return read_reg(USB_GUID);
}

void dwc2_print_core_regs(void)
{
  char buf[1024];
  dwc2_get_core_reg_description(buf, sizeof(buf));
  DWCINFO("core registers:"__endline"%s",buf);
}

dwc2_hs_iface dwc2_get_hs_iface(void)
{
  uint32_t hwcfg2 = read_reg(USB_GHWCFG2);
  return USB_GHWCFG2_GET_HSPHY_INTERFACE(hwcfg2);
}

dwc2_fs_iface dwc2_get_fs_iface(void)
{
  uint32_t hwcfg2 = read_reg(USB_GHWCFG2);
  return USB_GHWCFG2_GET_FSPHY_INTERFACE(hwcfg2);
}

void dwc2_set_fsls_config(int enabled)
{
  uint32_t ctl = read_reg(USB_GUSBCFG);

  if (enabled) {
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_FS_LS);
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_CLK_SUS_M);
  } else {
    BIT_CLEAR_U32(ctl, USB_GUSBCFG_ULPI_FS_LS);
    BIT_CLEAR_U32(ctl, USB_GUSBCFG_ULPI_CLK_SUS_M);
  }
  write_reg(USB_GUSBCFG, ctl);
}

void dwc2_set_dma_mode(void)
{
  uint32_t ahb = read_reg(USB_GAHBCFG);
  BIT_SET_U32(ahb, USB_GAHBCFG_DMA_EN);
  BIT_CLEAR_U32(ahb, USB_GAHBCFG_DMA_REM_MODE);
  write_reg(USB_GAHBCFG, ahb);
}

dwc2_op_mode_t dwc2_get_op_mode(void)
{
  uint32_t hwcfg2 = read_reg(USB_GHWCFG2);
  return USB_GHWCFG2_GET_MODE(hwcfg2);
}

void dwc2_set_otg_cap(dwc2_otg_cap_t cap)
{
  uint32_t ctl = read_reg(USB_GUSBCFG);
  switch(cap) {
    case DWC2_OTG_CAP_NONE:
      BIT_CLEAR_U32(ctl, USB_GUSBCFG_HNP_CAP);
      BIT_CLEAR_U32(ctl, USB_GUSBCFG_SRP_CAP);
      break;
    case DWC2_OTG_CAP_SRP:
      BIT_CLEAR_U32(ctl, USB_GUSBCFG_HNP_CAP);
      BIT_SET_U32(ctl, USB_GUSBCFG_SRP_CAP);
      break;
    case DWC2_OTG_CAP_HNP_SRP:
      BIT_SET_U32(ctl, USB_GUSBCFG_HNP_CAP);
      BIT_SET_U32(ctl, USB_GUSBCFG_SRP_CAP);
      break;
  }
  write_reg(USB_GUSBCFG, ctl);
}

void dwc2_power_clock_off(void)
{
  write_reg(USB_PCGCR, 0);
}

bool dwc2_is_ulpi_fs_ls_only(void)
{
  uint32_t ctl = read_reg(USB_GUSBCFG);
  return BIT_IS_SET(ctl, USB_GUSBCFG_ULPI_FS_LS) ? true : false;
}

void dwc2_set_host_speed(dwc2_clk_speed_t speed)
{
  uint32_t hostcfg = read_reg(USB_HCFG);
  USB_HCFG_CLR_SET_LS_PHY_CLK_SEL(hostcfg, speed);
  write_reg(USB_HCFG, hostcfg);
}

void dwc2_set_host_ls_support(void)
{
  uint32_t hostcfg = read_reg(USB_HCFG);
  USB_HCFG_CLR_SET_LS_SUPP(hostcfg, 1);
  write_reg(USB_HCFG, hostcfg);
}

void dwc2_setup_fifo_sizes(int rx_fifo_sz, int non_periodic_fifo_sz, int periodic_fifo_sz)
{
  write_reg(USB_GRXFSIZ  , rx_fifo_sz);
  write_reg(USB_GNPTXFSIZ, (rx_fifo_sz<<16)|non_periodic_fifo_sz);
  write_reg(USB_HPTXFSIZ, (periodic_fifo_sz<<16)|(rx_fifo_sz + non_periodic_fifo_sz));
}

void dwc2_set_otg_hnp(void)
{
  uint32_t otg = read_reg(USB_GOTGCTL);
  BIT_SET_U32(otg, USB_GOTGCTL_HST_SET_HNP_EN);
  write_reg(USB_GOTGCTL, otg);
}

void dwc2_tx_fifo_flush(int fifonum)
{
  uint32_t rst = read_reg(USB_GRSTCTL);
  USB_GRSTCTL_CLR_SET_TXF_NUM(rst, fifonum);
  USB_GRSTCTL_CLR_SET_TXF_FLSH(rst, 1);
  write_reg(USB_GRSTCTL, rst);
  do {
    rst = read_reg(USB_GRSTCTL);
  } while(USB_GRSTCTL_GET_TXF_FLSH(rst));
}

void dwc2_rx_fifo_flush(void)
{
  uint32_t rst = read_reg(USB_GRSTCTL);
  USB_GRSTCTL_CLR_SET_RXF_FLSH(rst, 1);
  write_reg(USB_GRSTCTL, rst);
  do {
    rst = read_reg(USB_GRSTCTL);
  } while(USB_GRSTCTL_GET_RXF_FLSH(rst));
}

bool dwc2_is_dma_enabled(void)
{
  uint32_t hostcfg = read_reg(USB_HCFG);
  return USB_HCFG_GET_DMA_DESC_ENA(hostcfg) ? true : false;
}

bool dwc2_is_port_pwr_enabled(void)
{
  uint32_t hostport = read_reg(USB_HPRT);
  return USB_HPRT_GET_PWR(hostport) ? true : false;
}

void dwc2_port_set_pwr_enabled(bool enabled)
{
  uint32_t hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_WRITE_MASK;
  USB_HPRT_CLR_SET_PWR(hostport, enabled);
  write_reg(USB_HPRT, hostport);
}

void dwc2_port_reset(void)
{
  uint32_t hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_WRITE_MASK;
  USB_HPRT_CLR_SET_RST(hostport, 1);
  write_reg(USB_HPRT, hostport);
}

void dwc2_port_reset_clear(void)
{
  uint32_t hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_WRITE_MASK;
  USB_HPRT_CLR_RST(hostport);
  write_reg(USB_HPRT, hostport);
}

void dwc2_enable_ahb_interrupts(void)
{
  uint32_t ahb = read_reg(USB_GAHBCFG);
  BIT_SET_U32(ahb, USB_GAHBCFG_GLBL_INTR_MSK);
  write_reg(USB_GAHBCFG, ahb);
}

void dwc2_disable_ahb_interrupts(void)
{
  uint32_t ahb = read_reg(USB_GAHBCFG);
  BIT_CLEAR_U32(ahb, USB_GAHBCFG_GLBL_INTR_MSK);
  write_reg(USB_GAHBCFG, ahb);
}

void dwc2_enable_interrupts(void)
{
  write_reg(USB_GINTMSK, 0);
}

void dwc2_dump_int_registers(void)
{
  uint32_t intsts, otgint;
  intsts = read_reg(USB_GINTSTS);
  otgint = read_reg(USB_GOTGINT);
  printf("intsts: %08x, otg: %08x" __endline, intsts, otgint);
}
