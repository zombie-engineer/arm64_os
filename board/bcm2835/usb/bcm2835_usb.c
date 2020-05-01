#include <board/bcm2835/bcm2835_usb.h>
#include <board_map.h>
#include "bcm2835_usb_types.h"
#include <reg_access.h>
#include <common.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <bits_api.h>
#include "dwc2_regs.h"
#include "dwc2_regs_bits.h"
#include <usb/usb.h>
#include <usb/usb_printers.h>
#include <delays.h>
#include <stringlib.h>
#include "root_hub.h"
#include "usb_dev_rq.h"

//
// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/Arm32_64_USB/rpi-usb.h
//

static inline uint64_t arm_to_gpu_addr(uint64_t addr)
{
  return 0xC0000000 | addr;
}

#define usb_info(fmt, ...) logf(fmt, ##__VA_ARGS__)
#define usb_err(fmt, ...) logf("err:" fmt, ##__VA_ARGS__)
#define usb_warn(fmt, ...) logf("warn:" fmt, ##__VA_ARGS__)

#define GET_DESC(__pipe, __desc_type, __desc_idx, __index, __dst, __dst_sz)\
  err = bcm2835_usb_get_descriptor(__pipe, USB_DESCRIPTOR_TYPE_## __desc_type, __desc_idx, __index, __dst, __dst_sz, &transferred)

#define USB_DEVICE_STATUS_ATTACHED   0
#define USB_DEVICE_STATUS_POWERED    1
#define USB_DEVICE_STATUS_DEFAULT    2
#define USB_DEVICE_STATUS_ADDRESSED  3
#define USB_DEVICE_STATUS_CONFIGURED 4

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

DECL_STATIC_SLOT(struct bcm2835_usb_device, bcm2835_usb_device, 12)
DECL_STATIC_SLOT(struct bcm2835_usb_hub_device, bcm2835_usb_hub_device, 12)

static int usb_phy_initialized = 0;

static inline void print_usb_device(struct bcm2835_usb_device *dev)
{
  printf("usb_device:parent:(%d:%d),pipe0:(max:%d,spd:%d,ep:%d,num:%d,ls_port:%d,ls_pt:%d)",
      dev->parent_hub.number, 
      dev->parent_hub.port_number,
      dev->pipe0.max_packet_size,
      dev->pipe0.speed,
      dev->pipe0.endpoint,
      dev->pipe0.number,
      dev->pipe0.ls_node_port,
      dev->pipe0.ls_node_point
      );
  puts("\r\n");
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

static struct bcm2835_usb_device *bcm2835_usb_allocate_device()
{
  struct bcm2835_usb_device *dev;
  dev = bcm2835_usb_device_alloc();
  if (dev == NULL)
    return ERR_PTR(ERR_BUSY);

  dev->config.status = USB_DEVICE_STATUS_ATTACHED;
  dev->parent_hub.port_number = 0;
  dev->parent_hub.number = 0xff;
  dev->payload_type = USB_PAYLOAD_TYPE_NONE;
  dev->payload.hub = 0;
  return dev;
}

static int bcm2835_usb_enumerate_hub(struct bcm2835_usb_device *dev);

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

int bcm2835_usb_check_xfer_status(uint32_t intr, int is_split, struct bcm2835_usb_xfer_ctrl *xctl) 
{
  RET_IF_INTR(intr, AHB_ERR, ERR_GENERIC); 
  RET_IF_INTR(intr, DATA_TOGGLE_ERR, ERR_GENERIC); 
  RET_IF_INTR(intr, ACK, ERR_OK); 
  if (is_split) {
    xctl->split_tries++;
    if (xctl->split_tries > xctl->split_max_retries)
      return ERR_GENERIC;
  }
  PRINT_INTR(intr, BABBLE_ERR);
  PRINT_INTR(intr, FRAME_OVERRUN);
  PRINT_INTR(intr, NACK);
  PRINT_INTR(intr, STALL);
  PRINT_INTR(intr, TRANSACTION_ERR);
  return ERR_OK;
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
#define SET_SPLT()      __write_ch_reg(USB_HCSPLT0  , ch, splt)
#define SET_CHAR()      __write_ch_reg(USB_HCCHAR0  , ch, chr)
#define SET_SIZ()       __write_ch_reg(USB_HCTSIZ0  , ch, siz)
#define SET_DMA()       __write_ch_reg(USB_HCDMA0   , ch, dma)

#define GET_CHAR()      chr     = __read_ch_reg(USB_HCCHAR0  , ch)
#define GET_SIZ()       siz     = __read_ch_reg(USB_HCTSIZ0  , ch)
#define GET_SPLT()      splt    = __read_ch_reg(USB_HCSPLT0  , ch)
#define GET_INTR()      intr    = __read_ch_reg(USB_HCINT0   , ch)
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


static int xfer(struct xfer_control *x, int ch)
{
  uint32_t chr, splt, siz, dma;
  uint64_t ptr = (uint64_t)(x->src);
  bcm2835_usb_print_ch_regs("xfer_start:", ch);
  CLEAR_INTR();
  CLEAR_INTRMSK();
  chr = bcm2835_usb_get_chr(
      x->max_pack_sz, 
      x->ep, 
      x->ep_dir, 
      x->low_speed, 
      x->ep_type, 
      x->dev_addr, 
      0 /* not enable */);
  SET_CHAR();

  splt = bcm2835_usb_get_split(x->hub_addr, x->port_addr, 1 /* enable */);
  SET_SPLT();

  siz = bcm2835_usb_get_xfer(x->src_sz, x->low_speed, x->pid, x->max_pack_sz);
  SET_SIZ();
  wait_msec(1);
  bcm2835_usb_print_ch_regs("xfer:set_siz:set_char:", 0);

  CLEAR_INTR();
  CLEAR_INTRMSK();

  GET_SPLT();
  USB_HOST_SPLT_CLR_COMPLETE_SPLIT(splt);
  SET_SPLT();
  dma = arm_to_gpu_addr(ptr);
  ptr++;
  SET_DMA();
  wait_msec(1);
  bcm2835_usb_print_ch_regs("xfer:set_siz:set_char:set_dma", 0);

  GET_CHAR();
  USB_HOST_CHAR_CLR_SET_PACK_PER_FRM(chr, 1);
  USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(chr, 1);
  SET_CHAR();
  wait_msec(2);
  bcm2835_usb_print_ch_regs("xfer:set_siz:set_char:xfer_ena", 0);
  printf("xfer:completed\r\n");
  return ERR_OK;
}

static int bcm2835_channel_transfer(struct bcm2835_usb_pipe *pipe,
  struct bcm2835_usb_pipe_control *pctl, 
  void *buf, 
  int bufsz,
  int packet_id) 
{
  int err = ERR_OK;
  int i; 
  int ch = pctl->channel;

  const int max_retries = 8;
  uint32_t chr, splt, intr, siz, dma;
  // uint32_t offset = 0;
  // struct bcm2835_usb_xfer_ctrl xctl = { 0 };
  uint32_t *ptr;
  usb_info("TRANSFER------------------------------\r\n");

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
  chr |= (pctl->transfer_type    &     1) << 18;
  chr |= (pipe->number          &  0x7f) << 22;
  SET_CHAR();

  /* Clear and setup split control to low speed devices */
  splt = 0;
  splt |= 1 << 31; // split_enable
  splt |= (pipe->ls_node_point & 0x7f) << 0;
  splt |= (pipe->ls_node_port  & 0x7f) << 7;
  SET_SPLT();

  /* Set transfer size. */
  siz = bcm2835_usb_get_xfer(bufsz, pipe->speed, packet_id, pipe->max_packet_size);
  SET_SIZ();

  ptr = (uint32_t*)buf;
  for (i = 0; i < max_retries; ++i) {
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
    } while(!BIT_IS_SET(intr, 1));

    GET_SPLT();
  }
//		result = HCDCheckErrorAndAction(tempInt,
//			tempSplit.split_enable, &sendCtrl);						// Check transmisson RESULT and set action flags
//		if (result) LOG("Result: %i Action: 0x%08x tempInt: 0x%08x tempSplit: 0x%08x Bytes sent: %i\n",
//			result, (unsigned int)sendCtrl.Raw32, (unsigned int)tempInt.Raw32, 
//			(unsigned int)tempSplit.Raw32, result ? 0 : DWC_HOST_CHANNEL[pctl.Channel].TransferSize.size);
//		if (sendCtrl.ActionFatalError) return result;				// Fatal error occured we need to bail
//
//		sendCtrl.SplitTries = 0;									// Zero split tries count
//		while (sendCtrl.ActionResendSplit) {						// Decision was made to resend split
//			/* Clear channel interrupts */
//			DWC_HOST_CHANNEL[pctl.Channel].Interrupt.Raw32 = 0xFFFFFFFF;
//			DWC_HOST_CHANNEL[pctl.Channel].InterruptMask.Raw32 = 0x0;
//
//			/* Set we are completing the split */
//			tempSplit = DWC_HOST_CHANNEL[pctl.Channel].SplitCtrl;
//			tempSplit.complete_split = true;						// Set complete split flag
//			DWC_HOST_CHANNEL[pctl.Channel].SplitCtrl = tempSplit;
//
//			/* Launch transmission */
//			tempChar = DWC_HOST_CHANNEL[pctl.Channel].Characteristic;
//			tempChar.channel_enable = true;
//			tempChar.channel_disable = false;
//			DWC_HOST_CHANNEL[pctl.Channel].Characteristic = tempChar;
//
//			// Polling wait on transmission only option right now .. other options soon :-)
//			if (HCDWaitOnTransmissionResult(5000, pctl.Channel, &tempInt) != OK) {
//				LOG("HCD: Request split completion on channel:%i has timed out.\n", pctl.Channel);// Log error
//				return ErrorTimeout;								// Return timeout error
//			}
//
//			tempSplit = DWC_HOST_CHANNEL[pctl.Channel].SplitCtrl;// Fetch the split details again
//			result = HCDCheckErrorAndAction(tempInt,
//				tempSplit.split_enable, &sendCtrl);					// Check RESULT of split resend and set action flags
//			//if (result) LOG("Result: %i Action: 0x%08lx tempInt: 0x%08lx tempSplit: 0x%08lx Bytes sent: %i\n",
//			//	result, sendCtrl.RawUsbSendContol, tempInt.RawInterrupt, tempSplit.RawSplitControl, RESULT ? 0 : DWC_HOST_CHANNEL[pctl.Channel].TransferSize.TransferSize);
//			if (sendCtrl.ActionFatalError) return result;			// Fatal error occured bail
//			if (sendCtrl.LongerDelay) timer_wait(10000);			// Not yet response slower delay
//				else timer_wait(2500);								// Small delay between split resends
//		}
//
//		if (sendCtrl.Success) {										// Send successful adjust buffer position
//			unsigned int this_transfer;
//			this_transfer = DWC_HOST_CHANNEL[pctl.Channel].TransferSize.size;
//			
//			if (((uint32_t)(intptr_t)&buffer[offset] & 3) != 0) {	// Buffer address is unaligned
//
//				// Since our buffer is unaligned for IN endpoints
//				// Copy the data from the the aligned buffer to the buffer
//				// We know the aligned buffer was used because it is unaligned
//				if (pctl.Direction == USB_DIRECTION_IN)
//				{
//					memcpy(&buffer[offset], aligned_bufs[pctl.Channel], this_transfer);
//				}
//			}
//
//			offset = bufferLength - this_transfer;
//		}
//
//	} while (DWC_HOST_CHANNEL[pctl.Channel].TransferSize.packet_count > 0);// Full data not sent
//
	return err;
}

static int bcm2835_usb_submit_control_message(
  struct bcm2835_usb_pipe *pipe,
  struct bcm2835_usb_pipe_control *pctl,
  void *buf,
  int buf_sz,
  uint64_t rq,
  int timeout,
  int *out_num_bytes)
{
  int err;
  int num_bytes = 0;
	struct bcm2835_usb_pipe_control int_pctl = *pctl;
  usb_info("started:rq:%08x,buf_sz:%d", rq, buf_sz);

	int_pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
	int_pctl.direction     = USB_DIRECTION_OUT;

  if (pipe->number == usb_root_hub_device_number)
    err = usb_root_hub_process_req(rq, buf, buf_sz, &num_bytes);
  else
    err = bcm2835_channel_transfer(pipe, &int_pctl, (uint8_t *)&rq, 8, 3);

  if (out_num_bytes)
    *out_num_bytes = num_bytes;

  usb_info("completed with status: %d, num_bytes: %d", err, num_bytes);
  return ERR_OK;
}

static int bcm2835_usb_get_descriptor(
    struct bcm2835_usb_pipe *p, 
    int desc_type, 
    int desc_idx,
    int lang_id, 
    void *buf, 
    int buf_sz, 
    int *bytes_transferred)
{
  int err;
	struct bcm2835_usb_pipe_control pctl;
  struct usb_descriptor_header header;
  uint64_t rq;

  pctl.channel = 0;
  pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));

  err = bcm2835_usb_submit_control_message(p, &pctl, 
      &header, sizeof(header), rq, USB_CONTROL_MSG_TIMEOUT_MS, bytes_transferred);
  if (err) {
    usb_err("failed to read descriptor header:%d", err);
    goto err;
  }

  if (header.descriptor_type != desc_type) {
    usb_err("wrong descriptor type: expected: %d, got:%d", desc_type, header.descriptor_type);
    err = ERR_GENERIC;
    goto err;
  }

  if (buf_sz < header.length) {
    usb_warn("shrinking returned descriptor size from %d to %d", header.length, buf_sz);
    header.length = buf_sz;
  }

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, header.length);
  err = bcm2835_usb_submit_control_message(p, &pctl, 
      buf, header.length, rq, USB_CONTROL_MSG_TIMEOUT_MS, bytes_transferred);
  if (err)
    usb_err("failed to read descriptor header:%d", err);
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

  if (send - sptr <= 2)
    return;
  if (*(sptr++) != 0x24)
    return;
  if (*(sptr++) != 0x03)
    return;

  while (*sptr && (sptr < send) && (dptr < dend)) {
    *(dptr++) = *sptr;
    sptr += 2;
  }
}

