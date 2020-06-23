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
  DWCINFO("resetting hprt: %08x", hostport);
  write_reg(USB_HPRT, hostport);
}

void dwc2_enable_ahb_interrupts(void)
{
  uint32_t ahb = read_reg(USB_GAHBCFG);
  USB_GAHBCFG_CLR_SET_GLBL_INTR_EN(ahb, 1);
  DWCINFO("enabling global ahb interrupts: %08x<-%08x", USB_GAHBCFG, ahb);
  write_reg(USB_GAHBCFG, ahb);
}

void dwc2_unmask_all_interrupts(void)
{
  int mask = 0xffffffff;
  USB_GINTSTS_CLR_CURMODE_HOST(mask);
  // USB_GINTSTS_CLR_RXFLVL(mask);
  USB_GINTSTS_CLR_NPTXFEMP(mask);
  USB_GINTSTS_CLR_PTXFEMP(mask);
  USB_GINTSTS_CLR_SOF(mask);
  write_reg(USB_GINTMSK, mask);
}

void dwc2_enable_host_interrupts(void)
{
  uint32_t intmsk;
  write_reg(USB_HAINTMSK, 0);
  write_reg(USB_HAINT, 0xffffffff);
  intmsk = read_reg(USB_GINTMSK);
  USB_GINTSTS_CLR_SET_HCHINT(intmsk, 1);
  write_reg(USB_GINTMSK, intmsk);
}

void dwc2_unmask_port_interrupts(void)
{
  uint32_t reg = 0;
  USB_GINTSTS_CLR_SET_PRTINT(reg, 1);
  write_reg(USB_GINTMSK, reg);

}

void dwc2_clear_all_interrupts(void)
{
  write_reg(USB_GINTSTS, 0xffffffff);
}

void dwc2_disable_ahb_interrupts(void)
{
  uint32_t ahb = read_reg(USB_GAHBCFG);
  USB_GAHBCFG_CLR_GLBL_INTR_EN(ahb);
  DWCINFO("disabling global ahb interrupts: %08x<-%08x", USB_GAHBCFG, ahb);
  write_reg(USB_GAHBCFG, ahb);
}


void dwc2_port_reset_clear(void)
{
  uint32_t hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_WRITE_MASK;
  USB_HPRT_CLR_RST(hostport);
  DWCINFO("reset clear: %08x", hostport);
  write_reg(USB_HPRT, hostport);
}

void dwc2_dump_internal_status_reg(void)
{
  char desc[512];
  uint32_t regval = read_reg(USB_GINTSTS);
  dwc2_print_intsts(regval, desc, sizeof(desc));
}

void dwc2_dump_int_registers(void)
{
  // uint32_t intsts, otgint;
  // char otgint_desc[512];
  // otgint = read_reg(USB_GOTGINT);
  // dwc2_print_otgint(otgint, otgint_desc, sizeof(otgint_desc));
  // DWCINFO("otg:%08x(%s)", otgint, otgint_desc);
  DWCINFO("intsts:*%08x=%08x,intmsk:%08x,haint:*%08x=%08x,haintmsk:%08x,hprt:%08x",
    USB_GINTSTS, read_reg(USB_GINTSTS),
    read_reg(USB_GINTMSK),
    USB_HAINT, read_reg(USB_HAINT),
    read_reg(USB_HAINTMSK),
    read_reg(USB_HPRT)
  );
}

static inline void dwc2_irq_handle_sof(void)
{
  uint32_t reg = 0;
  USB_GINTSTS_CLR_SET_SOF(reg, 1);
	write_reg(USB_GINTSTS, reg);
  DWCINFO("handling sof, reg = %08x", reg);
}

