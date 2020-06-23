#include "dwc2.h"
#include <types.h>
#include <reg_access.h>
#include <stringlib.h>
#include <bits_api.h>
#include "dwc2_regs.h"
#include "dwc2_regs_bits.h"
#include "dwc2_reg_access.h"
#include "dwc2_printers.h"
#include <usb/usb.h>
#include <spinlock.h>

static port_status_changed_cb_t port_status_changed_cb = NULL;

/*
 * DWC2 USB interrupt model.
 * USB_GAHBCFG.GLBL_INTR_EN should be enabled to channel all local interrupts to interrupt controller chip.
 * USB_GINTMSK - should be written to 0xffffffff or something more precise to unmask interrupt.
 * Set bit 9 to 0x3f00b210 "Enable IRQs 1" register in bcm2835_interrupt_controller.
 *
 * There multiple interrupt domens channeled to USB_GINTSTS
 * OTGINT - interrupts related to USB-OTG protocol - like session requests etc, check USB_OTGCTL for status
 * PRTINT - port interrupts, check USB_HPRT for statuses that trigger it, like connection detection, reset change, enabled, etc
 * HCHINT - host channel interrupts, if this is triggered, then USB_HAINT is also triggered, HAINT in turn - is a bitmask of
 * N channels each having it's own bit to indicate which channel exactly has triggered interrupt. So HAINT = 0x00000020 has
 * interrupts pending on channel 5 (1<<5 = 0x20). HAINT is coupled with HAINTMSK register which is used for masking out
 * exactl channels. HAINTMSK = 0x000000ff - means that any events on channels 0 to 7 will trigger interrupt.
 * HAINTMSK = 0x00000002 means only channel 1 will trigger interrupts.
 * For interrupt to propagate to HAINT register yuo should set up per-channel interrupt registers USB_HCINTn and USB_HCINTMSKn,
 * where 'n' is the channel index.
 * Channel interrupt summary:
 *   USB_HCINTn/USB_HCINTMSKn->USB_HAINT/USB_HAINTMSK->USB_GINTSTS/USB_GINTMSK
 *   USB_HCINTn - per-channel interrupt register
 *   USB_HCINTMSK - per-channel interrupt mask/unmask register
 *   USB_HAINT - bitmask, where bit represent number of channel, bit=1 - channel is the source of interrupt, bit=0 channel is silent.
 *   USB_HAINTMSK - mask away all interrupts from the channel you don't want to be triggered.
 *
 * Monitor 0x3f00b204 "IRQ pending 1" and 0x3f00b200 "IRQ basic pending".
 * "IRQ Basic pending" register will expose bit 9 (gpu1 interrupt bank pending).
 * "IRQ pending 1" register will expose bit 9 (USB irq bit).
 * Enable IRQ bit via function enable_irq() or "msr DAIFSet, #2"
 *
 * Overall interrupt hierarch summary:
 *
 *                                                   ARM CPU IRQ/FIQ
 *                                                            \
 *                                                            | \ enable by "msr daifset, #2" 2 = IRQ
 *                                                            | \ disable by "msr daifclr, #2"
 *                                                            |
 *                                                            |
 *                                                  BCM2835_INTERRUPT_CONTROLLER
 *                                                    | | | | | | | | | | | | | \
 *                                                        |                      \
 *                                                        |                       \ enable by write to IRQ1_enable/IRQ2_enable regs
 *                                                        |                        \ monitor by read from irq_basic_pending/irq1_pending/irq2_pending regs
 *                                                        |
 *                                                        |
 *                                                  USB_AHB_CONFIG.GLOBAL_INT_ENABLE bit
 *                                                        |                            \
 *                                                        |                             \ set bit 0 in USB_AHBCFG to enable interrupts from USB
 *                                                        |
 *                                                  USB_GINTSTS/USB_GINTMSK
 *                           OTG        _________/  /     |             \   \
 *                         interrupts /            /      |              \    \ mask/unmask/monitor port / channel / otg / core interrupts here
 *                        USB_GOTGCTL             /       |               \
 *                                          core        port           channel
 *                                        interrups   interrupts     interrupts
 *                                     (USB_GINTSTS)  (USB_HPRT)   (USB_HAINT/USB_HAINTMSK)
 *                                                                        |               \
 *                                                                        |                \ mask/unmask/monitor which channels are source of interrupts
 *                                                                        |
 *                                                                /   /   |   \    \
 *                                                              ch0 ch1 ch2  ch3  ch..X
 *                                                             /               \
 *                                                        (USB_HCINT0/) ..   ..(USB_HCINT3/)
 *                                                       (USB_HCINTMSK)   ... (USB_HCINTMSK3)
 *                                                                                           \
 *                                                                                            \ mask/unmask/monitor per-channel interrupts here
 *
 */

void dwc2_dump_port_int(int port)
{
  uint32_t portval;
  char port_desc[512];
  portval = read_reg(USB_HCINT0 + port * 0x20);
  dwc2_print_port_int(portval, port_desc, sizeof(port_desc));
  DWCINFO("port_int:%08x(%s)", portval, port_desc);
}

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
  USB_GAHBCFG_CLR_SET_DMA_EN(ahb, 1);
  USB_GAHBCFG_CLR_AHB_SINGLE(ahb);
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
  USB_GAHBCFG_CLR_SET_GLBL_INTR_EN(ahb, 1);
  printf("enabling global ahb interrupts: %08x<-%08x" __endline, USB_GAHBCFG, ahb);
  write_reg(USB_GAHBCFG, ahb);
}

void dwc2_disable_ahb_interrupts(void)
{
  uint32_t ahb = read_reg(USB_GAHBCFG);
  USB_GAHBCFG_CLR_GLBL_INTR_EN(ahb);
  printf("disabling global ahb interrupts: %08x<-%08x" __endline, USB_GAHBCFG, ahb);
  write_reg(USB_GAHBCFG, ahb);
}

void dwc2_enable_interrupts(void)
{
  write_reg(USB_GINTMSK, 0);
}

void dwc2_dump_int_registers(void)
{
  uint32_t intsts, otgint;
  char intsts_desc[512];
  char otgint_desc[512];
  intsts = read_reg(USB_GINTSTS);
  otgint = read_reg(USB_GOTGINT);
  dwc2_print_intsts(intsts, intsts_desc, sizeof(intsts_desc));
  dwc2_print_otgint(otgint, otgint_desc, sizeof(otgint_desc));
  printf("otg:%08x(%s), intsts:%08x(%s)" __endline, otgint, otgint_desc, intsts, intsts_desc);
}
