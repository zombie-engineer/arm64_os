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

static int dwc2_print_debug = 0;

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
  return snprintf(buf, bufsz, "%016lx:%s:%d/%d/%s/%s/%d:%d", 
      desc.u.raw, 
      usb_endpoint_type_to_short_string(desc.u.ep_type),
      desc.u.device_address, desc.u.ep_address,
      usb_direction_to_string(desc.u.ep_direction),
      desc.u.low_speed ? "ls" : "--",
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
  return DWC2_MAKE_TSIZE(length, get_num_packets(length, low_speed, max_packet_size), pid, 0);
}

static inline void dwc2_transfer_prologue(dwc2_pipe_desc_t pipe, void *buf, int bufsz, int pid)
{
  char pipe_str_buf[256];
  if (dwc2_print_debug) {
    dwc2_pipe_desc_to_string(pipe, pipe_str_buf, sizeof(pipe_str_buf));
    DWCDEBUG("=======TRANSFER=====================================");
    DWCDEBUG("pipe:%s", pipe_str_buf);

    if (pipe.u.ep_direction == USB_DIRECTION_OUT)
      hexdump_memory(buf, bufsz);
  }
}

int dwc2_transfer(dwc2_pipe_desc_t pipe, void *buf, int bufsz, int pid, int *out_num_bytes) 
{
  int err = ERR_OK;
  int i, j; 
  int ch = pipe.u.dwc_channel;

  const int max_retries = 8;
  uint32_t chr, splt, intr, siz, dma;
  uint32_t *ptr;

  dwc2_transfer_prologue(pipe, buf, bufsz, pid);

  ch = pipe.u.dwc_channel = 6;

  if ((uint64_t)buf & 3) {
    DWCERR("dwc2_transfer:buffer not aligned to 4 bytes\r\n");
    return ERR_ALIGN;
  }

  /* Clear all existing interrupts. */
  CLEAR_INTR();
  CLEAR_INTRMSK();

  /* Program the channel. */
  chr = 0;
  chr |= (pipe.u.max_packet_size & 0x7ff) << 0 ;
  chr |= (pipe.u.ep_address      &   0xf) << 11;
  chr |= (pipe.u.ep_direction    &     1) << 15;
  chr |= (pipe.u.low_speed       &     1) << 17;
  chr |= (pipe.u.ep_type         &     2) << 18;
  chr |= (pipe.u.device_address  &  0x7f) << 22;
  SET_CHAR();

  /* Clear and setup split control to low speed devices */
  splt = 0;
 //  splt |= 1 << 31; // split_enable
 //  splt |= (pipe.ls_node_point & 0x7f) << 0;
 //  splt |= (pipe.ls_node_port  & 0x7f) << 7;
  SET_SPLT();

  /* Set transfer size. */
  siz = dwc2_make_tsize(bufsz, pipe.u.low_speed, pid, pipe.u.max_packet_size);
  SET_SIZ();

  ptr = (uint32_t*)buf;
  for (i = 0; i < max_retries; ++i) {
    DWCDEBUG2("transfer: try:%d", i);
    CLEAR_INTR();
    CLEAR_INTRMSK();
    GET_SPLT();
    BIT_CLEAR_U32(splt, 16);
    SET_SPLT();

    dma = RAM_PHY_TO_BUS_UNCACHED((uint64_t)ptr);
    ptr++;
    SET_DMA();

    /* launch transmission */
    GET_CHAR();
    BIT_CLEAR_U32(chr, 20);
    BIT_CLEAR_U32(chr, 21); // 1 frame/packet
    BIT_SET_U32(chr, 20);
    BIT_SET_U32(chr, 31); // enable channel
    SET_CHAR();

    do {
      wait_usec(100);
      GET_INTR();
    } while(!USB_HOST_INTR_GET_HALT(intr));

    if (USB_HOST_INTR_GET_XFER_COMPLETE(intr)) {
      if (USB_HOST_INTR_GET_AHB_ERR(intr)) {
        DWCERR("AHB error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_STALL(intr)) {
        DWCWARN("STALL");
      }
      if (USB_HOST_INTR_GET_NAK(intr)) {
        DWCWARN("NAK. retrying");
        continue;
      }
      if (USB_HOST_INTR_GET_NYET(intr)) {
        DWCWARN("NYET. retrying.");
        continue;
      }
      if (USB_HOST_INTR_GET_TRNSERR(intr)) {
        DWCERR("transfer error.");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_BABBLERR(intr)) {
        DWCERR("BABBLE error.");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_FRMOVRN(intr)) {
        DWCERR("frame overrun");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_DATTGGLERR(intr)) {
        DWCERR("data toggle error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_DATTGGLERR(intr)) {
        DWCERR("data toggle error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_BUFNOTAVAIL(intr)) {
        DWCERR("buffer not available error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_EXCESSXMIT(intr)) {
        DWCERR("excess transmit error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_FRMLISTROLL(intr)) {
        DWCERR("frame list roll error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_ACK(intr)) {
        err = ERR_OK;
        goto out_err;
      }
    }

    GET_SPLT();
  }
out_err:
  if (out_num_bytes) {
    GET_SIZ();
    *out_num_bytes = bufsz - USB_HOST_SIZE_GET_SIZE(siz);
  }
  
  DWCDEBUG("=======TRANSFER END=tsz:%08x========================", siz);
	return err;
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
    channel_char = __read_ch_reg(USB_HCCHAR0, i);
    USB_HOST_CHAR_CLR_CHAN_ENABLE(channel_char);
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_EP_DIR(channel_char, USB_DIRECTION_IN);
    __write_ch_reg(USB_HCCHAR0, i, channel_char);
    DWCINFO("channel %d disabled", i);
  }
  for (i = 0; i < num_channels; ++i) {
    channel_char = __read_ch_reg(USB_HCCHAR0, i);
    USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_EP_DIR(channel_char, USB_DIRECTION_IN);
    __write_ch_reg(USB_HCCHAR0, i, channel_char);
    do {
      channel_char = __read_ch_reg(USB_HCCHAR0, i);
    } while(USB_HOST_CHAR_GET_CHAN_ENABLE(channel_char));
  }
  DWCINFO("channel initialization completed");
  return ERR_OK;
}