static inline void dwc2_hprt_to_port_status(uint32_t hprt, struct usb_hub_port_status *s)
{
  s->status.raw         = 0;
  s->status.connected   = USB_HPRT_GET_CONN_STS(hprt);
  s->status.enabled     = USB_HPRT_GET_ENA(hprt);
  s->status.suspended   = USB_HPRT_GET_SUSP(hprt);
  s->status.overcurrent = USB_HPRT_GET_OVR_CURR_ACT(hprt);
  s->status.reset       = USB_HPRT_GET_RST(hprt);
  s->status.power       = USB_HPRT_GET_PWR(hprt);
  s->status.low_speed   = USB_HPRT_GET_SPD(hprt) == USB_SPEED_LOW  ? 1 : 0;
  s->status.high_speed  = USB_HPRT_GET_SPD(hprt) == USB_SPEED_HIGH ? 1 : 0;

  s->change.raw                 = 0;
  s->change.connected_changed   = USB_HPRT_GET_CONN_DET(hprt);
  s->change.enabled_changed     = USB_HPRT_GET_EN_CHNG(hprt);
  s->change.overcurrent_changed = USB_HPRT_GET_OVR_CURR_CHNG(hprt);
}

static inline void dwc2_irq_handle_port_intr(void)
{
  uint32_t hprt = read_reg(USB_HPRT);
  uint32_t msk = hprt;
  USB_HPRT_CLR_ENA(msk);
  USB_HPRT_CLR_CONN_DET(msk);
  USB_HPRT_CLR_EN_CHNG(msk);
  USB_HPRT_CLR_OVR_CURR_CHNG(msk);

  DWCINFO("port interrupt, hprt:%08x", hprt);
  if (USB_HPRT_GET_CONN_DET(hprt)) {
    DWCINFO("CONN_DET");
    if (USB_HPRT_GET_RST(hprt)) {
      printf("reset bit");
    }
    USB_HPRT_CLR_SET_CONN_DET(msk, 1);
    write_reg(USB_HPRT, msk);
  }
  if (USB_HPRT_GET_EN_CHNG(hprt)) {
    DWCINFO("ENA_CHNG");
    USB_HPRT_CLR_SET_EN_CHNG(msk, 1);
    write_reg(USB_HPRT, msk);
  }
  if (USB_HPRT_GET_OVR_CURR_CHNG(hprt)) {
    DWCINFO("OVR_CURR_CHNG");
    USB_HPRT_CLR_SET_OVR_CURR_CHNG(msk, 1);
    write_reg(USB_HPRT, msk);
  }
  if (port_status_changed_cb) {
    struct usb_hub_port_status status ALIGNED(4);
    dwc2_hprt_to_port_status(hprt, &status);
    port_status_changed_cb(&status);
  }
}

void dwc2_irq_handle_rx_fifo_lvl_intr()
{
  uint32_t grxsts = read_reg(USB_GRXSTSP);
  int channel_idx = USB_GRXSTSP_GET_CHNUM(grxsts);
  int num_bytes = USB_GRXSTSP_GET_BYTECNT(grxsts);
  int dpid = USB_GRXSTSP_GET_DPID(grxsts);
  int pkt = USB_GRXSTSP_GET_PKTSTS(grxsts);
  DWCINFO("rx_fifo intr, grxsts:%08x, num_chan:%d, numbytes:%d,dpid:%d,pkt:%d", grxsts, channel_idx,num_bytes,dpid,pkt);
}

void dwc2_irq_handle_otgint()
{
  uint32_t otgint, otgctl;
  otgint = read_reg(USB_GOTGINT);
  otgctl = read_reg(USB_GOTGCTL);
  DWCINFO("otgint:%08x,otgctl:%08x", otgint, otgctl);
  if (USB_GOTGINT_GET_DBNCE_DONE(otgint))
    DWCINFO("debounce done");
  write_reg(USB_GOTGINT, otgint);
}

void dwc2_irq_handle_mode_mismatch(void)
{
  uint32_t reg = 0;
  DWCINFO("mode mismatch, clearing...");
  USB_GINTSTS_CLR_SET_MODEMIS(reg, 1);
  write_reg(USB_GINTSTS, reg);
}

static inline void dwc2_irq_handle_early_susp(void)
{
  uint32_t reg = 0;
  DWCINFO("early suspend, clearing...");
  USB_GINTSTS_CLR_SET_ERLYSUSP(reg, 1);
  write_reg(USB_GINTSTS, reg);
}

static inline void dwc2_irq_handle_usb_susp(void)
{
  uint32_t reg = 0;
  DWCINFO("usb suspend, clearing...");
  USB_GINTSTS_CLR_SET_USBSUSP(reg, 1);
  write_reg(USB_GINTSTS, reg);

}static inline void dwc2_irq_handle_eopf(void)
{
  uint32_t reg = 0;
  DWCINFO("eopf, clearing...");
  USB_GINTSTS_CLR_SET_EOPF(reg, 1);
  write_reg(USB_GINTSTS, reg);
}

