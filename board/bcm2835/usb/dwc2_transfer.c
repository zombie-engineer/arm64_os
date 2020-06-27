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

static inline int dwc2_char_reg_to_string(int ch, char *buf, int bufsz)
{
  uint32_t chr;
  GET_CHAR();
  return snprintf(buf, bufsz, "%08x", chr);
}

static inline int dwc2_split_reg_to_string(int ch, char *buf, int bufsz)
{
  uint32_t splt;
  GET_SPLT();
  return snprintf(buf, bufsz, "%08x", splt);
}

static inline int dwc2_intr_reg_to_string(int ch, char *buf, int bufsz)
{
  uint32_t intr;
  GET_INTR();
  return snprintf(buf, bufsz, "%08x", intr);
}

static inline int dwc2_intrmsk_reg_to_string(int ch, char *buf, int bufsz)
{
  uint32_t intrmsk;
  GET_INTRMSK();
  return snprintf(buf, bufsz, "%08x", intrmsk);
}

static inline int dwc2_tsize_reg_to_string(int ch, char *buf, int bufsz)
{
  uint32_t siz;
  GET_SIZ();
  return snprintf(buf, bufsz, "%08x", siz);
}

static inline int dwc2_dma_reg_to_string(int ch, char *buf, int bufsz)
{
  uint32_t dma;
  GET_DMA();
  return snprintf(buf, bufsz, "%08x", dma);
}


