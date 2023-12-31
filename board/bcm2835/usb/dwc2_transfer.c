#include "dwc2.h"
#include "dwc2_regs.h"
#include "dwc2_reg_access.h"
#include "dwc2_log.h"
#include "bits_api.h"
#include <stringlib.h>
#include <common.h>
#include "dwc2_regs_bits.h"
#include <reg_access.h>
#include <usb/usb.h>
#include <usb/usb_printers.h>
#include <error.h>
#include <delays.h>
#include "dwc2_printers.h"
#include "dwc2_channel.h"

int dwc2_log_level = 0;

int dwc2_set_log_level(int log_level)
{
  int old = dwc2_log_level;
  dwc2_log_level = log_level;
  return old;
}

int dwc2_get_log_level()
{
  return dwc2_log_level;
}

static inline int dwc2_channel_calc_num_packets(struct dwc2_channel *c)
{
  int num_packets;
  int max_packet_size = dwc2_channel_get_max_packet_size(c);

  if (dwc2_channel_is_speed_low(c))
    max_packet_size = 8;
  num_packets = (c->ctl->transfer_size + max_packet_size  - 1) / max_packet_size;
  if (!num_packets)
    num_packets = 1;
  return num_packets;
}

static inline void dwc2_transfer_prologue(struct dwc2_channel *c)
{
  char pipe_str_buf[256];

  if (dwc2_log_level) {
    // dwc2_pipe_desc_to_string(c->pipe, pipe_str_buf, sizeof(pipe_str_buf));
    DWCDEBUG("%s", pipe_str_buf);

    if (c->ctl->direction == USB_DIRECTION_OUT)
      hexdump_memory((void*)c->ctl->dma_addr_base, c->ctl->transfer_size);
  }
}

static inline dwc2_transfer_status_t dwc2_wait_halted(int ch_id)
{
  int intr;
  int timeout = 1;
  while(1) {
    GET_INTR();
    if (USB_HOST_INTR_GET_HALT(intr)) {
      timeout = 0;
      break;
    }
    if (USB_HOST_INTR_GET_NAK(intr)) {
      timeout = 0;
      break;
    }
    wait_usec(100);
  }
  SET_INTR();

  if (timeout)
    return DWC2_STATUS_TIMEOUT;

  if (USB_HOST_INTR_GET_XFER_COMPLETE(intr))
    return DWC2_STATUS_ACK;

  if (USB_HOST_INTR_GET_ACK(intr)) {
    return DWC2_STATUS_ACK;
  }

  if (USB_HOST_INTR_GET_NAK(intr)) {
    DWCERR("NAK");
    return DWC2_STATUS_NAK;
  }

  if (USB_HOST_INTR_GET_NYET(intr)) {
    DWCWARN("NYET");
    return DWC2_STATUS_NYET;
  }

  if (USB_HOST_INTR_GET_STALL(intr))
    DWCWARN("STALL");

  if (USB_HOST_INTR_GET_AHB_ERR(intr))
    DWCERR("AHB_ERR");

  if (USB_HOST_INTR_GET_TRNSERR(intr))
    DWCERR("TRANSFER_ERR.");

  if (USB_HOST_INTR_GET_BABBLERR(intr))
    DWCERR("BABBLE_ERR");

  if (USB_HOST_INTR_GET_FRMOVRN(intr))
    DWCERR("FRAME_OVERRUN");

  if (USB_HOST_INTR_GET_DATTGGLERR(intr))
    DWCERR("DATA_TOGGLE_ERR");

  if (USB_HOST_INTR_GET_BUFNOTAVAIL(intr))
    DWCERR("BUF_NOT_AVAIL");

  if (USB_HOST_INTR_GET_EXCESSXMIT(intr))
    DWCERR("EXCESS_XMIT_ERR");

  if (USB_HOST_INTR_GET_FRMLISTROLL(intr))
    DWCERR("FRM_LIST_ROLL");

  return DWC2_STATUS_ERR;
}

int usb_pid_to_dwc_pid(usb_pid_t pid)
{
  switch (pid) {
    case USB_PID_DATA0: return USB_HCTSIZ0_PID_DATA0;
    case USB_PID_DATA1: return USB_HCTSIZ0_PID_DATA1;
    case USB_PID_DATA2: return USB_HCTSIZ0_PID_DATA2;
    case USB_PID_SETUP: return USB_HCTSIZ0_PID_SETUP;
    default: return 0xff;
  }
}