static inline void dwc2_irq_handle_non_periodic_tx_fifo_empty(void)
{
  uint32_t reg = 0;
  DWCINFO("non-periodic tx fifo empty, clearing...");
  USB_GINTSTS_CLR_SET_NPTXFEMP(reg, 1);
  write_reg(USB_GINTSTS, reg);
}

static inline void dwc2_irq_handle_conn_id_sta_chng(void)
{
  uint32_t reg = 0;
  DWCINFO("connection id status changed, clearing...");
  USB_GINTSTS_CLR_SET_CONIDSTSCHNG(reg, 1);
  write_reg(USB_GINTSTS, reg);
}

static inline void dwc2_irq_handle_sess_req(void)
{
  uint32_t reg = 0;
  DWCINFO("session req, clearing...");
  USB_GINTSTS_CLR_SET_SESSREQINT(reg, 1);
  write_reg(USB_GINTSTS, reg);
}

static inline void dwc2_irq_handle_channel_int_one(int ch)
{
  uint32_t intr, intrmsk;
  GET_INTR();
  intrmsk = 0xffffffff;
  USB_HOST_INTR_CLR_HALT(intrmsk);
  SET_INTRMSK();
  DWCINFO("handling interrupt on channel %d, int=%08x", ch, intr);
}

static inline void dwc2_irq_handle_channel_int(void)
{
  int i;
  int num_channels = 6;
  uint32_t haint = read_reg(USB_HAINT);
  DWCINFO("channel interrupt:%08x, handling...", haint);
  for (i = 0; i < num_channels; ++i) {
    if ((1<<i) & haint)
      dwc2_irq_handle_channel_int_one(i);
  }
  write_reg(USB_HAINT, haint);
}

void dwc2_irq_cb(void)
{
  uint32_t intsts = read_reg(USB_GINTSTS);
  DWCINFO("IRQ: INTRSTS:%08x", intsts);
  dwc2_dump_int_registers();
  if (USB_GINTSTS_GET_HCHINT(intsts)) {
    dwc2_irq_handle_channel_int();
    USB_GINTSTS_CLR_HCHINT(intsts);
    write_reg(USB_GINTSTS, intsts);
  }
  if (USB_GINTSTS_GET_MODEMIS(intsts)) {
    dwc2_irq_handle_mode_mismatch();
  }
  if (USB_GINTSTS_GET_ERLYSUSP(intsts)) {
    dwc2_irq_handle_early_susp();
  }
  if (USB_GINTSTS_GET_EOPF(intsts)) {
    dwc2_irq_handle_eopf();
  }
  if (USB_GINTSTS_GET_CONIDSTSCHNG(intsts)) {
    dwc2_irq_handle_conn_id_sta_chng();
  }
  if (USB_GINTSTS_GET_SESSREQINT(intsts)) {
    dwc2_irq_handle_sess_req();
  }
  // if (USB_GINTSTS_GET_NPTXFEMP(intsts)) {
  //  dwc2_irq_handle_non_periodic_tx_fifo_empty();
  // }
  if (USB_GINTSTS_GET_USBSUSP(intsts)) {
    dwc2_irq_handle_usb_susp();
  }
  if (USB_GINTSTS_GET_OTGINT(intsts)) {
    dwc2_irq_handle_otgint();
  }
  if (USB_GINTSTS_GET_SOF(intsts)) {
    dwc2_irq_handle_sof();
  }
  if (USB_GINTSTS_GET_RXFLVL(intsts)) {
    dwc2_irq_handle_rx_fifo_lvl_intr();
  }
  /* non-periodic TX fifo empty */
  // if (USB_GINTSTS_GET_NPTXFEMP(intsts)) {
  //  DWCINFO("NPTXFEMP");
  // }
  /* port interrupts */
  if (USB_GINTSTS_GET_PRTINT(intsts)) {
    dwc2_irq_handle_port_intr();
  }
  /* periodic TX fifo empty */
  // if (USB_GINTSTS_GET_PTXFEMP(intsts)) {
  //  DWCINFO("PTXFEMP");
  // }
  if (USB_GINTSTS_GET_LPMTRANRCVD(intsts)) {
    DWCINFO("LPMTRANRCVD");
  }
  // write_reg(USB_GINTSTS, intsts);
}

