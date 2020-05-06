#include <board/bcm2835/bcm2835_usb.h>
#include "hcd_hub.h"
#include "hcd_submit.h"
#include "hcd_constants.h"
#include <board_map.h>
#include <reg_access.h>
#include <common.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <bits_api.h>
#include "dwc2.h"
#include "dwc2_regs.h"
#include "dwc2_printers.h"
#include "dwc2_regs_bits.h"
#include <usb/usb.h>
#include <delays.h>
#include <stringlib.h>
#include "hcd.h"
#include "root_hub.h"
#include "usb_dev_rq.h"

//
// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/Arm32_64_USB/rpi-usb.h
//

static int usb_hcd_unique_device_address = 1;
int usb_hcd_print_debug = 0;



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

struct usb_hcd_device *usb_hcd_allocate_device()
{
  struct usb_hcd_device *dev;
  dev = usb_hcd_device_alloc();
  if (dev == NULL)
    return ERR_PTR(ERR_BUSY);
  dev->address = 0;
  dev->state = USB_DEVICE_STATE_ATTACHED;
  dev->location.hub = NULL;
  dev->location.hub_port = 0;
  dev->class = NULL;
  dev->pipe0.address = 0;
  return dev;
}

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

int usb_hcd_get_descriptor(
    struct usb_hcd_pipe *p, 
    int desc_type, 
    int desc_idx,
    int lang_id, 
    void *buf, 
    int buf_sz, 
    int *num_bytes)
{
  int err;
	struct usb_hcd_pipe_control pctl;
  struct usb_descriptor_header header;
  uint64_t rq;

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  if (desc_type != USB_DESCRIPTOR_TYPE_HUB)
    rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));
  else
    rq = USB_DEV_HUB_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));

  err = usb_hcd_submit_cm(p, &pctl, 
      &header, sizeof(header), rq, USB_CONTROL_MSG_TIMEOUT_MS, num_bytes);
  if (err) {
    HCDERR("failed to read descriptor header");
    goto err;
  }

  if (header.descriptor_type != desc_type) {
    err = ERR_GENERIC;
    HCDERR("wrong descriptor type: expected: %d, got:%d", desc_type, header.descriptor_type);
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
      buf, header.length, rq, USB_CONTROL_MSG_TIMEOUT_MS, num_bytes);
  if (err)
    HCDERR("failed to read descriptor header");
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
	int num_bytes = 0;
  struct usb_descriptor_header *header;
  char desc_buffer[256] ALIGNED(4);
  uint16_t lang_ids[96] ALIGNED(4);
  HCDLOG("reading string with index %d", string_index);
  GET_DESC(pipe, STRING,            0,     0,    lang_ids, sizeof(lang_ids)   );
  GET_DESC(pipe, STRING, string_index, 0x409, desc_buffer, sizeof(desc_buffer));

  header = (struct usb_descriptor_header *)desc_buffer;
  HCDLOG("string read to %p, size: %d", header, header->length);
  // hexdump_memory(desc_buffer, header->length);

  wtomb(buf, buf_sz, desc_buffer + 2, header->length - 2);
  buf[((header->length - 2) / 2) - 1] = 0;
  // hexdump_memory(buf, buf_sz);
  HCDLOG("string:%s", buf);
out_err:
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
  // hexdump_memory(cfgbuf, cfgbuf_sz);

  while((void*)hdr < cfgbuf_end && should_continue) {
    HCDLOG("found descriptor:%s(%d),size:%d", 
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

        HCDLOG("-- address:%d,interface:%d,num_endpoints:%d", dev->address, 
          i->descriptor.number,
          i->descriptor.endpoint_count);
        // print_usb_interface_desc(&i->descriptor);
        break;
      case USB_DESCRIPTOR_TYPE_ENDPOINT:
        if (!ep) {
          err = ERR_GENERIC;
          HCDERR("endpoint parsed before interface. Failing."); 
          should_continue = 0;
          break;
        }
        if (ep >= end_ep) {
          HCDERR("endpoint count reached limit %d for current interface", 
              end_ep - i->endpoints);
          break;
        }
			  memcpy(&ep->descriptor, hdr, sizeof(struct usb_endpoint_descriptor));
        HCDLOG("---- address:%d,interface:%d,endpoint:%d,dir:%s,packet_size:%d", 
          dev->address, 
          i->descriptor.number, 
          ep->descriptor.endpoint_address & 0x7f,
          usb_direction_to_string(ep->descriptor.endpoint_address >> 7),
          ep->descriptor.max_packet_size
        );
        // print_usb_endpoint_desc(&ep->descriptor);
        ep++;
        break;
      case USB_DESCRIPTOR_TYPE_HID:
        HCDERR("Unimplemented parsing of USB_DESCRIPTOR_TYPE_HID");
        while(1);
        break;
      case USB_DESCRIPTOR_TYPE_CONFIGURATION:
        HCDLOG("configuration:%d,num_interfaces:%d,string_index:%d", c->configuration_value, 
          c->num_interfaces, c->iconfiguration);
        //print_usb_configuration_desc(c);
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
		.transfer_type = USB_ENDPOINT_TYPE_CONTROL,
		.direction = USB_DIRECTION_OUT,
	};

  uint64_t rq = USB_DEV_RQ_MAKE(SET_CONFIGURATION, SET_CONFIGURATION, configuration, 0, 0);
  err = usb_hcd_submit_cm(pipe, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, 0);
  HCDLOG("completed with status: %d", err);
  return err;
}