usb_pid_t dwc_pid_to_usb_pid(int pid)
{
  switch (pid) {
    case USB_HCTSIZ0_PID_DATA0: return USB_PID_DATA0;
    case USB_HCTSIZ0_PID_DATA1: return USB_PID_DATA1;
    case USB_HCTSIZ0_PID_DATA2: return USB_PID_DATA2;
    case USB_HCTSIZ0_PID_SETUP: return USB_PID_SETUP;
    default: return 0xff;
  }
}

static inline uint32_t dwc2_channel_ensure_disabled(struct dwc2_channel *c)
{
  int ch_id = c->id;
  uint32_t chr, intr;

  GET_CHAR();
  if (USB_HOST_CHAR_GET_CHAN_ENABLE(chr)) {
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(chr, 1);
    USB_HOST_CHAR_CLR_CHAN_ENABLE(chr);
    SET_CHAR();
    while(1) {
      GET_INTR();
      SET_INTR();
      if (USB_HOST_INTR_GET_HALT(intr))
        break;
      USB_HOST_INTR_CLR_HALT(intr);
      BUG(intr, "interrupt disable logic error");
    }
  }
  GET_CHAR();
  BUG(USB_HOST_CHAR_GET_CHAN_ENABLE(chr), "Channel not disabled");
  return chr;
}

static inline void dwc2_channel_program_char(struct dwc2_channel *c)
{
  uint32_t chr;
  int ch_id = c->id;
  /* Program the channel. */
  chr = dwc2_channel_ensure_disabled(c);

  USB_HOST_CHAR_CLR_SET_MAX_PACK_SZ(chr, dwc2_channel_get_max_packet_size(c));
  USB_HOST_CHAR_CLR_SET_EP(chr, dwc2_channel_get_ep_num(c));
  USB_HOST_CHAR_CLR_SET_EP_DIR(chr, c->ctl->direction);
  USB_HOST_CHAR_CLR_SET_IS_LOW(chr, dwc2_channel_is_speed_low(c) ? 1 : 0);
  USB_HOST_CHAR_CLR_SET_EP_TYPE(chr, dwc2_channel_get_ep_type(c));
  USB_HOST_CHAR_CLR_SET_DEV_ADDR(chr, dwc2_channel_get_device_address(c));
  USB_HOST_CHAR_CLR_SET_PACK_PER_FRM(chr, 1);
  SET_CHAR();
}

static inline void dwc2_channel_program_split(struct dwc2_channel *c)
{
  uint32_t splt;
  int ch_id = c->id;
  /* Clear and setup split control to low speed devices */
  splt = 0;

  if (dwc2_channel_split_mode(c)) {
    USB_HOST_SPLT_CLR_SET_SPLT_ENABLE(splt, 1);
    USB_HOST_SPLT_CLR_SET_HUB_ADDR(splt, c->pipe->ls_hub_address);
    USB_HOST_SPLT_CLR_SET_PORT_ADDR(splt, c->pipe->ls_hub_port);
    DWCDEBUG("LOW/FULL_SPEED:split: %08x, hub:%d, port:%d", splt, c->pipe->ls_hub_address, c->pipe->ls_hub_port);
  }
  SET_SPLT();
}

static inline void dwc2_channel_program_tsize(struct dwc2_channel *c)
{
  uint32_t siz = 0;
  int ch_id = c->id;
  /* Set transfer size. */
  int num_packets = dwc2_channel_calc_num_packets(c);

  USB_HOST_SIZE_CLR_SET_SIZE(siz, c->ctl->transfer_size);
  USB_HOST_SIZE_CLR_SET_PACKET_COUNT(siz, num_packets);
  USB_HOST_SIZE_CLR_SET_PID(siz, usb_pid_to_dwc_pid(c->pipe->next_pid));
  SET_SIZ();
}

static inline void dwc2_channel_set_split_start(struct dwc2_channel *c)
{
  uint32_t splt;
  int ch_id = c->id;
  GET_SPLT();
  USB_HOST_SPLT_CLR_COMPLETE_SPLIT(splt);
  SET_SPLT();
}