int bcm2835_usb_read_string_descriptor(struct bcm2835_usb_pipe *pipe, int string_index, char *buf, uint32_t buf_sz)
{
	int err;
	int transferred = 0;
  struct usb_descriptor_header *header;
  char desc_buffer[256] ALIGNED(4);
  uint16_t lang_ids[96] ALIGNED(4);
  usb_info("reading string with index %d", string_index);
  GET_DESC(pipe, STRING, 0, 0, lang_ids, sizeof(lang_ids));
  if (err)
    return err;

  GET_DESC(pipe, STRING, string_index, 0x409, desc_buffer, sizeof(desc_buffer));
  if (err)
    return err;

  header = (struct usb_descriptor_header *)desc_buffer;
  usb_info("%s", desc_buffer);
  wtomb(buf, buf_sz, desc_buffer, header->length - 2);
  usb_info("completed:%s", buf);
  return ERR_OK;
}

int bcm2835_usb_hub_read_port_status(struct bcm2835_usb_pipe *pipe, int port, struct usb_hub_port_status *status)
{
  int err;
	int transferred = 0;
	struct bcm2835_usb_pipe_control pctl = {
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

	err = bcm2835_usb_submit_control_message(
      pipe, &pctl, status, sizeof(*status), rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  if (err != ERR_OK) {
    usb_err("failed to read port status for port: %d, err:%d", port, err);
    return err;
  }
	if (transferred < sizeof(uint32_t)) {
		usb_err("failed to read hub device:%i port:%i status\n",
			pipe->number, port);
		return ERR_GENERIC;
	}
	return ERR_OK;
}

int bcm2835_usb_hub_change_port_feature(struct bcm2835_usb_pipe *pipe, int feature, uint8_t port, int set)
{
  int err;
  int transferred;
	struct bcm2835_usb_pipe_control pctl = {
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

	err = bcm2835_usb_submit_control_message(pipe, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  if (err != ERR_OK) {
    usb_err("failed to change hub/port feature: port:%d,feature:%d,err:%d",
      port, feature, err);
    return err;
  }

	return ERR_OK;
}

int bcm2835_usb_hub_port_reset(struct bcm2835_usb_device *dev, uint8_t port)
{
  const int max_retries = 3;
  int err;
  int i, j;
	struct usb_hub_port_status port_status ALIGNED(4);
  /* * * */
  uint32_t r;
#define SET(b)\
  r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;\
  BIT_SET_U32(r, USB_HPRT_ ## b);           \
  write_reg(USB_HPRT, r); \
  printf("bit set " #b"-> %08x\r\n", r);      \
  wait_msec(100);

#define CLR(b)\
  r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;\
  BIT_CLEAR_U32(r, USB_HPRT_ ## b);         \
  write_reg(USB_HPRT, r);\
  printf("bit clear" #b"-> %08x\r\n", r);     \
  wait_msec(100);

#define PR(after)\
  printf("HPRT: after " #after ":\r\n");\
  r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;\
  print_usb_hprt(r);\
  puts("\r\n");

  while(1) {
    // SET(RST);
    r = read_reg(USB_PCGCR);
    r |= (1<<5)|1;
    write_reg(USB_PCGCR, r);
    printf("CLOCK:%08x\n", r);
  	wait_msec(10);
    write_reg(USB_PCGCR, 0);
    r = read_reg(USB_HPRT) & ~0x2e;
    BIT_CLEAR_U32(r, USB_HPRT_SUSP);
    BIT_SET_U32(r, USB_HPRT_RST);
    BIT_SET_U32(r, USB_HPRT_PWR);
    write_reg(USB_HPRT, r);
    printf("P1:%08x\n", r);
		wait_msec(60);
 
    hexdump_memory(USB_GOTGCTL, 0x1000);
    r = read_reg(USB_HPRT) & ~0x2e;
   //  BIT_CLEAR_U32(r, USB_HPRT_RST);
    r = 0x1401;
    write_reg(USB_HPRT, r);
    printf("P2:%08x\n", r);
    wait_msec(2500);
    for (i = 0; i < 60; ++i) {
      r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
      printf("%08x\r\n", r);
//      if (r != 0x0021401) {
//        PR(ok);
//        break;
//      }
      wait_msec(100);

    }
  }
  CLR(PWR);
  PR(power_off);
  SET(PWR);
  PR(power_on);
  SET(RST);
  PR(reset_on);
  CLR(RST);
  PR(reset_off);
  SET(ENA);
  PR(ena_on);
  CLR(RST);
  PR(ena_off);


  while(1);
  /* * * */
  for (i = 0; i < max_retries; ++i) {
    usb_info("resetting hub port %d, retry: %d/%d", port, i, max_retries);
    err = bcm2835_usb_hub_change_port_feature(&dev->pipe0, USB_HUB_FEATURE_RESET, port + 1, 1);
    if (err)
      return err;
    usb_info("waiting hub port status changed");
    for (j = 0; j < max_retries; ++j) {
      wait_msec(20);
      err = bcm2835_usb_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
      if (err)
        return err;
      usb_info("got port status: %04x:%04x", port_status.status, port_status.changed);
      if (BIT_IS_SET(port_status.changed, USB_PORT_STATUS_CH_BIT_RESET_CHANGED) || 
          BIT_IS_SET(port_status.status, USB_PORT_STATUS_BIT_ENABLED))
        break;
    } 
  }
  return bcm2835_usb_hub_change_port_feature(&dev->pipe0, USB_HUB_FEATURE_RESET_CHANGE, port + 1, 0);
} 



#define USB_HUB_CLR_FEATURE(__feature, __port)\
  err = bcm2835_usb_hub_change_port_feature(\
      &dev->pipe0, USB_HUB_FEATURE_ ## __feature, __port, 0);\
  if (err) {\
    usb_err("clear feature "#__feature" failed: port:%d err:%d\n",\
        port, err);\
    goto out_err;\
  }

static int bcm2835_usb_set_address(struct bcm2835_usb_pipe *p, uint8_t channel, int address)
{
  int err;
	struct bcm2835_usb_pipe_control pctl; 
  uint64_t rq = USB_DEV_RQ_MAKE(USB_RQ_TYPE_SET_ADDRESS, USB_RQ_SET_ADDRESS, address, 0, 0);

	pctl.channel       = channel;
	pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
  pctl.direction     = USB_DIRECTION_OUT;

  err = bcm2835_usb_submit_control_message(p, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, 0);
  usb_info("completed with status: %d", err);
  return err;
}

static int usb_dev_parse_configuration(struct bcm2835_usb_device *dev, const void *cfgbuf, int cfgbuf_sz)
{
  const struct usb_descriptor_header *hdr;
  struct usb_interface_descriptor *current_iface;
  struct usb_endpoint_descriptor *current_ep;
  const struct usb_configuration_descriptor *current_cfg = cfgbuf;
  int err = ERR_OK;
  uint8_t ep_idx = 0;
  uint8_t hid_num = 0;
  void *cfgbuf_end = (char *)cfgbuf + cfgbuf_sz;
  hdr = cfgbuf;
  dev->max_interface = 0;
  print_usb_configuration_desc(current_cfg);

  while((void*)hdr < cfgbuf_end) {
    usb_info("hdr:type:%d,length:%d", hdr->descriptor_type, hdr->length);
    switch(hdr->descriptor_type) {
      case USB_DESCRIPTOR_TYPE_INTERFACE:
        current_iface = &dev->ifaces[dev->max_interface];
        usb_info("iface:dev:%p,iface:%p", dev, current_iface);
        memcpy(current_iface, hdr, sizeof(*current_iface));
        print_usb_interface_desc(current_iface);
        dev->max_interface++;
        ep_idx = 0;
        break;
      case USB_DESCRIPTOR_TYPE_ENDPOINT:
        current_ep = &dev->eps[dev->max_interface-1][ep_idx];
        usb_info("endpoint:dev:%p:%d/%d,%p", dev, dev->max_interface-1, ep_idx, current_ep);
			  memcpy(current_ep, hdr, sizeof(*current_ep));
        print_usb_endpoint_desc(current_ep);
  			ep_idx++;
        break;
      case USB_DESCRIPTOR_TYPE_HID:
        usb_err("fix HID descriptor");
        while(1);
        if (hid_num == 0) {
//          err = bcm2835_add_hid_payload(dev);
//          if (err != ERR_OK)
//            goto err;
//        }
//        if (hid_num < USB_MAX_HID_PER_DEVICE) {
//          memcpy(&dev->payload.hid->descriptor[hid_num],
//             config_buffer, sizeof(struct usb_hid_descriptor));
//          dev->payload.hid->hid_interface[hid_num] = dev->max_interface - 1;
//          hid_num++;
        }
        break;
      case USB_DESCRIPTOR_TYPE_CONFIGURATION:
        break;
      default:
        usb_err("unknown descriptor type: %d", hdr->descriptor_type);
        return ERR_GENERIC;
        break;
    }
    usb_info("hdr:%p\r\n", hdr);
    hdr = (const struct usb_descriptor_header *)(((char *)hdr) + hdr->length);
  }
  usb_info("completed with status:%d", err);
  return err;
}

static int bcm2835_usb_set_configuration(struct bcm2835_usb_pipe *pipe, uint8_t channel, int configuration)
{
  int err;
	struct bcm2835_usb_pipe_control pctl = {
		.channel = channel,
		.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL,
		.direction = USB_DIRECTION_OUT,
	};
  uint64_t rq = USB_DEV_RQ_MAKE(USB_RQ_TYPE_SET_CONFIGURATION, USB_RQ_SET_CONFIGURATION, configuration, 0, 0);
  err = bcm2835_usb_submit_control_message(pipe, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, 0);
  usb_info("completed with status: %d", err);
  return err;
}

static int bcm2835_usb_enumerate_device(
    struct bcm2835_usb_device *dev, 
    struct bcm2835_usb_device *parent_hub, 
    uint8_t port_num)
{
  int err;
  uint8_t address, config_num;
  int transferred;
  char buffer[256];
  uint8_t config_buffer[1024];

  struct usb_device_descriptor dev_desc = { 0 };
  struct usb_configuration_descriptor config_desc = { 0 };
  struct bcm2835_usb_pipe_control pctl;
  uint64_t rq;

  usb_info("started");
  address = dev->pipe0.number;
  dev->pipe0.number = 0;
  dev->pipe0.max_packet_size = 8;

  pctl.channel = 0;
  pctl.transfer_type = USB_EP_TRANSFER_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_DEVICE, 0, 0, 8);
  err = bcm2835_usb_submit_control_message(&dev->pipe0, &pctl, 
      &dev_desc, sizeof(dev_desc), rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  if (err)
    goto err;

  dev->pipe0.max_packet_size = dev_desc.max_packet_size_0;
  dev->config.status = USB_DEVICE_STATUS_DEFAULT;

  if (parent_hub) {
    err = bcm2835_usb_hub_port_reset(parent_hub, port_num);
    if (err != ERR_OK)
      goto err;
  }

  err = bcm2835_usb_set_address(&dev->pipe0, pctl.channel, address);
  if (err)
    goto err;

  dev->pipe0.number = address;
  wait_msec(10);
  dev->config.status = USB_DEVICE_STATUS_ADDRESSED;
  GET_DESC(&dev->pipe0, DEVICE,        0, 0, &dev->descriptor, sizeof(dev->descriptor));
  if (err) {
    usb_err("failed to get device descriptor, err: %d", err);
    goto err;
  }
  print_usb_device_descriptor(&dev->descriptor);
  GET_DESC(&dev->pipe0, CONFIGURATION, 0, 0, &config_desc, sizeof(config_desc));
  if (err) {
    usb_err("failed to get configuration descriptor, err: %d", err);
    goto err;
  }

  dev->config.string_index = config_desc.iconfiguration;
  config_num = config_desc.configuration_value;

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, 0, config_desc.total_length);

  err = bcm2835_usb_submit_control_message(&dev->pipe0, &pctl, config_buffer,
      config_desc.total_length, rq, USB_CONTROL_MSG_TIMEOUT_MS, &transferred);
  if (err) {
    usb_err("failed to get full configuration %d with total_length = %d", 
        config_num, 
        config_desc.total_length);
    goto err;
  }

  if (transferred != config_desc.total_length) {
    usb_err("failed to recieve total of requested bytes %d < %d",
        transferred, config_desc.total_length);
    err = ERR_GENERIC;
    goto err;
  }

  err = usb_dev_parse_configuration(dev, config_buffer, config_desc.total_length);
  if (err != ERR_OK) {
    usb_err("failed to parse configuration for device");
    goto err;
  }

  err = bcm2835_usb_set_configuration(&dev->pipe0, pctl.channel, config_num);
  if (err != ERR_OK) {
    usb_err("failed to set configuration %d for device", config_num);
    goto err;
  }

  usb_info("configuration set");
  dev->config.index = config_num;
  dev->config.status = USB_DEVICE_STATUS_CONFIGURED;

  if (dev->descriptor.i_product) {
    usb_info("reading string descriptor for product: %d", dev->descriptor.i_product);
    err = bcm2835_usb_read_string_descriptor(&dev->pipe0, dev->descriptor.i_product, buffer, sizeof(buffer));
    if (err != ERR_OK)
      goto err;
  }

  if (dev->descriptor.i_manufacturer != 0) {
    usb_info("reading string descriptor for manufacturer_id: %d", dev->descriptor.i_manufacturer);
    err = bcm2835_usb_read_string_descriptor(&dev->pipe0, dev->descriptor.i_manufacturer, buffer, sizeof(buffer));
    if (err != ERR_OK)
      goto err;
  }

  if (dev->descriptor.i_serial_number != 0) {
    usb_info("reading string descriptor for serial_number: %d", dev->descriptor.i_serial_number);
    err = bcm2835_usb_read_string_descriptor(&dev->pipe0, dev->descriptor.i_serial_number, buffer, sizeof(buffer));
    if (err != ERR_OK)
      goto err;
  }

  if (dev->config.string_index) {
    usb_info("reading config string descriptor: %d", dev->config.string_index);
    err = bcm2835_usb_read_string_descriptor(&dev->pipe0, dev->config.string_index, buffer, sizeof(buffer));
    if (err != ERR_OK)
      goto err;
  }

  if (dev->descriptor.device_class == USB_IFACE_CLASS_HUB) {
    err = bcm2835_usb_enumerate_hub(dev);
    if (err != ERR_OK) {
      usb_err("error enumerating hub:%d", err);
      goto err;
    }
//  } else if (hid_num > 0) {
//    dev->payload.hid->max_hid = hid_num;
//    err = bcm2835_usb_enumerate_hid(dev->pipe0, dev);
//    if (err != ERR_OK)
//      goto err;
  }
  puts("bcm2835_usb_enumerate_devices:completed\r\n");
  return ERR_OK;
err:
  return err;
}

static int bcm2835_usb_hub_port_connection_changed(struct bcm2835_usb_device *dev, uint8_t port)
{
  int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);
  struct bcm2835_usb_device *dev_on_port = NULL;
  err = bcm2835_usb_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
	if (err != ERR_OK) {
    usb_err("failed to reset port %d status, err: %d", port, err);
    return err;
  }
  usb_info("port_status: %04x:%04x", port_status.status, port_status.changed);
  USB_HUB_CLR_FEATURE(CONNECTION_CHANGE, port + 1);

  err = bcm2835_usb_hub_port_reset(dev, port);
  if (err != ERR_OK) {
    usb_err("failed to reset port %d", port);
    goto out_err;
  }

  dev_on_port = bcm2835_usb_allocate_device();
  if (!dev_on_port) {
    usb_err("failed to allocate device for port %d", port);
    goto out_err;
  }

  err = bcm2835_usb_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
	if (err != ERR_OK) {
    usb_err("failed to reset port %d status, err: %d", port, err);
    return err;
  }
  usb_info("port_status: %04x:%04x", port_status.status, port_status.changed);

	if (BIT_IS_SET(port_status.status, USB_PORT_STATUS_BIT_HIGHSPEED))
    dev_on_port->pipe0.speed = USB_SPEED_HIGH;
  else if (BIT_IS_SET(port_status.status, USB_PORT_STATUS_BIT_LOWSPEED)) {
		dev_on_port->pipe0.speed = USB_SPEED_LOW;
		dev_on_port->pipe0.ls_node_point = dev->pipe0.number;
		dev_on_port->pipe0.ls_node_port = port;
	}
	else 
    dev_on_port->pipe0.speed = USB_SPEED_FULL;

	dev_on_port->parent_hub.number = dev->pipe0.number;
	dev_on_port->parent_hub.port_number = port;

  err = bcm2835_usb_enumerate_device(dev_on_port, dev, port);
  if (err != ERR_OK) {
    int saved_err = err;
    usb_err("failed to enumerate device");
    //usb_deallocate_device(dev_on_port);
    USB_HUB_CLR_FEATURE(ENABLE, port);
    err = saved_err;
    goto out_err;
  }

out_err:
  usb_info("completed with status:%d", err);
  return err;
}


int bcm2835_usb_hub_check_connection(struct bcm2835_usb_device *dev, uint8_t port)
{
	int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);
  usb_info("check connection for port: %d", port + 1);

  err = bcm2835_usb_hub_read_port_status(&dev->pipe0, port + 1, &port_status);
	if (err != ERR_OK)
    return err;

  usb_info("port_status %d: %04x:%04x", port + 1, port_status.status, port_status.changed);
	if (BIT_IS_SET(port_status.changed, USB_PORT_STATUS_BIT_CONNECTED)) {
		usb_info("device: %d, port: %d connected changed\n", dev->pipe0.number, port + 1);
		err = bcm2835_usb_hub_port_connection_changed(dev, port);
    if (err) {
      goto out_err;
    }
	} else {
    printf("logic");
    while(1);
  }

#define USB_HUB_CHK_AND_CLR(__v, __port, __bit, __feature)\
	if (BIT_IS_SET(__v, USB_PORT_STATUS_BIT_## __bit)) {\
		usb_info("bit "#__bit" is set on port:%d. clearing...\n", __port);\
    USB_HUB_CLR_FEATURE(__feature, __port);\
	}

  usb_info("ENABLED");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, ENABLED    , ENABLE_CHANGE);
  usb_info("SUSPENDED");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, SUSPENDED  , SUSPEND_CHANGE);
  usb_info("OVERCURRENT");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, OVERCURRENT, OVERCURRENT_CHANGE);
  usb_info("RESET");
  USB_HUB_CHK_AND_CLR(port_status.changed, port, RESET      , RESET_CHANGE);
    //		if (BIT_IS_CLEAR(port_status.status, USB_PORT_STATUS_BIT_ENABLED) && 
    //        BIT_IS_SET(port_status.status, USB_PORT_STATUS_BIT_CONNECTED) &&
    //        data->children[port] != NULL) {
    //			usb_err("something is not right");
    //  		err = bcm2835_usb_hub_port_connection_changed(dev, port);
    //      if (err) {
    //  		  usb_err("device:%i, port: %i connected changed err:%d\n", dev->pipe0.number, port, err);
    //        return err;
    //      }
    //		}


out_err:
  usb_info("completed with status:%d", err);
  return err;
}

int bcm2835_usb_hub_add_payload(struct bcm2835_usb_device *dev)
{
  struct bcm2835_usb_hub_device *hub;
  hub = bcm2835_usb_hub_device_alloc();
  if (!hub)
    return ERR_BUSY;
  dev->payload_type = USB_PAYLOAD_TYPE_HUB;
  dev->payload.hub = hub;
  return ERR_OK;
}


static int bcm2835_usb_enumerate_hub(struct bcm2835_usb_device *dev)
{
  int i, err, transferred;
  struct usb_hub_descriptor hub ALIGNED(4);
  struct usb_hub_port_status status ALIGNED(4);
  struct usb_hub_descriptor *h = &hub;
  usb_info("enumerate:====================================================");
  h = &hub;

  GET_DESC(&dev->pipe0, HUB, 0, 0, h, sizeof(*h));
  if (err) {
    usb_err("failed to get device descriptor, err: %d", err);
    return err;
  }
  print_usb_hub_descriptor(h);

  print_usb_device_descriptor(&dev->descriptor);

  err = bcm2835_usb_hub_read_port_status(&dev->pipe0, 0, &status);
  if (err) {
    usb_err("failed to read hub port status. Enumeration will not continue. err: %d", err);
    return err;
  }

	usb_info("powering on ports");
  for (i = 0; i < h->port_count; ++i) {
	  usb_info("powering on port %d", i + 1);
    err = bcm2835_usb_hub_change_port_feature(&dev->pipe0, USB_HUB_FEATURE_PORT_POWER, i + 1, 1);
    if (err != ERR_OK)
      return err;
    wait_msec(h->power_good_delay * 2);
  }

  for (i = 0; i < h->port_count; ++i)
    bcm2835_usb_hub_check_connection(dev, i);

  return ERR_OK;
}

static int bcm2835_usb_reset()
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
  usb_info("reset done.");
  return ERR_OK;
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
  usb_info("fifo:%d", fifo);
  rst = read_reg(USB_GRSTCTL);
  USB_GRSTCTL_CLR_SET_TXF_NUM(rst, fifo);
  USB_GRSTCTL_CLR_SET_TXF_FLSH(rst, 1);
  write_reg(USB_GRSTCTL, rst);
  do { 
    rst = read_reg(USB_GRSTCTL);
  } while(USB_GRSTCTL_GET_TXF_FLSH(rst));
  return ERR_OK;
}

static void bcm2835_usb_print_basic_regs()
{
  printf("usbregs:OTG:%08x,AHBCFG:%08x,USBCFG:%08x,RST:%08x,\r\nRXSTSR:%08x,RXSTSP:%08x,RXFSIZ:%08x,NPTXFSIZ:%08x,NPTXSTS:%08x\r\n",
    read_reg(USB_GOTGCTL),
    read_reg(USB_GAHBCFG),
    read_reg(USB_GUSBCFG),
    read_reg(USB_GRSTCTL),
    read_reg(USB_GRXSTSR),
    read_reg(USB_GRXSTSP),
    read_reg(USB_GRXFSIZ),
    read_reg(USB_GNPTXFSIZ),
    read_reg(USB_GNPTXSTS));
  puts("HCFG:");
  print_usb_hcfg(read_reg(USB_HCFG));
  puts("\r\n");
}

static void bcm2835_usb_print_hw_regs()
{
  usb_info("USB_GHWCFG1:%08x,USB_GHWCFG2:%08x,USB_GHWCFG3:%08x,USB_GHWCFG4:%08x",
    read_reg(USB_GHWCFG1),
    read_reg(USB_GHWCFG2),
    read_reg(USB_GHWCFG3),
    read_reg(USB_GHWCFG4));
  print_usb_ghwcfg2(read_reg(USB_GHWCFG2));
  puts("\r\n");
  print_usb_ghwcfg3(read_reg(USB_GHWCFG3));
  puts("\r\n");
  print_usb_ghwcfg4(read_reg(USB_GHWCFG4));
  puts("\r\n");
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
    usb_info("channel %d disabled", i);
  }
  for (i = 0; i < num_channels; ++i) {
    channel_char = __read_ch_reg(USB_HCCHAR0, i);
    USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(channel_char, 1);
    USB_HOST_CHAR_CLR_SET_EP_DIR(channel_char, USB_DIRECTION_IN);
    do {
      usb_info("waiting until channel %d enabled", i);
      channel_char = __read_ch_reg(USB_HCCHAR0, i);
    } while(USB_HOST_CHAR_GET_CHAN_ENABLE(channel_char));
  }
  usb_info("channel initialization completed");
  return ERR_OK;
}

int bcm2835_usb_start()
{
  int err;
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
  err = bcm2835_usb_reset();

  if (!usb_phy_initialized) {
    usb_info("PHY not initialized yet. Initializing...");
    ctl = read_reg(USB_GUSBCFG);
    /* Set mode UTMI */
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_UTMI_SEL);
    /* Disable PHY */
    BIT_CLEAR_U32(ctl, USB_GUSBCFG_PHY_IF);
    write_reg(USB_GUSBCFG, ctl);
    usb_phy_initialized = 1;
  }

  hwcfg2 = read_reg(USB_GHWCFG2);
  ctl = read_reg(USB_GUSBCFG);

  bcm2835_usb_print_basic_regs();
  bcm2835_usb_print_hw_regs();
  hs_phy_iface = USB_GHWCFG2_GET_HSPHY_INTERFACE(hwcfg2);
  fs_phy_iface = USB_GHWCFG2_GET_FSPHY_INTERFACE(hwcfg2);

  usb_info("HS:%d,FS:%d", hs_phy_iface, fs_phy_iface);
  if (hs_phy_iface == HS_PHY_IFACE_ULPI && fs_phy_iface == FS_PHY_IFACE_DEDIC) {
    usb_info("ULPI: FSLS configuration enabled");
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_FS_LS);
    BIT_SET_U32(ctl, USB_GUSBCFG_ULPI_CLK_SUS_M);
  } else {
    usb_info("ULPI: FSLS configuration disabled");
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
  usb_info("core started");
  write_reg(USB_PCGCR, 0);
  hostcfg = read_reg(USB_HCFG);
  if (hs_phy_iface == HS_PHY_IFACE_ULPI && fs_phy_iface == FS_PHY_IFACE_DEDIC && BIT_IS_SET(ctl, USB_GUSBCFG_ULPI_FS_LS)) {
    usb_info("selecting host clock: 48MHz");
    USB_HCFG_CLR_SET_LS_PHY_CLK_SEL(hostcfg, USB_CLK_48MHZ);
  } else {
    usb_info("selecting host clock: 30-60MHz");
    USB_HCFG_CLR_SET_LS_PHY_CLK_SEL(hostcfg, USB_CLK_30_60MHZ);
  }
  USB_HCFG_CLR_SET_LS_SUPP(hostcfg, 1);
  write_reg(USB_HCFG, hostcfg);

  write_reg(USB_GRXFSIZ, USB_RECV_FIFO_SIZE);
  write_reg(USB_GNPTXFSIZ, (USB_RECV_FIFO_SIZE<<16)|USB_NON_PERIODIC_FIFO_SIZE);
  write_reg(USB_HPTXFSIZ, (USB_PERIODIC_FIFO_SIZE<<16)|(USB_RECV_FIFO_SIZE + USB_NON_PERIODIC_FIFO_SIZE));

	usb_info("HNP enabled.");

  otg = read_reg(USB_GOTGCTL);
  BIT_SET_U32(otg, USB_GOTGCTL_HST_SET_HNP_EN);
  write_reg(USB_GOTGCTL, otg);
  usb_info("OTG host is set.");

  bcm2835_usb_transmit_fifo_flush(16);
  bcm2835_usb_recieve_fifo_flush();
  if (!USB_HCFG_GET_DMA_DESC_ENA(hostcfg))
    bcm2835_usb_init_channels();

  hostport = read_reg(USB_HPRT);
  if (!USB_HPRT_GET_PWR(hostport)) {
    usb_info("host port PWR not set");
    hostport &= USB_HPRT_BIT_CLEAR;
    USB_HPRT_CLR_SET_PWR(hostport, 1);
    write_reg(USB_HPRT, hostport);
  }

  hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_BIT_CLEAR;
  USB_HPRT_CLR_SET_RST(hostport, 1);
  write_reg(USB_HPRT, hostport);
  wait_msec(60);
  hostport = read_reg(USB_HPRT);
  hostport &= USB_HPRT_BIT_CLEAR;
  USB_HPRT_CLR_RST(hostport);
  write_reg(USB_HPRT, hostport);
  usb_info("host controller device started");
  return err;
}

int bcm2835_usb_enumerate_hid (struct bcm2835_usb_pipe pipe, struct bcm2835_usb_device *dev) 
{
	// volatile uint8_t hi;
	// volatile uint8_t lo;
	// uint8_t buf[1024];
  // int interface;
  int i;
  int err = 0;
	for (i = 0; i < dev->payload.hid->max_hid; i++) {
	  //	hi = *(uint8_t*)&dev->payload.hid->descriptor[i].hid_version_hi;
		// lo = *(uint8_t*)&dev->payload.hid->descriptor[i].hid_version_lo;
		// interface = dev->payload.hid->hid_interface[i];
    // err = bcm2835_usb_hid_read_descriptor(pipe.number, i, buf, sizeof(buf));
    if (err)
      break;
	}
	return err;
}

//static int bcm2835_add_hid_payload()
//{
//  return ERR_OK;
//}

static int bcm2835_usb_attach_root_hub()
{
  int err;
  struct bcm2835_usb_device *root_hub_dev;
  usb_info("started");
  root_hub_dev = bcm2835_usb_allocate_device();
  if (IS_ERR(root_hub_dev))
    return PTR_ERR(root_hub_dev);

  root_hub_dev->pipe0.number = usb_root_hub_device_number;
  root_hub_dev->config.status = USB_DEVICE_STATUS_POWERED;
  root_hub_dev->pipe0.speed = USB_SPEED_FULL;
  root_hub_dev->pipe0.max_packet_size = 64;
  print_usb_device(root_hub_dev);
  err = bcm2835_usb_enumerate_device(root_hub_dev, NULL, 0);
  usb_info("completed");
  return err;
}

int bcm2835_usb_power_on()
{
  int err;
  uint32_t exists = 0, power_on = 1;
  err = mbox_set_power_state(MBOX_DEVICE_ID_USB, &power_on, 1, &exists);
  if (err) {
    logf("mbox call failed:%d\r\n", err);
    return ERR_GENERIC;
  }
  if (!exists) {
    logf("after mbox call:device does not exist\r\n");
    return ERR_GENERIC;
  }
  if (!power_on) {
    logf("after mbox call:device still not powered on\r\n");
    return ERR_GENERIC;
  }
  logf("device powered on\r\n");
  return ERR_OK;
}

int bcm2835_usb_power_off()
{
  int err;
  uint32_t exists = 0, power_on = 0;
  err = mbox_set_power_state(MBOX_DEVICE_ID_USB, &power_on, 1, &exists);
  if (err) {
    printf("bcm2835_usb_power_off:mbox call failed:%d\r\n", err);
    return ERR_GENERIC;
  }
  if (!exists) {
    puts("bcm2835_usb_power_off:after mbox call:device does not exist\r\n");
    return ERR_GENERIC;
  }
  if (power_on) {
    puts("bcm2835_usb_power_off:after mbox call:device still powered on\r\n");
    return ERR_GENERIC;
  }
  puts("bcm2835_usb_power_off:device powered off\r\n");
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

void usb_test()
{
  int ch;
  struct xfer_control x = { 0 };
  uint64_t rq = USB_DEV_RQ_MAKE(
    USB_RQ_TYPE_GET_DESCRIPTOR,
    USB_RQ_GET_DESCRIPTOR,
    (USB_DESCRIPTOR_TYPE_DEVICE<<8),
    0,
    8);
  printf("usb_test:started\r\n");

  print_usb_device_rq(rq, "r");

  x.max_pack_sz = size_to_usb_packet_size(USB_PACKET_SIZE_8);
  x.pid         = USB_PID_TYPE_SETUP;
  x.ep          = 0;
  x.ep_dir      = USB_DIRECTION_OUT;
  x.ep_type     = USB_EP_TRANSFER_TYPE_CONTROL;
  x.low_speed   = 0;
  x.dev_addr    = 0;
  x.hub_addr    = 0;
  x.port_addr   = 0;
  x.src         = &rq;
  x.src_sz      = sizeof(rq);

  for (ch = 0; ch < BCM2835_USB_NUM_CHANNELS; ++ch) {
    xfer(&x, ch);
  }
  while(1);
}

int bcm2835_usb_init()
{
  int err, i;
  uint32_t core, vendor_id, user_id;
  STATIC_SLOT_INIT_FREE(bcm2835_usb_device);
  for (i = 0; i < ARRAY_SIZE(__STATIC_SLOT_LIST(bcm2835_usb_device)); ++i) {
    __STATIC_SLOT_LIST(bcm2835_usb_device)[i].pipe0.number = i + 1;
    logf("init usb_device:%d", i);
  }
  STATIC_SLOT_INIT_FREE(bcm2835_usb_hub_device);

  vendor_id = read_reg(USB_GSNPSID);
  user_id = read_reg(USB_GUID);
  core = read_reg(USB_GUSBCFG);
  logf("vendor_id:%08x,user_id:%08x\r\n", vendor_id, user_id);
  logf("core_ctrl:%08x\r\n", core);
  err = bcm2835_usb_power_on();
  if (err)
    goto err;

  wait_msec(20);
  usb_info("powered_on");
  bcm2835_usb_print_basic_regs();
  bcm2835_usb_print_hw_regs();
  return 0;

  err = bcm2835_usb_start();
  if (err)
    goto err_power;
  // usb_test();
  err = bcm2835_usb_attach_root_hub();
  if (err)
    goto err_power;
  return ERR_OK;

err_power:
  if (bcm2835_usb_power_off() != ERR_OK)
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