#define HCD_ASSERT_DEVICE_STATE_CHANGE(__dev, __from, __to)\
  if (__dev->state != USB_DEVICE_STATE_##__from) {\
    err = ERR_INVAL_ARG;\
    HCDERR("can't promote device %d to "#__to" state from current %s",\
      dev->address, usb_hcd_device_state_to_string(dev->state));\
    goto out_err;\
  }
   
/*
 * At this point device's port has been:
 * - signalled by us to reset via SET_FEATURE FEATURE_RESET
 * - successfully recovered from reset by reporting RESET_CHANGED via GET_STATUS
 * - reported ENABLED status via GET_STATUS
 *
 * Now, we want to promote it to default state.
 * First GET_DESCRIPTOR is performed on default address with minimal packet size of 8 bytes.
 * By spec, we only need 8 bytes of descriptor to get it's max_packet_size.
 */
static int usb_hcd_to_default_state(struct usb_hcd_device *dev, struct usb_hcd_pipe_control *pctl)
{
  const int to_transfer_size = 8;
  int err, num_bytes;
  uint64_t rq;
  struct usb_device_descriptor desc = { 0 };

  if (dev->location.hub)
    HCDLOG("setting DEFAULT state for device at hub:%d.port:%d", 
      dev->location.hub->address, dev->location.hub_port);
  else
    HCDLOG("setting DEFAULT state for root hub device");

  dev->pipe0.address = USB_DEFAULT_ADDRESS;
  dev->pipe0.max_packet_size = USB_DEFAULT_PACKET_SIZE;

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, POWERED, DEFAULT);

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_DEVICE, 0, 0,
      to_transfer_size);
  err = usb_hcd_submit_cm(&dev->pipe0, pctl, 
      &desc, to_transfer_size, rq, USB_CONTROL_MSG_TIMEOUT_MS, &num_bytes);
  CHECK_ERR_SILENT();

  if (num_bytes != to_transfer_size) {
    HCDERR("first GET_DESCRIPTOR request should have returned %d bytes instead of %d",
        to_transfer_size, num_bytes);
    err = ERR_GENERIC;
    goto out_err;
  }

  // print_usb_device_descriptor(&dev_desc);

  dev->pipe0.max_packet_size = desc.max_packet_size_0;
  dev->state = USB_DEVICE_STATE_DEFAULT;
  HCDLOG("device in state DEFAULT: bcd:%04x, class:%s(class:%d,subclass:%d,proto:%d), max_packet_size: %d",
      desc.bcd_usb, 
      usb_device_subclass_to_string(desc.device_class, desc.device_subclass, desc.device_protocol), 
      desc.device_class,
      desc.device_subclass,
      desc.device_protocol, 
      desc.max_packet_size_0);
out_err:
  return err;
}

static inline int usb_hcd_allocate_device_address()
{
  return usb_hcd_unique_device_address++;
}

static int usb_hcd_set_address(struct usb_hcd_pipe *p, uint8_t channel, int address)
{
  int err;
	struct usb_hcd_pipe_control pctl; 
  uint64_t rq = USB_DEV_RQ_MAKE(SET_ADDRESS, SET_ADDRESS, address, 0, 0);

	pctl.channel       = channel;
	pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction     = USB_DIRECTION_OUT;

  err = usb_hcd_submit_cm(p, &pctl, 0, 0, rq, USB_CONTROL_MSG_TIMEOUT_MS, 0);
  HCDLOG("completed with status: %d", err);
  return err;
}