static inline void dwc2_channel_set_dma_addr(struct dwc2_channel *c)
{
  uint32_t dma;
  int ch_id = c->id;
  BUG(c->ctl->first_packet && (c->ctl->dma_addr & 3), "UNALIGNED DMA address");

  /*
   * TODO: Need to figure out why we need this flush here, because it
   * actually needs to be done (and is already done) on the upper level
   */
  dcache_flush(c->ctl->dma_addr, c->ctl->transfer_size);

  if (c->ctl->dma_addr) {
    dma = RAM_PHY_TO_BUS_UNCACHED(c->ctl->dma_addr);
    SET_DMA();
  }
}

void dwc2_channel_set_split_complete(struct dwc2_channel *c)
{
  uint32_t splt;
  int ch_id = c->id;
  GET_SPLT();
  DWCDEBUG("setting complete split, reg: %08x", splt);
  USB_HOST_SPLT_CLR_SET_COMPLETE_SPLIT(splt, 1);
  SET_SPLT();
}

void dwc2_channel_set_intmsk(struct dwc2_channel *c)
{
  uint32_t intrmsk;
  int ch_id = c->id;
  int ep_type;
  bool is_split;

  ep_type = dwc2_channel_get_ep_type(c);
  is_split = dwc2_channel_split_mode(c);

  intrmsk = 0;
  USB_HOST_INTR_CLR_SET_HALT(intrmsk, 1);
  USB_HOST_INTR_CLR_SET_XFER_COMPLETE(intrmsk, 1);

  USB_HOST_INTR_CLR_SET_AHB_ERR(intrmsk, 1);
  USB_HOST_INTR_CLR_SET_STALL(intrmsk, 1);
  USB_HOST_INTR_CLR_SET_FRMOVRN(intrmsk, 1);
  USB_HOST_INTR_CLR_SET_TRNSERR(intrmsk, 1);
  USB_HOST_INTR_CLR_SET_BABBLERR(intrmsk, 1);
  USB_HOST_INTR_CLR_SET_DATTGGLERR(intrmsk, 1);

  if (usb_is_periodic_ep_type(ep_type) || is_split) {
    USB_HOST_INTR_CLR_SET_ACK(intrmsk, 1);
    USB_HOST_INTR_CLR_SET_NAK(intrmsk, 1);
    USB_HOST_INTR_CLR_SET_NYET(intrmsk, 1);
  }

  // printf("SETINTR:%08x\n", intrmsk);
  SET_INTRMSK();
}

static inline void dwc2_channel_start_transmit(struct dwc2_channel *c)
{
  uint32_t chr;
  int ch_id = c->id;

  GET_CHAR();
  USB_HOST_CHAR_CLR_CHAN_DISABLE(chr);
  USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(chr, 1);
  SET_CHAR();
  dwc2_channel_set_intmsk(c);
  dwc2_channel_enable(ch_id);
}

bool dwc2_channel_is_split_enabled(struct dwc2_channel *c)
{
  uint32_t splt;
  int ch_id = c->id;
  GET_SPLT();
  return USB_HOST_SPLT_GET_SPLT_ENABLE(splt) ? true : false;
}

static inline void dwc2_channel_clear_pending(struct dwc2_channel *c)
{
  int ch_id = c->id;
  CLEAR_INTR();
  CLEAR_INTRMSK();
}

void dwc2_transfer_prepare(struct dwc2_channel *c)
{
  int ep_num;

  ep_num = dwc2_channel_get_ep_num(c);
  usb_pid_t next_pid = dwc2_channel_get_next_pid(c);

  DWCDEBUG2("addr:%p, ep:%d, sz:%d, pid:%s, dir:%s, speed:%s",
    c->ctl->dma_addr_base,
    ep_num,
    c->ctl->transfer_size,
    usb_pid_t_to_string(next_pid),
    usb_direction_to_string(c->ctl->direction),
    usb_speed_to_string(dwc2_channel_get_pipe_speed(c)));

  if (next_pid == USB_PID_SETUP && dwc2_log_level > 1)
    hexdump_memory((void *)c->ctl->dma_addr_base, 8);

  BUG((uint64_t)c->ctl->dma_addr_base & 3, "dwc2_transfer:buffer not aligned to 4 bytes");
  dwc2_channel_disable(c->id);
  dwc2_channel_clear_pending(c);

  c->ctl->status = DWC2_TRANSFER_STATUS_STARTED;
  dwc2_channel_program_char(c);
  dwc2_channel_program_split(c);
  dwc2_channel_program_tsize(c);

  if (dwc2_channel_split_mode(c))
    c->ctl->split_start = true;
  c->ctl->dma_addr = c->ctl->dma_addr_base;
  c->ctl->first_packet = 1;
}