void dwc2_set_port_status_changed_cb(port_status_changed_cb_t cb)
{
  port_status_changed_cb = cb;
}

static inline void dwc2_enable_channel_int(int ch)
{
  uint32_t haintmsk = read_reg(USB_HAINTMSK);
  haintmsk |= 1<<ch;
  write_reg(USB_HAINTMSK, haintmsk);
}

static inline void dwc2_disable_channel_int(int ch)
{
  uint32_t haintmsk = read_reg(USB_HAINTMSK);
  haintmsk &= ~((uint32_t)(1<<ch));
  write_reg(USB_HAINTMSK, haintmsk);
}

static inline void dwc2_init_channel_int(int ch)
{
  uint32_t intr = 0xffffffff;
  USB_HOST_INTR_CLR_HALT(intr);
  SET_INTR();
}

static inline void dwc2_deinit_channel_int(int ch)
{
  CLEAR_INTR();
}

void dwc2_enable_channel(int ch)
{
  dwc2_init_channel_int(ch);
  dwc2_enable_channel_int(ch);
}

void dwc2_disable_channel(int ch)
{
  dwc2_deinit_channel_int(ch);
  dwc2_disable_channel_int(ch);
}

struct dwc2_channel_desc {
  void *priv;
};

static DECL_SPINLOCK(dwc2_channels_lock);
static uint8_t channels_bitmap = 0;
static struct dwc2_channel_desc dwc2_channels[6] = { 0 };

dwc2_chan_id_t dwc2_channel_alloc()
{
  uint8_t bitmap;
  int ch;
  int num_channels = 6;
  printf("-------------------");
  if (spinlocks_enabled)
    spinlock_lock(&dwc2_channels_lock);
  bitmap = channels_bitmap;
  for (ch = 0; ch < num_channels; ++ch) {
    if (!(bitmap & (1<<ch)))
        break;
  }
  if (ch < num_channels) {
    bitmap |= (1<<ch);
    channels_bitmap = bitmap;
  }
  if (spinlocks_enabled)
    spinlock_unlock(&dwc2_channels_lock);
  if (ch == num_channels) {
    DWCINFO("Failed to allocate channel");
    return DWC2_INVALID_CHANNEL;
  }
  DWCINFO("channel %d allocated", ch);
  return ch;
}

void dwc2_channel_free(dwc2_chan_id_t ch)
{
  uint8_t bitmap;
  if (spinlocks_enabled)
    spinlock_lock(&dwc2_channels_lock);
  bitmap = channels_bitmap;
  BUG(!(bitmap & (1<<ch)), "Trying to release channel that's not busy");
  bitmap &= ~(uint8_t)(1<<ch); 
  channels_bitmap = bitmap;
  if (spinlocks_enabled)
    spinlock_unlock(&dwc2_channels_lock);
  DWCINFO("channel %d freed", ch);
}

void dwc2_channel_set_priv(dwc2_chan_id_t ch, void *priv)
{
  uint8_t bitmap;
  if (spinlocks_enabled)
    spinlock_lock(&dwc2_channels_lock);
  bitmap = channels_bitmap;
  BUG(!(bitmap & (1<<ch)), "Trying to access channel that's not allocated");
  struct dwc2_channel_desc *d = &dwc2_channels[ch];
  d->priv = priv;
  if (spinlocks_enabled)
    spinlock_unlock(&dwc2_channels_lock);
}

void *dwc2_channel_get_priv(dwc2_chan_id_t ch)
{
  uint8_t bitmap;
  void *priv;
  if (spinlocks_enabled)
    spinlock_lock(&dwc2_channels_lock);
  bitmap = channels_bitmap;
  BUG(!(bitmap & (1<<ch)), "Trying to access channel that's not allocated");
  struct dwc2_channel_desc *d = &dwc2_channels[ch];
  priv = d->priv;
  if (spinlocks_enabled)
    spinlock_unlock(&dwc2_channels_lock);
  return priv;
}

void dwc2_init(void)
{
  spinlock_init(&dwc2_channels_lock);
  memset(dwc2_channels, 0, sizeof(dwc2_channels));
}