/*
 * By this time device has been set to DEFAULT state.
 * We allocate a unique device address from 127 address space.
 */
static int usb_hcd_to_addressed_state(struct usb_hcd_device *dev, struct usb_hcd_pipe_control *pctl)
{
  int err;
  int device_address;

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, DEFAULT, ADDRESSED);

  /*
   * Ritual routine to support devices that still want to give us
   * last part of device descriptor, that we asked at DEFAULT state.
   */
  HCDLOG("resetting device second time");
  if (dev->location.hub) {
    err = usb_hub_enumerate_port_reset(usb_hcd_device_to_hub(dev->location.hub), 
        dev->location.hub_port);
    CHECK_ERR("failed to reset parent hub port");
  }

  /*
   * send SET_ADDRESS to a device, after which it will respond at this
   * address.
   */
  device_address = usb_hcd_allocate_device_address();
  HCDLOG("setting device address to %d", device_address);

  err = usb_hcd_set_address(&dev->pipe0, pctl->channel, device_address);
  CHECK_ERR("failed to set address");

  dev->address = dev->pipe0.address = device_address;

  wait_msec(10);
  dev->state = USB_DEVICE_STATE_ADDRESSED;
out_err:
  return err;
}

/*
 * The device is reset, enabled and set to respond at it's own unique address 
 * Now we need to read and parse the whole DEVICE_DESCRIPTOR along with 
 * CONFIGURATION_DESCRIPTOR + interface and endpoint descriptors and parse
 * it all. After that we send SET_CONFIGURATION to it and put it in 
 * a final CONFIGURED state, and that will complete it's enumeration
 */
static int usb_hcd_to_configured_state(struct usb_hcd_device *dev, struct usb_hcd_pipe_control *pctl)
{
  int err, num_bytes;
  uint64_t rq;
  struct usb_configuration_descriptor config_desc = { 0 };
  uint8_t config_buffer[1024];
  char buffer[256];
  int config_num;

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, ADDRESSED, CONFIGURED);
  HCDLOG("to configured");

  GET_DESC(&dev->pipe0, DEVICE       , 0, 0, &dev->descriptor, sizeof(dev->descriptor));
  HCDLOG("got full device descriptor: ID:%04x:%04x, num_configurations: %d",
    dev->descriptor.id_vendor,
    dev->descriptor.id_product,
    dev->descriptor.num_configurations);

  GET_DESC(&dev->pipe0, CONFIGURATION, 0, 0, &config_desc    , sizeof(config_desc));
  HCDLOG("got configuration header: total size: %d", config_desc.total_length);

  dev->configuration_string = config_desc.iconfiguration;
  config_num = config_desc.configuration_value;

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, 0, config_desc.total_length);

  err = usb_hcd_submit_cm(&dev->pipe0, pctl, config_buffer,
      config_desc.total_length, rq, USB_CONTROL_MSG_TIMEOUT_MS, &num_bytes);
  CHECK_ERR("failed to get full configuration %d with total_length = %d", 
      config_num, config_desc.total_length);

  if (num_bytes != config_desc.total_length) {
    err = ERR_GENERIC;
    HCDERR("failed to recieve total of requested bytes %d < %d",
        num_bytes, config_desc.total_length);
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
  return err;
}