int dwc2_transfer_retry(struct dwc2_channel *c)
{
  dwc2_channel_start_transmit(c);
  // putc(',');
  return ERR_OK;
}

int OPTIMIZED dwc2_transfer_stop(struct dwc2_channel *c)
{
  uint32_t chr;
  int ch_id = c->id;
  int intr;

  GET_CHAR();
  USB_HOST_CHAR_CLR_CHAN_DISABLE(chr);
  SET_CHAR();
  printf("transfer_stop\r\n");
  GET_INTR();
  USB_HOST_INTR_CLR_NAK(intr);
  SET_INTR();
  return ERR_OK;
}

int OPTIMIZED dwc2_transfer_start(struct dwc2_channel *c)
{
  if (dwc2_channel_split_mode(c)) {
    if (c->ctl->split_start) {
      dwc2_channel_set_split_start(c);
      dwc2_channel_set_dma_addr(c);
    } else
      dwc2_channel_set_split_complete(c);

    c->ctl->split_start = !c->ctl->split_start;
    // DWCINFO("split_mode: dwc2_transfer_start, split_start set to %s"__endline, c->ctl->split_start ? "yes" : "no");
  } else {
    // DWCINFO("no_split_mode: dwc2_transfer_start"__endline);
    dwc2_channel_set_dma_addr(c);
  }

  dwc2_channel_start_transmit(c);
  return ERR_OK;
}

int dwc2_transfer_recalc_next(struct dwc2_xfer_control *ctl)
{
  uint32_t siz;
  int packets_left;
  int bytes_left;
  int ch_id = ctl->c->id;
  int ep_num;

  GET_SIZ();
  ep_num = dwc2_channel_get_ep_num(ctl->c);
  dwc2_channel_set_next_pid(ctl->c, dwc_pid_to_usb_pid(USB_HOST_SIZE_GET_PID(siz)));

  if (dwc2_log_level > 1) {
    DWCDEBUG("ep:%d, %s tsiz_compl:%08x", ep_num, ctl->direction == USB_DIRECTION_IN ? " IN" : "OUT", siz);
  }
  if (ctl->direction == USB_DIRECTION_IN) {
    bytes_left = USB_HOST_SIZE_GET_SIZE(siz);
    packets_left = USB_HOST_SIZE_GET_PACKET_COUNT(siz);
    if (packets_left) {
      ctl->dma_addr = ctl->dma_addr_base + ctl->transfer_size - bytes_left;
      ctl->first_packet = 0;
      DWCDEBUG("transmission state(IN): packets left: %d, bytes left: %d, base_addr: %p, next_addr: %p",
        packets_left, bytes_left, ctl->dma_addr_base, ctl->dma_addr);
      return 1;
    }
    DWCDEBUG("transmission state(IN): complete");
  } else
    DWCDEBUG("transmission state(OUT): complete");
  return 0;
}

void dwc2_transfer_completed_debug(struct dwc2_channel *c)
{
  uint32_t intr, siz, splt;
  int ch_id = c->id;
  if (dwc2_log_level > 1) {
    GET_SPLT();
    GET_INTR();
    GET_SIZ();
    DWCDEBUG("siz:%08x(packets:%d, size:%d), intr:%08x, dma_addr:%p, transfer_size:%d, split:%08x",
      siz,
      USB_HOST_SIZE_GET_PACKET_COUNT(siz),
      USB_HOST_SIZE_GET_SIZE(siz),
      intr,
      c->ctl->dma_addr,
      c->ctl->transfer_size,
      splt);

   if (dwc2_log_level > 2)
     hexdump_memory((void*)c->ctl->dma_addr, c->ctl->transfer_size);
  }
}

