#include <board/bcm2835/bcm2835_usb.h>
#include <board_map.h>
#include "bcm2835_usb_types.h"
#include <reg_access.h>
#include <common.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <bits_api.h>
#include "dwc2_regs.h"
#include "dwc2_printers.h"
#include "dwc2_regs_bits.h"
#include <usb/usb.h>
#include <usb/usb_printers.h>
#include <delays.h>
#include <stringlib.h>
#include "hcd.h"
#include "root_hub.h"
#include "usb_dev_rq.h"

//
// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/Arm32_64_USB/rpi-usb.h
//

static int usb_hcd_device_id = 0;
static inline uint64_t arm_to_gpu_addr(uint64_t addr)
{
  return RAM_PHY_TO_BUS_UNCACHED(addr);
}

#define HCDLOGPREFIX(__log_level) "[USBHCD "#__log_level"] "
#define HCDLOG(fmt, ...) printf(HCDLOGPREFIX(INFO) fmt __endline, ##__VA_ARGS__)
#define HCDERR(fmt, ...) logf(HCDLOGPREFIX(ERR), fmt, ##__VA_ARGS__)
#define HCDWARN(fmt, ...) logf(HCDLOGPREFIX(WARN), fmt, ##__VA_ARGS__)

#define CHECK_ERR(__msg, ...)\
  if (err != ERR_OK) {\
    HCDLOG("err %d: "__msg, err, ##__VA_ARGS__);\
    goto out_err;\
  }

#define CHECK_ERR_SILENT() if (err != ERR_OK) goto out_err

#define __TO_DESC_HDR(__buf) ((struct usb_descriptor_header *)__buf)

#define __CHECK_DESC_TYPE(__buf, __type)\
  if (__TO_DESC_HDR(__buf)->descriptor_type != USB_DESCRIPTOR_TYPE_## __type) {\
    HCDERR("descriptor mismatch: got %d, wanted %d",\
      usb_descriptor_type_to_string(__TO_DESC_HDR(__buf)->descriptor_type),\
      usb_descriptor_type_to_string(USB_DESCRIPTOR_TYPE_## __type));\
    err = ERR_GENERIC;\
    goto out_err;\
  }

#define GET_DESC(__pipe, __desc_type, __desc_idx, __index, __dst, __dst_sz)\
  err = usb_hcd_get_descriptor(__pipe, USB_DESCRIPTOR_TYPE_## __desc_type, __desc_idx, __index, __dst, __dst_sz, &transferred);\
  CHECK_ERR("failed to get descriptor "#__desc_type);\
  __CHECK_DESC_TYPE(__dst, __desc_type);


#define USB_DEVICE_STATE_DETACHED   0
#define USB_DEVICE_STATE_ATTACHED   1
#define USB_DEVICE_STATE_POWERED    2
#define USB_DEVICE_STATE_DEFAULT    3
#define USB_DEVICE_STATE_ADDRESSED  4
#define USB_DEVICE_STATE_CONFIGURED 5
#define USB_DEVICE_STATE_SUSPENDED  6

static inline const char *usb_hcd_device_state_to_string(int s)
{
  switch(s) {
#define __CASE(__x) case USB_DEVICE_STATE_ ##__x: return #__x
    __CASE(DETACHED);
    __CASE(ATTACHED);
    __CASE(POWERED);
    __CASE(DEFAULT);
    __CASE(ADDRESSED);
    __CASE(CONFIGURED);
    __CASE(SUSPENDED);
#undef __CASE
    default: return "UNKNOWN";
  }
}

#define USB_PAYLOAD_TYPE_ERROR  0
#define USB_PAYLOAD_TYPE_NONE   1
#define USB_PAYLOAD_TYPE_HUB    2
#define USB_PAYLOAD_TYPE_HID    3
#define USB_PAYLOAD_TYPE_MSSTOR 4

#define USB_CHANNEL_INTERRUPT_TRANSFER_COMPLETE      0
#define USB_CHANNEL_INTERRUPT_HALT                   1
#define USB_CHANNEL_INTERRUPT_AHB_ERR                2
#define USB_CHANNEL_INTERRUPT_STALL                  3
#define USB_CHANNEL_INTERRUPT_NACK                   4
#define USB_CHANNEL_INTERRUPT_ACK                    5
#define USB_CHANNEL_INTERRUPT_NOT_YET                6
#define USB_CHANNEL_INTERRUPT_TRANSACTION_ERR        7
#define USB_CHANNEL_INTERRUPT_BABBLE_ERR             8
#define USB_CHANNEL_INTERRUPT_FRAME_OVERRUN          9
#define USB_CHANNEL_INTERRUPT_DATA_TOGGLE_ERR        10
#define USB_CHANNEL_INTERRUPT_BUFFER_NOT_AVAIL       11
#define USB_CHANNEL_INTERRUPT_EXCESSIVE_TRANSMISSION 12
#define USB_CHANNEL_INTERRUPT_FRAMELIST_ROLLOVER     13

#define HS_PHY_IFACE_UNSUP     0
#define HS_PHY_IFACE_UTMI      1
#define HS_PHY_IFACE_ULPI      2
#define HS_PHY_IFACE_UTMI_ULPI 3

#define FS_PHY_IFACE_PHY_0 0
#define FS_PHY_IFACE_DEDIC 1
#define FS_PHY_IFACE_PHY_2 2
#define FS_PHY_IFACE_PHY_3 3

DECL_STATIC_SLOT(struct usb_hcd_device, usb_hcd_device, 12)
DECL_STATIC_SLOT(struct usb_hcd_device_class_hub, usb_hcd_device_class_hub, 12)

static int usb_utmi_initialized = 0;

static inline void print_usb_device(struct usb_hcd_device *dev)
{
  printf("usb_device:parent:(%p:%d),pipe0:(max:%d,spd:%d,ep:%d,address:%d,ls_port:%d,ls_pt:%d)",
      dev->location.hub, 
      dev->location.hub_port,
      dev->pipe0.max_packet_size,
      dev->pipe0.speed,
      dev->pipe0.endpoint,
      dev->pipe0.address,
      dev->pipe0.ls_node_port,
      dev->pipe0.ls_node_point
      );
  puts(__endline);
}

#define USB_HOST_TRANSFER_SIZE_PID_DATA0 0
#define USB_HOST_TRANSFER_SIZE_PID_DATA1 1
#define USB_HOST_TRANSFER_SIZE_PID_DATA2 2
#define USB_HOST_TRANSFER_SIZE_PID_SETUP 3
#define USB_HOST_TRANSFER_SIZE_PID_MDATA 3

#define USB_HOST_TRANSFER_SIZE_VALUE(size, packet_count, packet_id, do_ping)\
   (BITS_PLACE_32(size        , 0 , 19)\
   |BITS_PLACE_32(packet_count, 19, 10)\
   |BITS_PLACE_32(packet_id   , 29,  2)\
   |BITS_PLACE_32(do_ping     , 31,  1))

#define USB_HOST_TRANSFER_SPLT_VALUE(hub_addr, port_addr, split_en)\
  (BITS_PLACE_32(hub_addr, 0, 7)|BITS_PLACE_32(port_addr, 7, 7)|BITS_PLACE_32(split_en, 31, 1))

#define USB_HOST_TRANSFER_CHAR_VALUE(max_pack_sz, ep, ep_dir, low_speed, ep_type, dev_addr, ena)\
  (BITS_PLACE_32(max_pack_sz, 0, 11)\
  |BITS_PLACE_32(ep, 11, 4)\
  |BITS_PLACE_32(ep_dir, 15, 1)\
  |BITS_PLACE_32(low_speed, 17, 1)\
  |BITS_PLACE_32(ep_type, 18, 2)\
  |BITS_PLACE_32(dev_addr, 22, 7)\
  |BITS_PLACE_32(ena, 31, 1))

static inline uint32_t bcm2835_usb_get_xfer(int length, int low_speed, int packet_id, int max_packet_size)
{
  int packet_size = low_speed ? 8 : max_packet_size;
  int packet_count = (length + packet_size  - 1) / packet_size;
  if (!packet_count)
    packet_count = 1;

  return USB_HOST_TRANSFER_SIZE_VALUE(length, packet_count, packet_id, 0);
}

static struct usb_hcd_device *usb_hcd_allocate_device()
{
  struct usb_hcd_device *dev;
  dev = usb_hcd_device_alloc();
  if (dev == NULL)
    return ERR_PTR(ERR_BUSY);
  dev->id = ++usb_hcd_device_id;
  dev->state = USB_DEVICE_STATE_ATTACHED;
  dev->location.hub = NULL;
  dev->location.hub_port = 0;
  dev->class = NULL;
  dev->pipe0.address = dev->id;
  return dev;
}

static int usb_hcd_enumerate_hub(struct usb_hcd_device *dev);

#define INT_FLAG(reg, flag)\
  BIT_IS_SET(reg, USB_CHANNEL_INTERRUPT_ ## flag)

#define PRINT_INTR(reg, flag)\
  if (INT_FLAG(reg, flag))\
    puts(#flag "-")

#define RET_IF_INTR(reg, flag, err)\
  if (INT_FLAG(reg, flag)) {\
    puts(#flag"-");\
    return err;\
  }

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
#define SET_SPLT()      __write_ch_reg(USB_HCSPLT0  , ch, splt); printf("transfer:splt:ch:%d,%08x->%p\n", ch, splt, USB_HCSPLT0 + ch * 8);
#define SET_CHAR()      __write_ch_reg(USB_HCCHAR0  , ch, chr);  printf("transfer:char:ch:%d,%08x->%p\n", ch, chr , USB_HCCHAR0 + ch * 8);
#define SET_SIZ()       __write_ch_reg(USB_HCTSIZ0  , ch, siz);  printf("transfer:size:ch:%d,%08x->%p\n", ch, siz , USB_HCTSIZ0 + ch * 8);
#define SET_DMA()       __write_ch_reg(USB_HCDMA0   , ch, dma);  printf("transfer:dma :ch:%d,%08x->%p\n", ch, dma , USB_HCDMA0  + ch * 8);

#define GET_CHAR()      chr     = __read_ch_reg(USB_HCCHAR0  , ch); printf("transfer:read_chc :%d,%p->%08x\n", ch, USB_HCCHAR0 + ch * 8, chr);
#define GET_SIZ()       siz     = __read_ch_reg(USB_HCTSIZ0  , ch)
#define GET_SPLT()      splt    = __read_ch_reg(USB_HCSPLT0  , ch); printf("transfer:read_splt:%d,%p->%08x\n", ch, USB_HCSPLT0 + ch * 8, splt);
#define GET_INTR()      intr    = __read_ch_reg(USB_HCINT0   , ch); printf("transfer:read_intr:%p->%08x\n", USB_HCINT0 + ch * 8, intr);
#define GET_INTRMSK()   intrmsk = __read_ch_reg(USB_HCINTMSK0, ch)
#define GET_DMA()       dma     = __read_ch_reg(USB_HCDMA0   , ch)

static inline void bcm2835_usb_print_ch_chr(int ch, const char *tag)
{
  uint32_t chr;
  GET_CHAR();
  puts(tag);
  print_usb_host_char(chr);
  puts("\r\n");
}

static inline void bcm2835_usb_print_ch_split(int ch, const char *tag)
{
  uint32_t splt;
  GET_SPLT();
  puts(tag);
  print_usb_host_splt(splt);
  puts("\r\n");
}

static inline void bcm2835_usb_print_ch_intr(int ch, const char *tag)
{
  uint32_t intr;
  GET_INTR();
  puts(tag);
  print_usb_host_intr(intr);
  puts("\r\n");
}

static inline void bcm2835_usb_print_ch_intrmsk(int ch, const char *tag)
{
  uint32_t intrmsk;
  GET_INTRMSK();
  puts(tag);
  print_usb_host_intr(intrmsk);
  puts("\r\n");
}

static inline void bcm2835_usb_print_ch_size(int ch, const char *tag)
{
  uint32_t siz;
  GET_SIZ();
  puts(tag);
  print_usb_host_size(siz);
  puts("\r\n");
}

static inline void bcm2835_usb_print_ch_dma(int ch, const char *tag)
{
  uint32_t dma;
  GET_DMA();
  printf("%s:%08x\r\n", tag, dma);
}

static inline void bcm2835_usb_print_ch_regs(const char *tag, int ch)
{
  puts(tag);
  puts("\r\n");
  bcm2835_usb_print_ch_chr(ch, "char:");
  bcm2835_usb_print_ch_split(ch, "splt:");
  bcm2835_usb_print_ch_intr(ch, "int:");
  bcm2835_usb_print_ch_intr(ch, "intmsk:");
  bcm2835_usb_print_ch_size(ch, "size:");
  bcm2835_usb_print_ch_dma(ch, "dma:");
  puts("\r\n");
}

static inline uint32_t bcm2835_usb_get_split(int hub_addr, int port_addr, int ena)
{
  return USB_HOST_TRANSFER_SPLT_VALUE(hub_addr, port_addr, ena);
}

static inline uint32_t bcm2835_usb_get_chr(int max_pack_sz, int ep, int ep_dir, int speed, int ep_type, int dev_addr, int ena)
{
  return USB_HOST_TRANSFER_CHAR_VALUE(max_pack_sz, ep, ep_dir, speed, ep_type, dev_addr, ena);
}

static int bcm2835_channel_transfer(struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl, 
  void *buf, 
  int bufsz,
  int packet_id,
  int *out_num_bytes) 
{
  int err = ERR_OK;
  int i; 
  int ch = pctl->channel;

  const int max_retries = 8;
  uint32_t chr, splt, intr, siz, dma;
  uint32_t *ptr;
  printf("=======TRANSFER=====================================\n");
  if (pctl->direction == USB_DIRECTION_OUT)
    hexdump_memory(buf, bufsz);

  ch = pctl->channel = 6;
  //pipe->max_packet_size = 64;

  if ((uint64_t)buf & 3) {
    puts("bcm2835_channel_transfer:buffer not aligned to 4 bytes\r\n");
    return ERR_ALIGN;
  }

  /* Clear all existing interrupts. */
  CLEAR_INTR();
  CLEAR_INTRMSK();

  /* Program the channel. */
  chr = 0;
  chr |= (pipe->max_packet_size & 0x7ff) << 0 ;
  chr |= (pipe->endpoint        &   0xf) << 11;
  chr |= (pctl->direction       &     1) << 15;
  chr |= (pipe->speed == USB_SPEED_LOW ? 1 : 0) << 17;
  chr |= (pctl->transfer_type   &     1) << 18;
  chr |= (pipe->address         &  0x7f) << 22;
  SET_CHAR();

  /* Clear and setup split control to low speed devices */
  splt = 0;
 //  splt |= 1 << 31; // split_enable
 //  splt |= (pipe->ls_node_point & 0x7f) << 0;
 //  splt |= (pipe->ls_node_port  & 0x7f) << 7;
  SET_SPLT();

  /* Set transfer size. */
  siz = bcm2835_usb_get_xfer(bufsz, pipe->speed, packet_id, pipe->max_packet_size);
  SET_SIZ();

  ptr = (uint32_t*)buf;
  for (i = 0; i < max_retries; ++i) {
    printf("transfer: try:%d\n", i);
    CLEAR_INTR();
    CLEAR_INTRMSK();
    GET_SPLT();
    BIT_CLEAR_U32(splt, 16);
    SET_SPLT();
    dma = arm_to_gpu_addr((uint64_t)ptr);
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
      GET_INTR();
    } while(!USB_HOST_INTR_GET_HALT(intr));

    if (USB_HOST_INTR_GET_XFER_COMPLETE(intr)) {
      if (USB_HOST_INTR_GET_ACK(intr)) {
        err = ERR_OK;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_AHB_ERR(intr)) {
        HCDERR("AHB error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_STALL(intr)) {
        HCDWARN("STALL");
      }
      if (USB_HOST_INTR_GET_NAK(intr)) {
        HCDWARN("NAK. retrying");
        continue;
      }
      if (USB_HOST_INTR_GET_NYET(intr)) {
        HCDWARN("NYET. retrying.");
        continue;
      }
      if (USB_HOST_INTR_GET_TRNSERR(intr)) {
        HCDERR("transfer error.");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_BABBLERR(intr)) {
        HCDERR("BABBLE error.");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_FRMOVRN(intr)) {
        HCDERR("frame overrun");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_DATTGGLERR(intr)) {
        HCDERR("data toggle error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_DATTGGLERR(intr)) {
        HCDERR("data toggle error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_BUFNOTAVAIL(intr)) {
        HCDERR("buffer not available error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_EXCESSXMIT(intr)) {
        HCDERR("excess transmit error");
        err = ERR_GENERIC;
        goto out_err;
      }
      if (USB_HOST_INTR_GET_FRMLISTROLL(intr)) {
        HCDERR("frame list roll error");
        err = ERR_GENERIC;
        goto out_err;
      }
    }

    GET_SPLT();
  }
out_err:
  GET_SIZ();
  printf("siz:%08x\n", siz);
  *out_num_bytes = bufsz - USB_HOST_SIZE_GET_SIZE(siz);
  printf("=======TRANSFER END=================================\n");
	return err;
}

static int usb_hcd_submit_cm(
  struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl,
  void *buf,
  int buf_sz,
  uint64_t rq,
  int timeout,
  int *out_num_bytes)
{
  int err;
  int num_bytes = 0;
  int num_bytes_last = 0;
	struct usb_hcd_pipe_control int_pctl = *pctl;
  char rq_desc[256];
  usb_rq_get_description(rq, rq_desc, sizeof(rq_desc));
  HCDLOG("usb_hcd_submit_cm:address:%d, req:%s", pipe->address, rq_desc);

	int_pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
	int_pctl.direction     = USB_DIRECTION_OUT;

  if (pipe->address == usb_root_hub_device_number) {
    err = usb_root_hub_process_req(rq, buf, buf_sz, &num_bytes);
    goto out_err;
  }

  /*
   * Send SETUP packet
   */
  err = bcm2835_channel_transfer(pipe, &int_pctl, &rq, sizeof(rq), USB_HCTSIZ0_PID_SETUP, &num_bytes);
  HCDLOG("completed with status: %d, num_bytes: wanted:%d, got:%d", err, sizeof(rq), num_bytes);

  /*
   * Transmit DATA packet
   */
  if (buf) {
    int_pctl.direction = pctl->direction;
    memset(buf, 0x66, buf_sz);
    err = bcm2835_channel_transfer(pipe, &int_pctl, buf, buf_sz, USB_HCTSIZ0_PID_DATA1, &num_bytes);
    HCDLOG("completed with status: %d, num_bytes: wanted:%d, got:%d", err, buf_sz, num_bytes);
    hexdump_memory(buf, 12);
  }

  /*
   * Transmit STATUS packet
   */
  if (!buf || pctl->direction == USB_DIRECTION_OUT)
    int_pctl.direction = USB_DIRECTION_IN;
  else
    int_pctl.direction = USB_DIRECTION_OUT;

  err = bcm2835_channel_transfer(pipe, &int_pctl, buf, 0, USB_HCTSIZ0_PID_DATA1, &num_bytes_last);
  HCDLOG("completed with status: %d, num_bytes: wanted:%d, got:%d", err, 0, num_bytes_last);

out_err:
  if (out_num_bytes) {
    *out_num_bytes = num_bytes;
    HCDLOG("--:%d", *out_num_bytes);
  }

  return ERR_OK;
}

static int usb_hcd_get_descriptor(
    struct usb_hcd_pipe *p, 
    int desc_type, 
    int desc_idx,
    int lang_id, 
    void *buf, 
    int buf_sz, 
    int *bytes_transferred)
{
  int err;
	struct usb_hcd_pipe_control pctl;
  struct usb_descriptor_header header;
  uint64_t rq;

  pctl.channel = 0;
  pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  if (desc_type != USB_DESCRIPTOR_TYPE_HUB)
    rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));
  else
    rq = USB_DEV_HUB_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));

  err = usb_hcd_submit_cm(p, &pctl, 
      &header, sizeof(header), rq, USB_CONTROL_MSG_TIMEOUT_MS, bytes_transferred);
  if (err) {
    HCDERR("failed to read descriptor header:%d", err);
    goto err;
  }

  if (header.descriptor_type != desc_type) {
    HCDERR("wrong descriptor type: expected: %d, got:%d", desc_type, header.descriptor_type);
    err = ERR_GENERIC;
    goto err;
  }

  if (buf_sz < header.length) {
    HCDWARN("shrinking returned descriptor size from %d to %d", header.length, buf_sz);
    header.length = buf_sz;
  }

  if (desc_type != USB_DESCRIPTOR_TYPE_HUB)
    rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, header.length);
  else
    rq = USB_DEV_HUB_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, header.length);
  err = usb_hcd_submit_cm(p, &pctl, 
      buf, header.length, rq, USB_CONTROL_MSG_TIMEOUT_MS, bytes_transferred);
  if (err)
    HCDERR("failed to read descriptor header:%d", err);
err:
  return err;
}

void wtomb(char *buf, size_t buf_sz, char *src, int src_sz)
{
  const char *sptr = src;
  const char *send = src + src_sz;

  char *dptr = buf;
  char *dend = buf + buf_sz;

  if (dptr >= dend)
    return;

  *dptr = 0;

  while (*sptr && (sptr < send) && (dptr < dend)) {
    *(dptr++) = *sptr;
    sptr += 2;
  }
}

int usb_hcd_read_string_descriptor(struct usb_hcd_pipe *pipe, int string_index, char *buf, uint32_t buf_sz)
{
	int err;
	int transferred = 0;
  struct usb_descriptor_header *header;
  char desc_buffer[256] ALIGNED(4);
  uint16_t lang_ids[96] ALIGNED(4);
  HCDLOG("reading string with index %d", string_index);
  GET_DESC(pipe, STRING,            0,     0,    lang_ids, sizeof(lang_ids)   );
  GET_DESC(pipe, STRING, string_index, 0x409, desc_buffer, sizeof(desc_buffer));

  header = (struct usb_descriptor_header *)desc_buffer;
  HCDLOG("string read to %p, size: %d", header, header->length);
  hexdump_memory(desc_buffer, header->length);

  wtomb(buf, buf_sz, desc_buffer + 2, header->length - 2);
  buf[((header->length - 2) / 2) - 1] = 0;
  hexdump_memory(buf, buf_sz);
  HCDLOG("string:%s", buf);
out_err:
  return err;
}

int usb_hcd_hub_read_port_status(struct usb_hcd_pipe *pipe, int port, struct usb_hub_port_status *status)
{
  int err;
	int transferred = 0;
	struct usb_hcd_pipe_control pctl = {
		.channel = 0, //dwc_get_free_channel(),
		.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL,
		.direction = USB_DIRECTION_IN,
	};
  uint64_t rq = USB_DEV_RQ_MAKE(
    port ? USB_RQ_HUB_TYPE_GET_PORT_STATUS : USB_RQ_HUB_TYPE_GET_HUB_STATUS,
    USB_RQ_GET_STATUS,
    0,
    port,
    sizeof(*status)
  );

	err = usb_hcd_submit_cm(
      pipe, &pctl, status, sizeof(*status), rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  if (err != ERR_OK) {
    HCDERR("failed to read port status for port: %d, err:%d", port, err);
    return err;
  }
	if (transferred < sizeof(uint32_t)) {
		HCDERR("failed to read hub device:%i port:%i status\n",
			pipe->address, port);
		return ERR_GENERIC;
	}
	return ERR_OK;
}

int bcm2835_usb_hub_change_port_feature(struct usb_hcd_pipe *pipe, int feature, uint8_t port, int set)
{
  int err;
  int transferred;
	struct usb_hcd_pipe_control pctl = {
		.channel = 0,
		.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL,
		.direction = USB_DIRECTION_OUT,
	};

  uint64_t rq = USB_DEV_RQ_MAKE(
    port ? USB_RQ_HUB_TYPE_SET_PORT_FEATURE : USB_RQ_HUB_TYPE_SET_HUB_FEATURE,
    set ? USB_RQ_SET_FEATURE : USB_RQ_CLEAR_FEATURE,
    feature,
    port,
    0
  );

	err = usb_hcd_submit_cm(pipe, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  if (err != ERR_OK) {
    HCDERR("failed to change hub/port feature: port:%d,feature:%d,err:%d",
      port, feature, err);
    return err;
  }

	return ERR_OK;
}

#define USB_HUB_CLR_FEATURE(__feature, __port)\
  err = bcm2835_usb_hub_change_port_feature(\
      &dev->pipe0, USB_HUB_FEATURE_ ## __feature, __port, 0);\
  if (err) {\
    HCDERR("clear feature "#__feature" failed: port:%d err:%d\n",\
        port, err);\
    goto out_err;\
  }

#define USB_HUB_SET_FEATURE(__feature, __port)\
  err = bcm2835_usb_hub_change_port_feature(\
      &dev->pipe0, USB_HUB_FEATURE_ ## __feature, __port, 1);\
  if (err) {\
    HCDERR("set feature "#__feature" failed: port:%d err:%d\n",\
        port, err);\
    goto out_err;\
  }

int usb_hcd_hub_port_reset(struct usb_hcd_device *dev, uint8_t port)
{
  const int reset_retries = 3;
  const int get_sta_retries = 4;
  int err = ERR_OK;
  int i, j;
	struct usb_hub_port_status port_status ALIGNED(4);

  for (i = 0; i < reset_retries; ++i) {
    USB_HUB_SET_FEATURE(RESET, port + 1);

    for (j = 0; j < get_sta_retries; ++j) {
      wait_msec(20);
      err = usb_hcd_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
      if (err) {
        HCDERR("failed to read port status port: %d, err: %d", port + 1, err);
        goto out_err;
      }

      if (BIT_IS_SET(port_status.changed, USB_PORT_STATUS_CH_BIT_RESET_CHANGED)) {
        HCDLOG("RESET_CHANGED");
        goto out_clear;
      }
      if (BIT_IS_SET(port_status.status, USB_PORT_STATUS_BIT_ENABLED)) {
        HCDLOG("ENABLED");
        goto out_clear;
      }
    } 
  }

out_clear:
  USB_HUB_CLR_FEATURE(RESET_CHANGE, port + 1);
out_err:
  return err;
} 

static int usb_hcd_set_address(struct usb_hcd_pipe *p, uint8_t channel, int address)
{
  int err;
	struct usb_hcd_pipe_control pctl; 
  uint64_t rq = USB_DEV_RQ_MAKE(USB_RQ_TYPE_SET_ADDRESS, USB_RQ_SET_ADDRESS, address, 0, 0);

	pctl.channel       = channel;
	pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
  pctl.direction     = USB_DIRECTION_OUT;

  err = usb_hcd_submit_cm(p, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, 0);
  HCDLOG("completed with status: %d", err);
  return err;
}

static int usb_hcd_parse_configuration(struct usb_hcd_device *dev, const void *cfgbuf, int cfgbuf_sz)
{
  const struct usb_descriptor_header *hdr;

  struct usb_hcd_interface *i = NULL;
  struct usb_hcd_interface *end_i = dev->interfaces + USB_MAX_INTERFACES_PER_DEVICE;
  struct usb_hcd_endpoint *ep = NULL, *end_ep = NULL;
  const struct usb_configuration_descriptor *c = cfgbuf;
  int should_continue = 1;
  int err = ERR_OK;
  void *cfgbuf_end = (char *)cfgbuf + cfgbuf_sz;
  hdr = cfgbuf;

  while((void*)hdr < cfgbuf_end && should_continue) {
    HCDLOG("parsing descriptor:%s(%d),size:%d", 
        usb_descriptor_type_to_string(hdr->descriptor_type), 
        hdr->descriptor_type,
        hdr->length);
    switch(hdr->descriptor_type) {
      case USB_DESCRIPTOR_TYPE_INTERFACE:
        if (!i)
          i = dev->interfaces;
        else
          i++;
        if (i >= end_i) {
          /* VULNURABILITY idea */
          HCDERR("inteface count reached limit %d", end_i - dev->interfaces);
          should_continue = 0;
          break;
        }

        memcpy(&i->descriptor, hdr, sizeof(struct usb_interface_descriptor));
        ep = i->endpoints;
        end_ep = i->endpoints + USB_MAX_ENDPOINTS_PER_DEVICE;

        HCDLOG("parsed new interface: %d/%d", dev->id, i->descriptor.number);
        print_usb_interface_desc(&i->descriptor);
        break;
      case USB_DESCRIPTOR_TYPE_ENDPOINT:
        if (!ep) {
          HCDERR("endpoint parsed before interface. Failing."); 
          err = ERR_GENERIC;
          should_continue = 0;
          break;
        }
        if (ep >= end_ep) {
          HCDERR("endpoint count reached limit %d for current interface", 
              end_ep - i->endpoints);
          break;
        }
			  memcpy(&ep->descriptor, hdr, sizeof(struct usb_endpoint_descriptor));
        HCDLOG("parsed new endpoint:%d/%d/%d at %p", dev->id, i->descriptor.number, ep->descriptor.endpoint_address, ep);
        print_usb_endpoint_desc(&ep->descriptor);
        ep++;
        break;
      case USB_DESCRIPTOR_TYPE_HID:
        HCDERR("Unimplemented parsing of USB_DESCRIPTOR_TYPE_HID");
        while(1);
        break;
      case USB_DESCRIPTOR_TYPE_CONFIGURATION:
        HCDLOG("parsed new configuration:%d, iconfiguration:%d");
        print_usb_configuration_desc(c);
        break;
      default:
        HCDERR("Unimplemented parsing of unknown descriptor type %d", hdr->descriptor_type);
        while(1);
        break;
    }
    hdr = (const struct usb_descriptor_header *)(((char *)hdr) + hdr->length);
  }
  return err;
}

static int usb_hcd_set_configuration(struct usb_hcd_pipe *pipe, uint8_t channel, int configuration)
{
  int err;
	struct usb_hcd_pipe_control pctl = {
		.channel = channel,
		.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL,
		.direction = USB_DIRECTION_OUT,
	};

  uint64_t rq = USB_DEV_RQ_MAKE(USB_RQ_TYPE_SET_CONFIGURATION, USB_RQ_SET_CONFIGURATION, configuration, 0, 0);
  err = usb_hcd_submit_cm(pipe, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, 0);
  HCDLOG("completed with status: %d", err);
  return err;
}

#define HCD_ASSERT_DEVICE_STATE_CHANGE(__dev, __from, __to)\
  if (__dev->state != USB_DEVICE_STATE_##__from) {\
    HCDERR("can't promote device %d to "#__to" state from current %s",\
      dev->id, usb_hcd_device_state_to_string(dev->state));\
    err = ERR_INVAL_ARG;\
    goto out_err;\
  }
   
static int usb_hcd_to_default_state(struct usb_hcd_device *dev, struct usb_hcd_pipe_control *pctl)
{
  const int to_transfer_size = 8;
  int err, transferred;
  uint64_t rq;
  struct usb_device_descriptor dev_desc = { 0 };
  HCDLOG("to default: pipe:%d, root_number:%d", dev->pipe0.address, usb_root_hub_device_number);

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, POWERED, DEFAULT);

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_DEVICE, 0, 0,
      to_transfer_size);
  err = usb_hcd_submit_cm(&dev->pipe0, pctl, 
      &dev_desc, to_transfer_size, rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  CHECK_ERR_SILENT();

  if (transferred != to_transfer_size) {
    HCDERR("first GET_DESCRIPTOR request should have returned %d bytes instead of %d",
        to_transfer_size, transferred);
    err = ERR_GENERIC;
    goto out_err;
  }

  print_usb_device_descriptor(&dev_desc);

  dev->pipe0.max_packet_size = dev_desc.max_packet_size_0;
  dev->state = USB_DEVICE_STATE_DEFAULT;
out_err:
  return ERR_OK;
}

static int usb_hcd_to_addressed_state(struct usb_hcd_device *dev, struct usb_hcd_pipe_control *pctl, int address)
{
  int err;
  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, DEFAULT, ADDRESSED);
  HCDLOG("to addressed: address=%d", address);

  if (dev->location.hub) {
    err = usb_hcd_hub_port_reset(dev->location.hub, 
        dev->location.hub_port);
    CHECK_ERR("failed to reset parent hub port");
  }

  err = usb_hcd_set_address(&dev->pipe0, pctl->channel, address);
  CHECK_ERR("failed to set address");

  dev->pipe0.address = address;
  wait_msec(10);
  dev->state = USB_DEVICE_STATE_ADDRESSED;
out_err:
  return ERR_OK;
}

static int usb_hcd_to_configured_state(struct usb_hcd_device *dev, struct usb_hcd_pipe_control *pctl)
{
  int err, transferred;
  uint64_t rq;
  struct usb_configuration_descriptor config_desc = { 0 };
  uint8_t config_buffer[1024];
  char buffer[256];
  int config_num;

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, ADDRESSED, CONFIGURED);
  HCDLOG("to configured");

  GET_DESC(&dev->pipe0, DEVICE       , 0, 0, &dev->descriptor, sizeof(dev->descriptor));
  GET_DESC(&dev->pipe0, CONFIGURATION, 0, 0, &config_desc    , sizeof(config_desc));
  print_usb_device_descriptor(&dev->descriptor);

  dev->configuration_string = config_desc.iconfiguration;
  config_num = config_desc.configuration_value;

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, 0, config_desc.total_length);

  err = usb_hcd_submit_cm(&dev->pipe0, pctl, config_buffer,
      config_desc.total_length, rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  CHECK_ERR("failed to get full configuration %d with total_length = %d", 
      config_num, config_desc.total_length);

  if (transferred != config_desc.total_length) {
    HCDERR("failed to recieve total of requested bytes %d < %d",
        transferred, config_desc.total_length);
    err = ERR_GENERIC;
    goto out_err;
  }

  err = usb_hcd_parse_configuration(dev, config_buffer, config_desc.total_length);
  CHECK_ERR("failed to parse configuration for device");

  err = usb_hcd_set_configuration(&dev->pipe0, pctl->channel, config_num);
  CHECK_ERR("failed to set configuration %d for device", config_num);

  dev->configuration = config_num;
  dev->state = USB_DEVICE_STATE_CONFIGURED;

#define READ_STRING(__string_idx, __nice_name)\
  if (__string_idx) {\
    err = usb_hcd_read_string_descriptor(&dev->pipe0,\
        __string_idx, buffer, sizeof(buffer));\
    CHECK_ERR("failed to get "#__nice_name" string");\
  }

  READ_STRING(dev->descriptor.i_product, "product");
  READ_STRING(dev->descriptor.i_manufacturer, "manufacturer");
  READ_STRING(dev->descriptor.i_serial_number, "serial number");
  READ_STRING(dev->configuration_string, "configuration");
out_err:
  return ERR_OK;
}

static int usb_hcd_enumerate_device(struct usb_hcd_device *dev)
{
  int err = ERR_OK;
  uint8_t address;

  struct usb_hcd_pipe_control pctl;

  address = dev->pipe0.address;
  dev->pipe0.address = 0;
  dev->pipe0.max_packet_size = 8;

  pctl.channel = 0;
  pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  HCDLOG("enumerating device %p:%d:%d:%d", dev, address, pctl.channel, usb_root_hub_device_number);

  err = usb_hcd_to_default_state(dev, &pctl);
  CHECK_ERR_SILENT();

  err = usb_hcd_to_addressed_state(dev, &pctl, address);
  CHECK_ERR_SILENT();

  err = usb_hcd_to_configured_state(dev, &pctl);
  CHECK_ERR_SILENT();

  switch(dev->descriptor.device_class) {
    case USB_INTERFACE_CLASS_HUB:
      printf("HUB: vendor:%04x:product:%04x\r\n", dev->descriptor.id_vendor, dev->descriptor.id_product);
      printf("   : max_packet_size: %d\r\n", dev->descriptor.max_packet_size_0);
      err = usb_hcd_enumerate_hub(dev);
      CHECK_ERR_SILENT();
      break;
    case USB_INTERFACE_CLASS_HID:
      HCDLOG("HID enumeration not implemented");
      break;
    default:
      break;
  }
  HCDLOG("device enumeration completed");
out_err:
  return err;
}

static int usb_hcd_hub_port_conn_changed(struct usb_hcd_device *dev, int port)
{
  int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);
  struct usb_hcd_device *dev_on_port = NULL;
  err = usb_hcd_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
	if (err != ERR_OK) {
    HCDERR("failed to reset port %d status, err: %d", port, err);
    return err;
  }
  HCDLOG("port_status: %04x:%04x", port_status.status, port_status.changed);
  USB_HUB_CLR_FEATURE(CONNECTION_CHANGE, port + 1);

  err = usb_hcd_hub_port_reset(dev, port);
  if (err != ERR_OK) {
    HCDERR("failed to reset port %d", port);
    goto out_err;
  }

  dev_on_port = usb_hcd_allocate_device();
  dev_on_port->state = USB_DEVICE_STATE_POWERED;
  dev_on_port->location.hub = dev;
  dev_on_port->location.hub_port = port;
  if (!dev_on_port) {
    HCDERR("failed to allocate device for port %d", port);
    goto out_err;
  }

  err = usb_hcd_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
	if (err != ERR_OK) {
    HCDERR("failed to reset port %d status, err: %d", port, err);
    return err;
  }
  HCDLOG("port_status: %04x:%04x", port_status.status, port_status.changed);

	if (BIT_IS_SET(port_status.status, USB_PORT_STATUS_BIT_HIGHSPEED))
    dev_on_port->pipe0.speed = USB_SPEED_HIGH;
  else if (BIT_IS_SET(port_status.status, USB_PORT_STATUS_BIT_LOWSPEED)) {
		dev_on_port->pipe0.speed = USB_SPEED_LOW;
		dev_on_port->pipe0.ls_node_point = dev->pipe0.address;
		dev_on_port->pipe0.ls_node_port = port;
	}
	else 
    dev_on_port->pipe0.speed = USB_SPEED_FULL;

  err = usb_hcd_enumerate_device(dev_on_port);
  if (err != ERR_OK) {
    int saved_err = err;
    HCDERR("failed to enumerate device");
    //usb_deallocate_device(dev_on_port);
    USB_HUB_CLR_FEATURE(ENABLE, port);
    err = saved_err;
    goto out_err;
  }

out_err:
  HCDLOG("completed with status:%d", err);
  return err;
}


int usb_hcd_check_connection(struct usb_hcd_device *dev, int port)
{
	int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);

  err = usb_hcd_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
	if (err != ERR_OK)
    return err;

  HCDLOG("port_status %d: %04x:%04x", port + 1, port_status.status, port_status.changed);
	if (BIT_IS_SET(port_status.changed, USB_PORT_STATUS_CH_BIT_CONNECTED_CHANGED)) {
		HCDLOG("device: %d, port: %d connected changed\n", dev->pipe0.address, port + 1);
		err = usb_hcd_hub_port_conn_changed(dev, port);
    if (err != ERR_OK)
      goto out_err;
	} else {
    HCDERR("LOGIC ERROR: port status not CONNECTED CHANGED");
    while(1);
  }

#define USB_HUB_CHK_AND_CLR(__v, __port, __bit, __feature)\
	if (BIT_IS_SET(__v, USB_PORT_STATUS_BIT_## __bit)) {\
		HCDLOG("bit "#__bit" is set on port:%d. clearing...\n", __port);\
    USB_HUB_CLR_FEATURE(__feature, __port);\
	}

  HCDLOG("ENABLED");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, ENABLED    , ENABLE_CHANGE);
  HCDLOG("SUSPENDED");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, SUSPENDED  , SUSPEND_CHANGE);
  HCDLOG("OVERCURRENT");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, OVERCURRENT, OVERCURRENT_CHANGE);
  HCDLOG("RESET");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, RESET      , RESET_CHANGE);
out_err:
  HCDLOG("completed with status:%d", err);
  return err;
}

static struct usb_hcd_device_class_hub *usb_hcd_allocate_hub()
{
  struct usb_hcd_device_class_hub *hub;
  hub = usb_hcd_device_class_hub_alloc();
  return hub;
}

static int usb_hcd_enumerate_hub(struct usb_hcd_device *dev)
{
  int i, err, transferred;
  struct usb_hub_port_status status ALIGNED(4);
  struct usb_hcd_device_class_hub *hub = NULL;
  struct usb_hub_descriptor *h;
  HCDLOG("=============================================================");
  HCDLOG("===================== ENUMERATE HUB =========================");
  HCDLOG("=============================================================");

  hub = usb_hcd_allocate_hub();
  if (!hub) {
    HCDERR("failed to allocate hub device object");
    err = ERR_BUSY;
    goto out_err;
  }

  dev->class = &hub->base;
  h = &hub->descriptor;

  GET_DESC(&dev->pipe0, HUB, 0, 0, h, sizeof(*h));
  print_usb_hub_descriptor(h);

  print_usb_device_descriptor(&dev->descriptor);

  err = usb_hcd_hub_read_port_status(&dev->pipe0, 0, &status);
  if (err) {
    HCDERR("failed to read hub port status. Enumeration will not continue. err: %d", err);
    goto out_err;
  }

	HCDLOG("powering on ports");
  for (i = 0; i < h->port_count; ++i) {
	  HCDLOG("powering on port %d", i + 1);
    err = bcm2835_usb_hub_change_port_feature(&dev->pipe0, USB_HUB_FEATURE_PORT_POWER, i + 1, 1);
    if (err != ERR_OK)
      goto out_err;
  }
  wait_msec(h->power_good_delay * 2);

  for (i = 0; i < h->port_count; ++i) {
    err = usb_hcd_check_connection(dev, i);
    if (err != ERR_OK)
      goto out_err;
  }

out_err:
  if (err != ERR_OK && hub) {
    usb_hcd_allocate_hub(hub);
    dev->class = NULL;
  }

  HCDLOG("=============================================================");
  HCDLOG("=================== ENUMERATE HUB END =======================");
  HCDLOG("=============================================================");
  return err;
}

static void usb_hcd_reset()
{
  uint32_t rst;
  do {
    rst = read_reg(USB_GRSTCTL);
  } while(!USB_GRSTCTL_GET_AHB_IDLE(rst));

  USB_GRSTCTL_CLR_SET_H_SFT_RST(rst, 1)
  write_reg(USB_GRSTCTL, rst);

  do {
    rst = read_reg(USB_GRSTCTL);
  } while(!USB_GRSTCTL_GET_AHB_IDLE(rst) || USB_GRSTCTL_GET_H_SFT_RST(rst));
  HCDLOG("reset done.");
}

static int bcm2835_usb_recieve_fifo_flush()
{
  uint32_t rst;
  printf("bcm2835_usb_recieve_fifo_flush: started\r\n");
  rst = read_reg(USB_GRSTCTL);
  USB_GRSTCTL_CLR_SET_RXF_FLSH(rst, 1);
  write_reg(USB_GRSTCTL, rst);
  do { 
    rst = read_reg(USB_GRSTCTL);
  } while(USB_GRSTCTL_GET_RXF_FLSH(rst));
  printf("bcm2835_usb_recieve_fifo_flush: completed\r\n");
  return ERR_OK;
}

static int bcm2835_usb_transmit_fifo_flush(int fifo)
{
  uint32_t rst;
  HCDLOG("fifo:%d", fifo);
  rst = read_reg(USB_GRSTCTL);
  USB_GRSTCTL_CLR_SET_TXF_NUM(rst, fifo);
  USB_GRSTCTL_CLR_SET_TXF_FLSH(rst, 1);
  write_reg(USB_GRSTCTL, rst);
  do { 
    rst = read_reg(USB_GRSTCTL);
  } while(USB_GRSTCTL_GET_TXF_FLSH(rst));
  return ERR_OK;
}

static int bcm2835_usb_hwcfg_get_num_host_channels()
{
  uint32_t hwcfg = read_reg(USB_GHWCFG2);
  return USB_GHWCFG2_GET_NUM_HOST_CHAN(hwcfg);
}

static int bcm2835_usb_init_channels()
{
  int i;
  int num_channels;
  uint32_t channel_char;
  num_channels = bcm2835_usb_hwcfg_get_num_host_channels();
  for (i = 0; i < num_channels; ++i) {
    channel_char = __read_ch_reg(USB_HCCHAR0, i);
    USB_HOST_CHAR_CLR_CHAN_ENABLE(channel_char);
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_EP_DIR(channel_char, USB_DIRECTION_IN);
    __write_ch_reg(USB_HCCHAR0, i, channel_char);
    HCDLOG("channel %d disabled", i);
  }
  for (i = 0; i < num_channels; ++i) {
    channel_char = __read_ch_reg(USB_HCCHAR0, i);
    USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_EP_DIR(channel_char, USB_DIRECTION_IN);
    do {
      HCDLOG("waiting until channel %d enabled", i);
      channel_char = __read_ch_reg(USB_HCCHAR0, i);
    } while(USB_HOST_CHAR_GET_CHAN_ENABLE(channel_char));
  }
  HCDLOG("channel initialization completed");
  return ERR_OK;
}

static inline void usb_hcd_print_core_reg_description()
{
  char buf[1024];
  dwc2_get_core_reg_description(buf, sizeof(buf));
  HCDLOG("core registers:"__endline"%s",buf);
}

static const char *hwconfig_hs_iface_to_string(int hs)
{
  switch(hs) {
    case 0: return "UNKNOWN0";
    case 1: return "UTMI";
    case 2: return "ULPI";
    case 3: return "UTMI_ULPI";
    default: return "UNKNOWN";
  }
}

static const char *hwconfig_fs_iface_to_string(int fs)
{
  switch(fs) {
    case 0: return "PHYSICAL0";
    case 1: return "DEDICATED";
    case 2: return "PHYSICAL2";
    case 3: return "PHYSICAL3";
    default: return "UNKNOWN";
  }
}

int usb_hcd_start()
{
  int err = ERR_OK;
  uint32_t ctl;
  uint32_t hwcfg2;
  uint32_t hostcfg;
  uint32_t ahb;
  uint32_t otg;
  uint32_t hostport;
  int hs_phy_iface, fs_phy_iface;

  ctl  = read_reg(USB_GUSBCFG);
  BIT_CLEAR_U32(ctl, USB_GUSBCFG_ULPI_EXT_VBUS_DRV);
  BIT_CLEAR_U32(ctl, USB_GUSBCFG_TERM_SEL_DL_PULSE);
  write_reg(USB_GUSBCFG, ctl);
  usb_hcd_reset();

  if (!usb_utmi_initialized) {
    HCDLOG("initializing USB to UTMI+,no PHY");
    ctl = read_reg(USB_GUSBCFG);
    /* Set mode UTMI */
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_UTMI_SEL);
    /* Disable PHY */
    BIT_CLEAR_U32(ctl, USB_GUSBCFG_PHY_IF);
    write_reg(USB_GUSBCFG, ctl);
    usb_utmi_initialized = 1;
  }

  hwcfg2 = read_reg(USB_GHWCFG2);
  ctl = read_reg(USB_GUSBCFG);

  hs_phy_iface = USB_GHWCFG2_GET_HSPHY_INTERFACE(hwcfg2);
  fs_phy_iface = USB_GHWCFG2_GET_FSPHY_INTERFACE(hwcfg2);

  HCDLOG("HW config: high speed interface:%d(%s)", hs_phy_iface,
      hwconfig_hs_iface_to_string(hs_phy_iface));
  HCDLOG("HW config: full speed interface:%d(%s)", fs_phy_iface,
      hwconfig_fs_iface_to_string(fs_phy_iface));
  if (hs_phy_iface == HS_PHY_IFACE_ULPI && fs_phy_iface == FS_PHY_IFACE_DEDIC) {
    HCDLOG("ULPI: FSLS configuration enabled");
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_FS_LS);
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_CLK_SUS_M);
  } else {
    HCDLOG("ULPI: FSLS configuration disabled");
    BIT_CLEAR_U32(ctl, USB_GUSBCFG_ULPI_FS_LS);
    BIT_CLEAR_U32(ctl, USB_GUSBCFG_ULPI_CLK_SUS_M);
  }
  write_reg(USB_GUSBCFG, ctl);

  ahb = read_reg(USB_GAHBCFG);
  BIT_SET_U32(ahb, USB_GAHBCFG_DMA_EN);
  BIT_CLEAR_U32(ahb, USB_GAHBCFG_DMA_REM_MODE);
  write_reg(USB_GAHBCFG, ahb);
  
  ctl = read_reg(USB_GUSBCFG);
  switch(hwcfg2 & 7) {
    case 0:
      printf("USB: HNP/SRP configuration: HNP,SRP\r\n");
      BIT_SET_U32(ctl, USB_GUSBCFG_SRP_CAP);
      BIT_SET_U32(ctl, USB_GUSBCFG_HNP_CAP);
      break;
    case 1:
    case 3:
    case 5:
      printf("USB: HNP/SRP configuration: SRP\r\n");
      BIT_SET_U32(ctl, USB_GUSBCFG_SRP_CAP);
      BIT_CLEAR_U32(ctl, USB_GUSBCFG_HNP_CAP);
      break;
    case 2:
    case 4:
    case 6:
      printf("USB: HNP/SRP configuration: none\r\n");
      BIT_CLEAR_U32(ctl, USB_GUSBCFG_SRP_CAP);
      BIT_CLEAR_U32(ctl, USB_GUSBCFG_HNP_CAP);
      break;
  }
  write_reg(USB_GUSBCFG, ctl);
  HCDLOG("core started");
  write_reg(USB_PCGCR, 0);
  hostcfg = read_reg(USB_HCFG);
  if (hs_phy_iface == HS_PHY_IFACE_ULPI && fs_phy_iface == FS_PHY_IFACE_DEDIC && BIT_IS_SET(ctl, USB_GUSBCFG_ULPI_FS_LS)) {
    HCDLOG("selecting host clock: 48MHz");
    USB_HCFG_CLR_SET_LS_PHY_CLK_SEL(hostcfg, USB_CLK_48MHZ);
  } else {
    HCDLOG("selecting host clock: 30-60MHz");
    USB_HCFG_CLR_SET_LS_PHY_CLK_SEL(hostcfg, USB_CLK_30_60MHZ);
  }
  USB_HCFG_CLR_SET_LS_SUPP(hostcfg, 1);
  write_reg(USB_HCFG, hostcfg);

  write_reg(USB_GRXFSIZ  , USB_RECV_FIFO_SIZE);
  write_reg(USB_GNPTXFSIZ, (USB_RECV_FIFO_SIZE<<16)|USB_NON_PERIODIC_FIFO_SIZE);
  write_reg(USB_HPTXFSIZ, (USB_PERIODIC_FIFO_SIZE<<16)|(USB_RECV_FIFO_SIZE + USB_NON_PERIODIC_FIFO_SIZE));

	HCDLOG("HNP enabled.");

  otg = read_reg(USB_GOTGCTL);
  BIT_SET_U32(otg, USB_GOTGCTL_HST_SET_HNP_EN);
  write_reg(USB_GOTGCTL, otg);
  HCDLOG("OTG host is set.");

  bcm2835_usb_transmit_fifo_flush(16);
  bcm2835_usb_recieve_fifo_flush();
  if (!USB_HCFG_GET_DMA_DESC_ENA(hostcfg))
    bcm2835_usb_init_channels();

  hostport = read_reg(USB_HPRT);
  if (!USB_HPRT_GET_PWR(hostport)) {
    HCDLOG("host port PWR not set");
    hostport &= USB_HPRT_WRITE_MASK;
    USB_HPRT_CLR_SET_PWR(hostport, 1);
    write_reg(USB_HPRT, hostport);
  }

  hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_WRITE_MASK;
  USB_HPRT_CLR_SET_RST(hostport, 1);
  write_reg(USB_HPRT, hostport);
  wait_msec(60);
  hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_WRITE_MASK;
  USB_HPRT_CLR_RST(hostport);
  write_reg(USB_HPRT, hostport);
  HCDLOG("host controller device started");
  return err;
}