int usb_hcd_enumerate_device(struct usb_hcd_device *dev)
{
  int err = ERR_OK;

  struct usb_hcd_pipe_control pctl;
  HCDLOG("=============================================================");
  HCDLOG("********************* ENUMERATE DEVICE **********************");
  HCDLOG("=============================================================");

  // http://microsin.net/adminstuff/windows/how-usb-stack-enumerate-device.html
  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  err = usb_hcd_to_default_state(dev, &pctl);
  CHECK_ERR_SILENT();

  err = usb_hcd_to_addressed_state(dev, &pctl);
  CHECK_ERR_SILENT();

  err = usb_hcd_to_configured_state(dev, &pctl);
  CHECK_ERR_SILENT();

  switch(dev->descriptor.device_class) {
    case USB_INTERFACE_CLASS_HUB:
      HCDLOG("HUB: vendor:%04x:product:%04x", dev->descriptor.id_vendor, dev->descriptor.id_product);
      HCDLOG("   : max_packet_size: %d", dev->descriptor.max_packet_size_0);
      err = usb_hub_enumerate(dev);
      CHECK_ERR_SILENT();
      break;
    case USB_INTERFACE_CLASS_HID:
      HCDLOG("HID enumeration not implemented");
      break;
    default:
      break;
  }
out_err:
  HCDLOG("=============================================================");
  HCDLOG("********************* ENUMERATE DEVICE END ******************");
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
    dwc2_init_channels();

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

  /*
   * Defaultly -), our root hub is in not in ADDRESSED state,
   * so it should respond on default address.
   */
  usb_root_hub_device_number = USB_DEFAULT_ADDRESS;
  root_hub_dev->state = USB_DEVICE_STATE_POWERED;
  root_hub_dev->pipe0.speed = USB_SPEED_FULL;
  root_hub_dev->pipe0.max_packet_size = 64;
  // print_usb_device(root_hub_dev);
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
    HCDERR("mbox call failed");
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
    HCDERR("mbox call failed");
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
  usb_hcd_hub_init();

  vendor_id = read_reg(USB_GSNPSID);
  user_id   = read_reg(USB_GUID);
  HCDLOG("USB CHIP: VENDOR:%08x USER:%08x", vendor_id, user_id);

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

static int prefix_padding_to_string(const char *prefix, int depth, char *buf, int buf_sz)
{
  int buf_sz_saved = buf_sz;
  const char padchar = '-';
  char c;

  while(buf_sz > 0) {
    c = *prefix;
    if (!c)
      break;

    *buf++ = c;
    buf_sz--;
    prefix++;
  }
  while(buf_sz > 0 && depth) {
    *buf++ = padchar;
    depth--;
  }
  *buf = 0;
  return buf_sz_saved - buf_sz;
}

static int usb_hcd_device_path_to_string(struct usb_hcd_device *dev, char *buf, int bufsz)
{
  int n = 0;

  if (!bufsz)
    return 0;

  if (dev->location.hub) {
    n = usb_hcd_device_path_to_string(dev->location.hub, buf, bufsz);
    n += snprintf(buf + n, bufsz - n, ".%02d->", dev->location.hub_port);
  }
  n += snprintf(buf + n, bufsz - n, "%02d", dev->address);
  return n;
}

static int usb_hcd_device_to_string_r(struct usb_hcd_device *dev, const char *prefix, char *buf, int bufsz, int depth)
{
  int n = 0;
  struct usb_hcd_device_class_hub *hub;

  n = prefix_padding_to_string(prefix, depth, buf, bufsz);
  n += snprintf(buf + n, bufsz - n, "USB device:");
  n += usb_hcd_device_path_to_string(dev, buf + n, bufsz - n);

  n += snprintf(buf + n, bufsz - n, " %04x:%04x", dev->descriptor.id_vendor, dev->descriptor.id_product);
  n += snprintf(buf + n, bufsz - n, " %d:%d", (dev->descriptor.bcd_usb >> 8) & 0xff , dev->descriptor.bcd_usb & 0xff);
  n += snprintf(buf + n, bufsz - n, " class:%s(%03x)", 
      usb_device_class_to_string(dev->descriptor.device_class), dev->descriptor.device_class);
  n += snprintf(buf + n, bufsz - n, " %d", dev->descriptor.device_subclass);
  n += snprintf(buf + n, bufsz - n, " %d", dev->descriptor.bcd_device);
  return n;

  n += snprintf(buf + n, bufsz - n, "%susb_dev_%d:config:%d,pipe0:addr:%d/ep:%d/pack_size:%d" __endline,
    prefix,
    dev->address,
    dev->configuration,
    dev->pipe0.address,
    dev->pipe0.endpoint,
    dev->pipe0.max_packet_size);
  return n;
  switch (dev->class->device_class) {
   case USB_HCD_DEVICE_CLASS_HUB:
     hub = usb_hcd_device_to_hub(dev);
     n = usb_hcd_hub_device_to_string(hub, prefix, buf + n, bufsz - n);
     break;
   default: break;
  }
  return n;
}

int usb_hcd_device_to_string(struct usb_hcd_device *dev, const char *prefix, char *buf, int bufsz)
{
  return usb_hcd_device_to_string_r(dev, prefix, buf, bufsz, 0);
}

void usb_hcd_print_device(struct usb_hcd_device *dev)
{
  char buf[512];
  usb_hcd_device_to_string(dev, "  ", buf, sizeof(buf));
  printf("%s" __endline, buf);
}