int dwc2_xfer_one_job(struct usb_xfer_job *j)
{
  struct dwc2_channel *c = j->jc->channel;
  c->ctl->dma_addr_base = (uint64_t)j->addr;
  c->ctl->direction = j->direction;
  c->ctl->transfer_size = j->transfer_size;
  if (j->next_pid != USB_PID_UNSET)
    dwc2_channel_set_next_pid(c, j->next_pid);
  DWCDEBUG2("%s, j:%p, sz:%d",
    j->direction == USB_DIRECTION_OUT ? "out" : "in",
    j,
    j->transfer_size);

  c->ctl->completion = j->completion;
  c->ctl->completion_arg = j->completion_arg;
  dwc2_transfer_prepare(c);
  return dwc2_transfer_start(c);
}

dwc2_transfer_status_t dwc2_transfer_blocking(struct dwc2_channel *c, int *out_num_bytes)
{
  dwc2_transfer_status_t status = DWC2_STATUS_ACK;
  int ch_id = c->id;

  uint32_t intr UNUSED;
  uint32_t siz;

  dwc2_transfer_prologue(c);
  dwc2_transfer_prepare(c);

  while(1) {
    dwc2_channel_clear_pending(c);
    dwc2_transfer_start(c);

    status = dwc2_wait_halted(ch_id);
    if (status != DWC2_STATUS_ACK) {
      DWCERR("status:%s\n", dwc2_transfer_status_to_string(status));
      if (status == DWC2_STATUS_NYET)
        continue;
      if (status == DWC2_STATUS_ERR)
        continue;
      goto out;
    }

    if (dwc2_channel_is_split_enabled(c)) {
      dwc2_channel_clear_pending(c);
      dwc2_transfer_start(c);
      status = dwc2_wait_halted(ch_id);
      if (status != DWC2_STATUS_ACK) {
        DWCERR("split status:%s\n", dwc2_transfer_status_to_string(status));
        if (status == DWC2_STATUS_NYET)
          continue;
        if (status == DWC2_STATUS_ERR)
          continue;
        goto out;
      }
    }
    dwc2_transfer_completed_debug(c);
    if (!dwc2_transfer_recalc_next(c->ctl))
      break;
  }
out:
  GET_SIZ();
  if (out_num_bytes) {
    if (c->ctl->direction == USB_DIRECTION_IN)
      *out_num_bytes = c->ctl->transfer_size - USB_HOST_SIZE_GET_SIZE(siz);
    else
      *out_num_bytes = c->ctl->transfer_size;
  }

  dwc2_channel_set_next_pid(c, dwc_pid_to_usb_pid(USB_HOST_SIZE_GET_PID(siz)));

  DWCDEBUG("=======TRANSFER END=tsz:%08x========================", siz);
	return status;
}

static int dwc2_hwcfg_get_num_host_channels()
{
  uint32_t hwcfg = read_reg(USB_GHWCFG2);
  return USB_GHWCFG2_GET_NUM_HOST_CHAN(hwcfg);
}

int dwc2_init_channels()
{
  int i;
  int num_channels;
  uint32_t channel_char;
  num_channels = dwc2_hwcfg_get_num_host_channels();
  for (i = 0; i < num_channels; ++i) {
    channel_char = __read_ch_reg(USB_HCCHAR0, i, usb_host_char_to_string);
    USB_HOST_CHAR_CLR_CHAN_ENABLE(channel_char);
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_EP_DIR(channel_char, USB_DIRECTION_IN);
    __write_ch_reg(USB_HCCHAR0, i, channel_char, usb_host_char_to_string);
    DWCINFO("channel %d disabled", i);
  }
  for (i = 0; i < num_channels; ++i) {
    channel_char = __read_ch_reg(USB_HCCHAR0, i, usb_host_char_to_string);
    USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_EP_DIR(channel_char, USB_DIRECTION_IN);
    __write_ch_reg(USB_HCCHAR0, i, channel_char, usb_host_char_to_string);
    do {
      channel_char = __read_ch_reg(USB_HCCHAR0, i, usb_host_char_to_string);
    } while(USB_HOST_CHAR_GET_CHAN_ENABLE(channel_char));
    DWCINFO("channel %d enabled", i);
  }
  DWCINFO("channel initialization completed");
  return ERR_OK;
}

//usb_pid_t dwc2_channel_get_next_pid(struct dwc2_channel *c)
//{
//  uint32_t siz;
//  int ch_id = c->id;
//  GET_SIZ();
//  return dwc_pid_to_usb_pid(USB_HOST_SIZE_GET_PID(siz));
//}