static int usb_hcd_attach_root_hub()
{
  int err;
  struct usb_hcd_device *root_hub_dev;
  HCDLOG("started");
  root_hub_dev = usb_hcd_allocate_device();
  if (IS_ERR(root_hub_dev))
    return PTR_ERR(root_hub_dev);

  usb_root_hub_device_number = 0;
  root_hub_dev->state = USB_DEVICE_STATE_POWERED;
  root_hub_dev->pipe0.speed = USB_SPEED_FULL;
  root_hub_dev->pipe0.max_packet_size = 64;
  print_usb_device(root_hub_dev);
  err = usb_hcd_enumerate_device(root_hub_dev);
  if (err != ERR_OK)
    HCDERR("failed to add root hub. err: %d", err);
  else
    HCDLOG("root hub added");
  return err;
}

int usb_device_power_on()
{
  int err;
  uint32_t exists = 0, power_on = 1;
  err = mbox_set_power_state(MBOX_DEVICE_ID_USB, &power_on, 1, &exists);
  if (err) {
    HCDERR("mbox call failed:%d", err);
    return ERR_GENERIC;
  }
  if (!exists) {
    HCDERR("after mbox call:device does not exist");
    return ERR_GENERIC;
  }
  if (!power_on) {
    HCDERR("after mbox call:device still not powered on");
    return ERR_GENERIC;
  }
  return ERR_OK;
}