static inline int dwc2_channel_regs_to_string(int ch, char *buf, int bufsz)
{
  int n = 0;
#define __REG(__r)\
  n = snprintf(buf + n, bufsz - n, "  "#__r": "__endline);\
  n = dwc2_ ## __r ## _reg_to_string(ch, buf + n, bufsz - n)

  __REG(char);
  __REG(split);
  __REG(intr);
  __REG(intrmsk);
  __REG(tsize);
  __REG(dma);
#undef __REG
  return n;
}

int dwc2_pipe_desc_to_string(dwc2_pipe_desc_t desc, char *buf, int bufsz)
{
  return snprintf(buf, bufsz, "%016lx:ep:%s(%d),addr:%d,ep:%d,dir:%s,speed:%s(%d),sz:%d,ch:%d", 
      desc.u.raw, 
      usb_endpoint_type_to_short_string(desc.u.ep_type), desc.u.ep_type,
      desc.u.device_address, desc.u.ep_address,
      usb_direction_to_string(desc.u.ep_direction),
      usb_speed_to_string(desc.u.speed), desc.u.speed,
      desc.u.max_packet_size,
      desc.u.dwc_channel);
}

#define DWC2_MAKE_TSIZE(size, packet_count, pid, do_ping)\
   (BITS_PLACE_32(size        , 0 , 19)\
   |BITS_PLACE_32(packet_count, 19, 10)\
   |BITS_PLACE_32(pid         , 29,  2)\
   |BITS_PLACE_32(do_ping     , 31,  1))

#define DWC2_MAKE_SPLT(hub_addr, port_addr, split_en)\
  (BITS_PLACE_32(hub_addr, 0, 7)|BITS_PLACE_32(port_addr, 7, 7)|BITS_PLACE_32(split_en, 31, 1))

#define DWC2_MAKE_CHAR(max_pack_sz, ep, ep_dir, low_speed, ep_type, dev_addr, ena)\
  (BITS_PLACE_32(max_pack_sz, 0, 11)\
  |BITS_PLACE_32(ep, 11, 4)\
  |BITS_PLACE_32(ep_dir, 15, 1)\
  |BITS_PLACE_32(low_speed, 17, 1)\
  |BITS_PLACE_32(ep_type, 18, 2)\
  |BITS_PLACE_32(dev_addr, 22, 7)\
  |BITS_PLACE_32(ena, 31, 1))


static inline uint32_t dwc2_make_split(int hub_addr, int port_addr, int ena)
{
  return DWC2_MAKE_SPLT(hub_addr, port_addr, ena);
}

static inline uint32_t dwc2_make_char(int max_pack_sz, int ep, int ep_dir, int speed, int ep_type, int dev_addr, int ena)
{
  return DWC2_MAKE_CHAR(max_pack_sz, ep, ep_dir, speed, ep_type, dev_addr, ena);
}

static inline int get_num_packets(int datasz, int low_speed, int max_packet_size)
{
  int num_packets;
  if (low_speed)
    max_packet_size = 8;
  num_packets = (datasz + max_packet_size  - 1) / max_packet_size;
  if (!num_packets)
    num_packets = 1;
  return num_packets;
}

static inline uint32_t dwc2_make_tsize(int length, int low_speed, int pid, int max_packet_size)
{
  uint32_t r = 0;
  int num_packets = get_num_packets(length, low_speed, max_packet_size);
  USB_HOST_SIZE_CLR_SET_SIZE(r, length);
  USB_HOST_SIZE_CLR_SET_PACKET_COUNT(r, num_packets);
  USB_HOST_SIZE_CLR_SET_PID(r, pid);
  return r;
}

static inline void dwc2_transfer_prologue(dwc2_pipe_desc_t pipe, void *buf, int bufsz, int pid)
{
  char pipe_str_buf[256];
  if (dwc2_log_level) {
    dwc2_pipe_desc_to_string(pipe, pipe_str_buf, sizeof(pipe_str_buf));
    DWCDEBUG("=======TRANSFER=====================================");
    DWCDEBUG("pipe:%s,size:%d,pid:%d", pipe_str_buf, bufsz, pid);

    if (pipe.u.ep_direction == USB_DIRECTION_OUT)
      hexdump_memory(buf, bufsz);
  }
}

static inline dwc2_transfer_status_t dwc2_wait_halted(int ch)
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

static inline void dwc2_channel_program_char(dwc2_pipe_desc_t pipe)
{
  uint32_t chr;
  int ch = pipe.u.dwc_channel;
  /* Program the channel. */
  chr = 0;
  USB_HOST_CHAR_CLR_SET_MAX_PACK_SZ(chr, pipe.u.max_packet_size);
  USB_HOST_CHAR_CLR_SET_EP(chr, pipe.u.ep_address);
  USB_HOST_CHAR_CLR_SET_EP_DIR(chr, pipe.u.ep_direction);
  USB_HOST_CHAR_CLR_SET_IS_LOW(chr, pipe.u.speed == USB_SPEED_LOW ? 1 : 0);
  USB_HOST_CHAR_CLR_SET_EP_TYPE(chr, pipe.u.ep_type);
  USB_HOST_CHAR_CLR_SET_DEV_ADDR(chr, pipe.u.device_address);
  USB_HOST_CHAR_CLR_SET_PACK_PER_FRM(chr, 1);
  SET_CHAR();
}

static inline void dwc2_channel_program_split(struct dwc2_channel *c)
{
  uint32_t splt;
  int ch = c->pipe.u.dwc_channel;
  /* Clear and setup split control to low speed devices */
  splt = 0;
  if (c->pipe.u.speed != USB_SPEED_HIGH) {
    USB_HOST_SPLT_CLR_SET_SPLT_ENABLE(splt, 1);
    USB_HOST_SPLT_CLR_SET_HUB_ADDR(splt, c->pipe.u.hub_address);
    USB_HOST_SPLT_CLR_SET_PORT_ADDR(splt, c->pipe.u.hub_port);
    DWCDEBUG("LOW/FULL_SPEED:split: %08x, hub:%d, port:%d\r\n", splt, c->pipe.u.hub_address, c->pipe.u.hub_port);
  }
  SET_SPLT();
}

static inline void dwc2_channel_program_tsize(dwc2_pipe_desc_t pipe, int transfer_size, int pid)
{
  uint32_t siz;
  int ch = pipe.u.dwc_channel;
  /* Set transfer size. */
  siz = dwc2_make_tsize(transfer_size, pipe.u.speed == USB_SPEED_LOW, pid, pipe.u.max_packet_size);
  SET_SIZ();
}

static inline void dwc2_channel_set_split_start(dwc2_pipe_desc_t pipe)
{
  uint32_t splt;
  int ch = pipe.u.dwc_channel;
  GET_SPLT();
  USB_HOST_SPLT_CLR_COMPLETE_SPLIT(splt);
  SET_SPLT();
}

static inline void dwc2_channel_set_dma_addr(dwc2_pipe_desc_t pipe, uint64_t dma_addr, int transfer_size)
{
  uint32_t dma;
  int ch = pipe.u.dwc_channel;
  // printf("dma_addr:%016x transfer_size:%d\n", dma_addr, transfer_size);
  BUG(dma_addr & 3, "UNALIGNED DMA address");

  if (dma_addr && transfer_size && pipe.u.ep_direction == USB_DIRECTION_OUT)
    dcache_flush(dma_addr, transfer_size);

  dcache_flush(dma_addr, transfer_size);
  dma = RAM_PHY_TO_BUS_UNCACHED(dma_addr);
  SET_DMA();
}

static inline void dwc2_channel_set_split_complete(dwc2_pipe_desc_t pipe)
{
  uint32_t splt;
  int ch = pipe.u.dwc_channel;
  GET_SPLT();
  DWCDEBUG("SPLT ENA: %08x", splt);
  USB_HOST_SPLT_CLR_SET_COMPLETE_SPLIT(splt, 1);
  SET_SPLT();
}

static inline void dwc2_channel_start_transmit(dwc2_pipe_desc_t pipe)
{
  uint32_t chr;
  int ch = pipe.u.dwc_channel;
  GET_CHAR();
  USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(chr, 1);
  USB_HOST_CHAR_CLR_CHAN_DISABLE(chr);
  SET_CHAR();
}

bool dwc2_is_split_enabled(dwc2_pipe_desc_t pipe)
{
  uint32_t splt;
  int ch = pipe.u.dwc_channel;
  GET_SPLT();
  return USB_HOST_SPLT_GET_SPLT_ENABLE(splt) ? true : false;
}

static inline void dwc2_channel_clear_intr(dwc2_pipe_desc_t pipe)
{
  int ch = pipe.u.dwc_channel;
  CLEAR_INTR();
  CLEAR_INTRMSK();
}

void dwc2_transfer_continue(dwc2_pipe_desc_t pipe)
{
  if (dwc2_is_split_enabled(pipe)) {
    dwc2_channel_clear_intr(pipe);
    dwc2_channel_set_split_complete(pipe);
    dwc2_channel_start_transmit(pipe);
  }
}

void dwc2_transfer_prepare(dwc2_pipe_desc_t pipe, void *addr, int transfer_size, usb_pid_t *pid)
{
  int dwc_pid = usb_pid_to_dwc_pid(*pid);
  struct dwc2_channel *c;
  struct dwc2_transfer_ctl *tc;

  BUG((uint64_t)addr & 3, "dwc2_transfer:buffer not aligned to 4 bytes");

  // if (addr && transfer_size && pipe.u.ep_direction == USB_DIRECTION_OUT)
  //  dcache_flush(addr, transfer_size);

  c = dwc2_channel_get(pipe.u.dwc_channel);
  c->pipe.u.raw = pipe.u.raw;
  c->tc->split_start = true;
  tc = c->tc;
  tc->status = DWC2_TRANSFER_STATUS_STARTED;
  tc->transfer_size = transfer_size;
  dwc2_channel_program_char(pipe);
  dwc2_channel_program_split(c);
  dwc2_channel_program_tsize(pipe, transfer_size, dwc_pid);
  c->tc->dma_addr = c->tc->dma_addr_base = (uint64_t)addr;
}

int dwc2_transfer_start(dwc2_pipe_desc_t pipe)
{
  uint32_t UNUSED intr, intrmsk;
  struct dwc2_channel *c;
  int ch = pipe.u.dwc_channel;
  c = dwc2_channel_get(pipe.u.dwc_channel);

  if (c->tc->split_start) {
    dwc2_channel_set_split_start(pipe);
    dwc2_channel_set_dma_addr(pipe, c->tc->dma_addr, c->tc->transfer_size);
    c->tc->split_start = false;
  } else {
    dwc2_channel_set_split_complete(pipe);
    c->tc->split_start = true;
  }

  dwc2_channel_start_transmit(pipe);

  GET_INTR();
  intrmsk = 0xffffffff;
  USB_HOST_INTR_CLR_HALT(intrmsk);
  SET_INTRMSK();
  return ERR_OK;
}

int dwc2_transfer_recalc_next(struct dwc2_channel *c)
{
  uint32_t siz;
  int packets_left;
  int bytes_left;
  int ch = c->pipe.u.dwc_channel;
  if (c->pipe.u.ep_direction == USB_DIRECTION_IN) {
    GET_SIZ();
    bytes_left = USB_HOST_SIZE_GET_SIZE(siz);
    packets_left = USB_HOST_SIZE_GET_PACKET_COUNT(siz);
    if (packets_left) {
      DWCDEBUG("################## packets_left: %d, bytes_left: %d", packets_left, bytes_left);
      c->tc->dma_addr = c->tc->dma_addr_base + c->tc->transfer_size - bytes_left;
      return 1;
    }
  } else
    DWCDEBUG("################## out");
  return 0;
}

void dwc2_transfer_completed_debug(struct dwc2_channel *c)
{
  uint32_t intr, siz, splt;
  int ch = c->pipe.u.dwc_channel;
  if (dwc2_log_level > 1) {
    GET_SPLT();
    GET_INTR();
    GET_SIZ();
    DWCDEBUG("siz:%08x(packets:%d, size:%d), intr:%08x, dma_addr:%p, transfer_size:%d, split:%08x",
      siz,
      USB_HOST_SIZE_GET_PACKET_COUNT(siz),
      USB_HOST_SIZE_GET_SIZE(siz),
      intr,
      c->tc->dma_addr,
      c->tc->transfer_size,
      splt);

   if (dwc2_log_level > 2)
     hexdump_memory((void*)c->tc->dma_addr, c->tc->transfer_size);
  }
}

dwc2_transfer_status_t dwc2_transfer_blocking(dwc2_pipe_desc_t pipe, void *addr, int transfer_size, usb_pid_t *pid, int *out_num_bytes)
{
  dwc2_transfer_status_t status = DWC2_STATUS_ACK;
  int ch = pipe.u.dwc_channel;
  struct dwc2_channel *c = dwc2_channel_get(ch);

  uint32_t intr UNUSED;
  uint32_t siz;

  dwc2_transfer_prologue(pipe, addr, transfer_size, *pid);

  dwc2_channel_clear_intr(pipe);
  dwc2_transfer_prepare(pipe, addr, transfer_size, pid);

  while(1) {
    dwc2_channel_clear_intr(pipe);
    dwc2_transfer_start(pipe);

    status = dwc2_wait_halted(ch);
    if (status != DWC2_STATUS_ACK) {
      DWCERR("status:%s\n", dwc2_transfer_status_to_string(status));
      if (status == DWC2_STATUS_NYET)
        continue;
      if (status == DWC2_STATUS_ERR)
        continue;
      goto out;
    }

    if (dwc2_is_split_enabled(pipe)) {
      dwc2_channel_clear_intr(pipe);
      dwc2_transfer_start(pipe);
      wait_usec(100);
      status = dwc2_wait_halted(ch);
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
    if (!dwc2_transfer_recalc_next(c))
      break;
  }
out:
  GET_SIZ();
  if (out_num_bytes) {
    if (pipe.u.ep_direction == USB_DIRECTION_IN)
      *out_num_bytes = transfer_size - USB_HOST_SIZE_GET_SIZE(siz);
    else
      *out_num_bytes = transfer_size;
  }
  *pid = dwc_pid_to_usb_pid(USB_HOST_SIZE_GET_PID(siz));

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