int usb_hcd_power_off()
{
  int err;
  uint32_t exists = 0, power_on = 0;
  err = mbox_set_power_state(MBOX_DEVICE_ID_USB, &power_on, 1, &exists);
  if (err) {
    HCDERR("mbox call failed:%d", err);
    return ERR_GENERIC;
  }
  if (!exists) {
    HCDERR("after mbox call:device does not exist");
    return ERR_GENERIC;
  }
  if (power_on) {
    HCDERR("after mbox call:device still powered on");
    return ERR_GENERIC;
  }
  return ERR_OK;
}

void print_usb_device_rq(uint64_t rq, const char *tag)
{
  printf("usb_dev_rq:%s,type:%02x,rq:%02x,value:%04x,idx:%04x,len:%04x",
      tag, 
      rq,
      USB_DEV_RQ_GET_TYPE(rq),
      USB_DEV_RQ_GET_RQ(rq),
      USB_DEV_RQ_GET_VALUE(rq),
      USB_DEV_RQ_GET_INDEX(rq),
      USB_DEV_RQ_GET_LENGTH(rq));
}

int usb_hcd_init()
{
  int err;
  uint32_t vendor_id, user_id;
  STATIC_SLOT_INIT_FREE(usb_hcd_device);
  STATIC_SLOT_INIT_FREE(usb_hcd_device_class_hub);

  vendor_id = read_reg(USB_GSNPSID);
  user_id   = read_reg(USB_GUID);
  HCDLOG("USB CHIP: VENDOR:%08x USER:%08x\r\n", vendor_id, user_id);

  err = usb_device_power_on();
  if (err) {
    goto err;
  }

  wait_msec(20);
  HCDLOG("device powered on");
  usb_hcd_print_core_reg_description();
  // return 0;

  err = usb_hcd_start();
  if (err)
    goto err_power;
  // usb_test();
  err = usb_hcd_attach_root_hub();
  if (err)
    goto err_power;
  return ERR_OK;

err_power:
  if (usb_hcd_power_off() != ERR_OK)
    kernel_panic("Failed to shutdown usb");
err:
  return err;
}

/* Registers at start
usbregs:
OTG     :001c0000, - 18,19,20 USB_GOTGCTL_A_SES_VLD, USB_GOTGCTL_B_SES_VLD
ahbcfg  :00000000, - dma not ena
usbcfg  :00001400, - 10,12 usbtrdtim
rst     :80000000, - 
rxstsr  :10400240,
rxstsp  :10400240,
rxfsiz  :00001000,
nptxfsiz:01001000,
nptxsts :00080100

USB_GHWCFG1:00000000,
USB_GHWCFG2:228ddd50, SRP_HNP_CAP, INTERNAL_DMA, UTMI, DEDICATED, 7 endpoints, 7 channel, periodic EP, dynamic fifo, non-per-queue-depth 2, host-periodic-queue-depth-2, dev-token-queue-depath:2
USB_GHWCFG3:0ff000e8, transfer_size_control_width:8,pack_size_control_width:6,otg:1,fifo_depth:0x0ff0 - 4080
USB_GHWCFG4:1ff00020:  min_ahb_freq, in_ep_count:0111:7,ded_fifo_en:1,session_end_filter_en:1,valid_filt_a,valid_filt_b

Registers after reset / power_on
usbregs:
otg     :001c0000 - same
ahbcfg  :0000000e - bits 123 Axi Bursts Len = 3
usbcfg  :20002700 - 8,9,10,13,29 SRP CAP/HNP CAP/usbtrdtim/force_host_mode
rst     :80000000 - 
rxstsr  :00400040 - 
rxstsp  :00400040
rxfsiz  :00001000
nptxfsiz:01001000
nptxsts :00080100
USB_GHWCFG1:00000000,USB_GHWCFG2:228ddd50,USB_GHWCFG3:0ff000e8,USB_GHWCFG4:1ff00020

*/
