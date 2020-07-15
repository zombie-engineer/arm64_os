#include <drivers/usb/hcd.h>
#include <board/bcm2835/bcm2835_usb.h>
#include <drivers/usb/hcd_hub.h>
#include <drivers/usb/hcd_hid.h>
#include "hcd_constants.h"
#include <board_map.h>
#include <reg_access.h>
#include <common.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <bits_api.h>
#include "dwc2.h"
#include <usb/usb.h>
#include <delays.h>
#include <stringlib.h>
#include "root_hub.h"
#include <drivers/usb/usb_dev_rq.h>
#include <drivers/usb/usb_mass_storage.h>
#include "dwc2_log.h"
#include <mem_access.h>
#include <sched.h>
#include <intr_ctl.h>
#include <board/bcm2835/bcm2835_irq.h>
#include <irq.h>
#include <drivers/usb/usb_xfer_queue.h>

//
// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/Arm32_64_USB/rpi-usb.h
//

static int usb_hcd_unique_device_address = 1;

static bool powered_on = 0;

int usb_hcd_log_level = 0;

int usb_hcd_set_log_level(int level)
{
  int old_level = usb_hcd_log_level;
  usb_hcd_log_level = level;
  return old_level;
}

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

DECL_STATIC_SLOT(struct usb_hcd_device, usb_hcd_device, 12)

static int usb_utmi_initialized = 0;

static inline void print_usb_device(struct usb_hcd_device *dev)
{
  printf("usb_device:parent:(%p:%d),pipe0:(max:%d,spd:%d,ep:%d,address:%d,hubaddr:%d,hubport:%d)",
      dev->location.hub,
      dev->location.hub_port,
      dev->pipe0.max_packet_size,
      dev->pipe0.speed,
      dev->pipe0.ep,
      dev->pipe0.address,
      dev->pipe0.ls_hub_address,
      dev->pipe0.ls_hub_port
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
  INIT_LIST_HEAD(&dev->hub_children);
  return dev;
}

void usb_hcd_deallocate_device(struct usb_hcd_device *d)
{
  if (d->class) {
    switch(d->class->device_class) {
      case USB_HCD_DEVICE_CLASS_HUB:
        usb_hcd_hub_destroy(usb_hcd_device_to_hub(d));
        break;
      default:
        break;
    }
  }
  usb_hcd_device_release(d);
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
  struct usb_descriptor_header header ALIGNED(64);
  uint64_t rq;

  if (desc_type != USB_DESCRIPTOR_TYPE_HUB)
    rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));
  else
    rq = USB_DEV_HUB_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));


  err = HCD_TRANSFER_CONTROL(p, USB_DIRECTION_IN,
      &header, sizeof(header), rq, num_bytes);
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
  else if (desc_type == USB_DESCRIPTOR_TYPE_HID_REPORT)
    rq = USB_DEV_HID_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, sizeof(header));
  else
    rq = USB_DEV_HUB_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, lang_id, header.length);
  err = HCD_TRANSFER_CONTROL(p, USB_DIRECTION_IN,
      buf, header.length, rq, num_bytes);
  if (err)
    HCDERR("failed to read descriptor header");
err:
  return err;
}

int usb_hcd_read_string_descriptor(struct usb_hcd_pipe *pipe, int string_index, char *buf, uint32_t buf_sz)
{
	int err;
	int num_bytes = 0;
  struct usb_descriptor_header *header;
  char desc_buffer[256] ALIGNED(4);
  uint16_t lang_ids[96] ALIGNED(4);
  // HCDLOG("reading string with index %d", string_index);
  GET_DESC(pipe, STRING,            0,     0,    lang_ids, sizeof(lang_ids)   );
  GET_DESC(pipe, STRING, string_index, 0x409, desc_buffer, sizeof(desc_buffer));

  header = (struct usb_descriptor_header *)desc_buffer;
  // HCDLOG("string read to %p, size: %d", header, header->length);
  // hexdump_memory(desc_buffer, header->length);

  wtomb(buf, buf_sz, desc_buffer + 2, header->length - 2);
  // buf[((header->length - 2) / 2) - 1] = 0;
  // hexdump_memory(buf, buf_sz);
  // HCDLOG("string:%s", buf);
out_err:
  return err;
}

static int usb_hcd_parse_handle_hid(struct usb_hcd_device *dev, struct usb_hid_descriptor *hid_desc)
{
  int err = ERR_OK;
  struct usb_hcd_device_class_hid *h;
  struct usb_hcd_hid_interface *i;

  if (!dev->class) {
    h = usb_hcd_allocate_hid();
    if (!h) {
      err = ERR_BUSY;
      HCDERR("failed to allocate hid device");
      goto out_err;
    }
    h->d = dev;
    dev->class = &h->base;
  } else
    h = usb_hcd_device_to_hid(dev);

  if (dev->class->device_class != USB_HCD_DEVICE_CLASS_HID) {
    err = ERR_BUSY;
    HCDERR("existring device type already identified as non-hid (%s)",
      usb_hcd_device_class_to_string(dev->class->device_class));
    goto out_err;
  }

  i = usb_hcd_allocate_hid_interface();
  if (!i) {
    err = ERR_BUSY;
    HCDERR("failed to allocate hid interface");
    goto out_err;
  }
  list_add_tail(&i->hid_interfaces, &h->hid_interfaces);

  memcpy(&i->descriptor, hid_desc, sizeof(struct usb_hid_descriptor));

  HCDLOG("hid: version:%04x, country:%d, desc_count:%d, type:%d, length:%d",
    get_unaligned_16_le(&hid_desc->hid_version),
    hid_desc->country_code,
    hid_desc->descriptor_count,
    hid_desc->type,
    get_unaligned_16_le(&hid_desc->length));
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
  if (usb_hcd_log_level)
    hexdump_memory(cfgbuf, cfgbuf_sz);

  while((void*)hdr < cfgbuf_end && should_continue) {
    HCDDEBUG("found descriptor:%s(%d),size:%d",
        usb_descriptor_type_to_string(hdr->descriptor_type),
        hdr->descriptor_type,
        hdr->length);
    switch(hdr->descriptor_type) {
      case USB_DESCRIPTOR_TYPE_INTERFACE:
        i = &dev->interfaces[dev->num_interfaces++];
        if (i >= end_i) {
          /* VULNURABILITY idea */
          HCDERR("inteface count reached limit %d", end_i - dev->interfaces);
          should_continue = 0;
          break;
        }

        memcpy(&i->descriptor, hdr, sizeof(struct usb_interface_descriptor));
        ep = i->endpoints;
        end_ep = i->endpoints + USB_MAX_ENDPOINTS_PER_DEVICE;

        HCDDEBUG("-- address:%d,interface:%d,num_endpoints:%d,class:%s(%d,%d,%d)", dev->address,
          i->descriptor.number,
          i->descriptor.endpoint_count,
          usb_full_class_to_string(i->descriptor.class, i->descriptor.subclass, i->descriptor.protocol),
          i->descriptor.class, i->descriptor.subclass, i->descriptor.protocol);
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
        ep->device = dev;
        ep->next_toggle_pid = USB_PID_DATA0;
        HCDDEBUG("---- address:%d,interface:%d,endpoint:%d,dir:%s,attr:%02x(%s,%s,%s),packet_size:%d,int:%d",
          dev->address,
          i->descriptor.number,
          ep->descriptor.endpoint_address & 0x7f,
          usb_direction_to_string(ep->descriptor.endpoint_address >> 7),
          ep->descriptor.attributes,
          usb_endpoint_type_to_string(ep->descriptor.attributes & 3),
          usb_endpoint_synch_type_to_string((ep->descriptor.attributes >> 2) & 3),
          usb_endpoint_usage_type_to_string((ep->descriptor.attributes >> 4) & 3),
          ep->descriptor.max_packet_size,
          ep->descriptor.interval
        );
        ep++;
        break;
      case USB_DESCRIPTOR_TYPE_HID:
        err = usb_hcd_parse_handle_hid(dev, (struct usb_hid_descriptor *)hdr);
        if (err != ERR_OK) {
          HCDERR("failed to parse hid interface. skipping");
          err = ERR_OK;
        }
        break;
      case USB_DESCRIPTOR_TYPE_CONFIGURATION:
        HCDDEBUG("---- configuration:%d,num_interfaces:%d,string_index:%d", c->configuration_value,
          c->num_interfaces, c->iconfiguration);
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

static int usb_hcd_set_configuration(struct usb_hcd_pipe *pipe, int configuration)
{
  int err;
  uint64_t rq = USB_DEV_RQ_MAKE(SET_CONFIGURATION, SET_CONFIGURATION, configuration, 0, 0);
  err = HCD_TRANSFER_CONTROL(pipe, USB_DIRECTION_OUT, 0, 0, rq, 0);
  CHECK_ERR("failed to set configuration");

out_err:
  return err;
}

#define HCD_ASSERT_DEVICE_STATE_CHANGE(__dev, __from, __to)\
  if (__dev->state != USB_DEVICE_STATE_##__from) {\
    err = ERR_INVAL_ARG;\
    HCDERR("can't promote device %d to "#__to" state from current %s",\
      dev->address, usb_hcd_device_state_to_string(dev->state));\
    goto out_err;\
  }

static inline void usb_hcd_device_descriptor_to_nice_string(struct usb_device_descriptor *desc, const char *prefix, int full)
{
  if (full) {
    HCDDEBUG("%s: VENDOR:%04x,PRODUCT:%04x,bcd:%04x, class:%s(class:%d,subclass:%d,proto:%d), max_packet_size: %d",
        prefix,
        desc->id_vendor,
        desc->id_product,
        desc->bcd_usb,
        usb_full_class_to_string(desc->device_class, desc->device_subclass, desc->device_protocol),
        desc->device_class,
        desc->device_subclass,
        desc->device_protocol,
        desc->max_packet_size_0);
  } else /* first half */ {
    HCDDEBUG("%s: bcd:%04x, class:%s(class:%d,subclass:%d,proto:%d), max_packet_size: %d",
        prefix,
        desc->bcd_usb,
        usb_full_class_to_string(desc->device_class, desc->device_subclass, desc->device_protocol),
        desc->device_class,
        desc->device_subclass,
        desc->device_protocol,
        desc->max_packet_size_0);
  }
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
static int usb_hcd_to_default_state(struct usb_hcd_device *dev)
{
  const int to_transfer_size = 8;
  int err, num_bytes;
  uint64_t rq;
  struct usb_device_descriptor desc ALIGNED(64) = { 0 };

  if (dev->location.hub) {
    HCDDEBUG("setting DEFAULT state for device at hub:%d.port:%d",
      dev->location.hub->address, dev->location.hub_port);
  } else {
    HCDDEBUG("setting DEFAULT state for root hub device");
  }

  dev->pipe0.address = USB_DEFAULT_ADDRESS;
  dev->pipe0.max_packet_size = USB_DEFAULT_PACKET_SIZE;

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, POWERED, DEFAULT);

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_DEVICE, 0, 0,
      to_transfer_size);
  err = HCD_TRANSFER_CONTROL(&dev->pipe0, USB_DIRECTION_IN,
      &desc, to_transfer_size, rq, &num_bytes);
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
  if (usb_hcd_log_level > 2)
    usb_hcd_device_descriptor_to_nice_string(&desc, "device in state DEFAULT", 0);
out_err:
  return err;
}

static inline int usb_hcd_allocate_device_address()
{
  return usb_hcd_unique_device_address++;
}

static int usb_hcd_set_address(struct usb_hcd_pipe *p, int address)
{
  int err;
  uint64_t rq = USB_DEV_RQ_MAKE(SET_ADDRESS, SET_ADDRESS, address, 0, 0);
  err = HCD_TRANSFER_CONTROL(p, USB_DIRECTION_OUT, 0, 0, rq, 0);
  CHECK_ERR("failed to set address");
out_err:
  return err;
}

/*
 * By this time device has been set to DEFAULT state.
 * We allocate a unique device address from 127 address space.
 */
static int usb_hcd_to_addressed_state(struct usb_hcd_device *dev)
{
  int err;
  int device_address;

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, DEFAULT, ADDRESSED);
  dwc2_dump_int_registers();

  /*
   * Ritual routine to support devices that still want to give us
   * last part of device descriptor, that we asked at DEFAULT state.
   */
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
  HCDDEBUG("setting device address to %d", device_address);

  err = usb_hcd_set_address(&dev->pipe0, device_address);
  CHECK_ERR_SILENT();

  dev->address = dev->pipe0.address = device_address;

  wait_on_timer_ms(10);
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
static int usb_hcd_to_configured_state(struct usb_hcd_device *dev)
{
  int err, num_bytes;
  uint64_t rq;
  struct usb_configuration_descriptor config_desc ALIGNED(64);
  uint8_t config_buffer[1024] ALIGNED(64);
  char buffer[256] ALIGNED(64);
  int config_num;

  memset(&config_desc, 0xfc, sizeof(config_desc));
  memset(config_buffer, 0xfc, sizeof(config_buffer));
  memset(&buffer, 0xfc, sizeof(buffer));

  HCD_ASSERT_DEVICE_STATE_CHANGE(dev, ADDRESSED, CONFIGURED);

  GET_DESC(&dev->pipe0, DEVICE       , 0, 0, &dev->descriptor, sizeof(dev->descriptor));
  if (usb_hcd_log_level > 2)
    usb_hcd_device_descriptor_to_nice_string(&dev->descriptor, "got full device descriptor", 1);

  GET_DESC(&dev->pipe0, CONFIGURATION, 0, 0, &config_desc    , sizeof(config_desc));
  HCDDEBUG("got configuration header: total size: %d", config_desc.total_length);

  dev->configuration_string = config_desc.iconfiguration;
  config_num = config_desc.configuration_value;

  rq = USB_DEV_RQ_MAKE_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, 0, config_desc.total_length);

  err = HCD_TRANSFER_CONTROL(&dev->pipe0, USB_DIRECTION_IN, config_buffer,
      config_desc.total_length, rq, &num_bytes);
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

  err = usb_hcd_set_configuration(&dev->pipe0, config_num);
  CHECK_ERR("failed to set configuration %d for device", config_num);

  dev->configuration = config_num;
  dev->state = USB_DEVICE_STATE_CONFIGURED;

#define READ_STRING(__string_idx, __nice_name, dst, dst_sz)\
  if (__string_idx) {\
    err = usb_hcd_read_string_descriptor(&dev->pipe0,\
        __string_idx, dst, dst_sz);\
    CHECK_ERR("failed to get "#__nice_name" string");\
  }

  READ_STRING(dev->descriptor.i_product      , "product", dev->string_product, sizeof(dev->string_product));
  READ_STRING(dev->descriptor.i_manufacturer , "manufacturer", dev->string_manufacturer, sizeof(dev->string_manufacturer));
  READ_STRING(dev->descriptor.i_serial_number, "serial number", dev->string_serial, sizeof(dev->string_serial));
  READ_STRING(dev->configuration_string      , "configuration", dev->string_configuration, sizeof(dev->string_configuration));
out_err:
  return err;
}

int usb_hcd_enumerate_device(struct usb_hcd_device *dev)
{
  int err = ERR_OK;

  HCDDEBUG("=============================================================");
  HCDDEBUG("********************* ENUMERATE DEVICE **********************");
  HCDDEBUG("=============================================================");

  // http://microsin.net/adminstuff/windows/how-usb-stack-enumerate-device.html

  err = usb_hcd_to_default_state(dev);
  CHECK_ERR_SILENT();

  err = usb_hcd_to_addressed_state(dev);
  CHECK_ERR_SILENT();

  err = usb_hcd_to_configured_state(dev);
  CHECK_ERR_SILENT();

  if (dev->descriptor.device_class == USB_INTERFACE_CLASS_HUB) {
      HCDDEBUG("HUB: vendor:%04x:product:%04x", dev->descriptor.id_vendor, dev->descriptor.id_product);
      HCDDEBUG("   : max_packet_size: %d", dev->descriptor.max_packet_size_0);
      err = usb_hub_enumerate(dev);
      CHECK_ERR_SILENT();
  } else if (dev->class && dev->class->device_class == USB_HCD_DEVICE_CLASS_HID) {
      HCDLOG("Enumerate HID");
      err = usb_hid_enumerate(dev);
      CHECK_ERR_SILENT();
  } else if (usb_hcd_get_interface_class(dev, 0) == USB_INTERFACE_CLASS_MASSSTORAGE) {
      usb_mass_init(dev);
      // while(1);
  }
out_err:
  HCDDEBUG("=============================================================");
  HCDDEBUG("********************* ENUMERATE DEVICE END ******************");
  HCDDEBUG("=============================================================");
  return err;
}

static int hcd_rx_fifo_flush()
{
  HCDDEBUG("started");
  dwc2_rx_fifo_flush();
  HCDDEBUG("completed");
  return ERR_OK;
}

static int hcd_tx_fifo_flush(int fifo)
{
  HCDLOG("fifo:%d", fifo);
  dwc2_tx_fifo_flush(fifo);
  return ERR_OK;
}

int usb_hcd_start()
{
  int err = ERR_OK;
  dwc2_fs_iface fs_iface;
  dwc2_hs_iface hs_iface;
  dwc2_op_mode_t opmode;
  int fsls_mode_ena = 0;

  err = irq_set(get_cpu_num(), ARM_BASIC_USB, dwc2_irq_cb);
  dwc2_enable_ahb_interrupts();
  dwc2_clear_all_interrupts();

  intr_ctl_usb_irq_enable();
  HCDLOG("before IRQ enable");
  enable_irq();

  dwc2_start_vbus();
  dwc2_reset();

  if (!usb_utmi_initialized) {
    HCDLOG("initializing USB to UTMI+,no PHY");
    dwc2_set_ulpi_no_phy();
    dwc2_reset();
    usb_utmi_initialized = 1;
  }

  hs_iface = dwc2_get_hs_iface();
  fs_iface = dwc2_get_fs_iface();

  HCDLOG("HW config: high speed interface:%d(%s)", hs_iface, dwc2_hs_iface_to_string(hs_iface));
  HCDLOG("HW config: full speed interface:%d(%s)", fs_iface, dwc2_fs_iface_to_string(fs_iface));
  if (hs_iface == DWC2_HS_I_ULPI && fs_iface == DWC2_FS_I_DEDICATED)
    fsls_mode_ena = 1;

  dwc2_set_fsls_config(fsls_mode_ena);
  HCDLOG("ULPI: setting FSLS configuration to %sabled", fsls_mode_ena ? "en" : "dis");

  dwc2_set_dma_mode();

  opmode = dwc2_get_op_mode();
  dwc2_enable_channel_interrupts();
  HCDLOG("dwc2 op mode: %s", dwc2_op_mode_to_string(opmode));
  switch(opmode) {
    case DWC2_OP_MODE_HNP_SRP_CAPABLE:
      dwc2_set_otg_cap(DWC2_OTG_CAP_HNP_SRP);
      break;
    case DWC2_OP_MODE_SRP_ONLY_CAPABLE:
    case DWC2_OP_MODE_SRP_CAPABLE_DEVICE:
    case DWC2_OP_MODE_SRP_CAPABLE_HOST:
      dwc2_set_otg_cap(DWC2_OTG_CAP_SRP);
      break;
    case DWC2_OP_MODE_NO_HNP_SRP_CAPABLE:
    case DWC2_OP_MODE_NO_SRP_CAPABLE_DEVICE:
    case DWC2_OP_MODE_NO_SRP_CAPABLE_HOST:
      dwc2_set_otg_cap(DWC2_OTG_CAP_NONE);
      break;
  }
  HCDLOG("core started");
  // dwc2_unmask_all_interrupts();

  HCDLOG("setting host clock...");
  dwc2_power_clock_off();
  if (hs_iface == DWC2_HS_I_ULPI && fs_iface == DWC2_FS_I_DEDICATED && dwc2_is_ulpi_fs_ls_only()) {
    dwc2_set_host_speed(DWC2_CLK_48MHZ);
    HCDLOG("host clock set to 48MHz");
  } else {
    HCDLOG("host clock set to 30-60MHz");
    dwc2_set_host_speed(DWC2_CLK_30_60MHZ);
  }

  dwc2_set_host_ls_support();
  HCDLOG("enabled host low-speed support");

  dwc2_setup_fifo_sizes(
    USB_RECV_FIFO_SIZE,
    USB_NON_PERIODIC_FIFO_SIZE,
    USB_PERIODIC_FIFO_SIZE);
  HCDLOG("fifos set to recv:%d non-peridic:%d periodic:%d",
    USB_RECV_FIFO_SIZE,
    USB_NON_PERIODIC_FIFO_SIZE,
    USB_PERIODIC_FIFO_SIZE);

  dwc2_set_otg_hnp();
	HCDLOG("OTG host is set with HNP enabled.");

  hcd_tx_fifo_flush(16);
  hcd_rx_fifo_flush();
  if (!dwc2_is_dma_enabled())
    dwc2_init_channels();

  if (!dwc2_is_port_pwr_enabled()) {
    dwc2_port_set_pwr_enabled(true);
    // HCDLOG("host power enabled");
  }

  dwc2_port_reset();
  wait_on_timer_ms(60);
  dwc2_port_reset_clear();
  HCDLOG("host reset done");
  // if (usb_hcd_log_level > 0)
  //  dwc2_dump_int_registers();
  HCDLOG("host controller device started");
  return err;
}

static int usb_hcd_attach_root_hub()
{
  int err;
  struct usb_hcd_device_class_hub *h = NULL;

  if (root_hub) {
    err = ERR_BUSY;
    HCDERR("root_hub already attached");
    goto out_err;
  }

  root_hub = usb_hcd_allocate_device();
  if (IS_ERR(root_hub)) {
    err = PTR_ERR(root_hub);
    goto out_err;
  }

  h = usb_hcd_hub_create();
  if (!h) {
    err = ERR_NO_RESOURCE;
    HCDERR("failed to allocate hub device object");
    goto out_err;
  }

  h->d = root_hub;
  root_hub->class = &h->base;

  /*
   * by default our root hub is in not in ADDRESSED state,
   * so it should respond on default address.
   */
  usb_root_hub_device_number = USB_DEFAULT_ADDRESS;
  root_hub->state = USB_DEVICE_STATE_POWERED;
  root_hub->pipe0.speed = USB_SPEED_FULL;
  root_hub->pipe0.max_packet_size = 64;
  err = usb_hcd_enumerate_device(root_hub);
  if (err != ERR_OK)
    HCDERR("failed to add root hub. err: %d", err);
  else
    HCDDEBUG("root hub added");
out_err:
  return err;
}

static int usb_hcd_power_on()
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

static int usb_hcd_is_powered_on()
{
  int err;
  uint32_t exists = 0, powered_on = 1;
  err = mbox_get_power_state(MBOX_DEVICE_ID_USB, &powered_on, &exists);
  if (err) {
    HCDERR("mbox call failed");
    return ERR_GENERIC;
  }
  if (!exists) {
    HCDERR("after mbox call:device does not exist");
    return ERR_GENERIC;
  }
  return powered_on;
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

int usb_hcd_init()
{
  int err;
  uint32_t vendor_id, user_id;
  int irqflags;
  STATIC_SLOT_INIT_FREE(usb_hcd_device);
  dwc2_init();
  usb_hcd_hub_init();
  usb_hcd_hid_init();
  usb_hcd_mass_init();

  vendor_id = dwc2_get_vendor_id();
  user_id = dwc2_get_user_id();
  HCDLOG("Initializing: usb chip info: vendor:%08x user:%08x", vendor_id, user_id);

  disable_irq_save_flags(irqflags);
  err = usb_hcd_power_on();
  CHECK_ERR("failed to power on");
  wait_msec(20);
  restore_irq_flags(irqflags);
  powered_on = usb_hcd_is_powered_on();

  BUG(powered_on < 0, "Failed to get USB power on state");
  BUG(powered_on != 1, "USB failed to power on");
  HCDLOG("Device powered on");
  dwc2_print_core_regs();

  err = usb_hcd_start();
  CHECK_ERR("hcd start failed");
  usb_xfer_queue_init();

  err = usb_hcd_attach_root_hub();
  CHECK_ERR("attach root hub failed");

  return ERR_OK;

out_err:
  if (powered_on) {
    BUG(usb_hcd_power_off() != ERR_OK, "Failed to shutdown usb");
    powered_on = false;
  }
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

const char *usb_hcd_device_class_to_string(int c)
{
  switch(c) {
    case USB_HCD_DEVICE_CLASS_HUB: return "HUB";
    case USB_HCD_DEVICE_CLASS_HID: return "HID";
    default: return "UNKNOWN";
  }
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

  n = prefix_padding_to_string(prefix, ' ', depth, 1, buf, bufsz);
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
    dev->pipe0.ep,
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

int hcd_endpoint_get_status(struct usb_hcd_endpoint *ep, void *status)
{
  int err;
  int num_bytes;
  struct usb_device_request rq = {
    .request_type = USB_RQ_TYPE_ENDPOINT_GET_STATUS,
    .request      = USB_RQ_GET_STATUS,
    .value        = 0,
    .index        = hcd_endpoint_get_number(ep),
    .length       = 2
  };

  HCDLOG("ENDPOINT GET_STATUS: ep%d", hcd_endpoint_get_number(ep));

  struct usb_hcd_device *dev = ep->device;

  err = HCD_TRANSFER_CONTROL(&dev->pipe0, USB_DIRECTION_IN, status, 2, rq.raw, &num_bytes);
  if (err) {
    HCDERR("failed to clear halt on ep 1");
  }
  return err;
}

int hcd_endpoint_clear_feature(struct usb_hcd_endpoint *ep, int feature)
{
  int err;
  int num_bytes;
  struct usb_device_request rq = {
    .request_type = USB_RQ_TYPE_ENDPOINT_CLEAR_FEATURE,
    .request      = USB_RQ_CLEAR_FEATURE,
    .value        = feature,
    .index        = hcd_endpoint_get_number(ep),
    .length       = 0
  };
  HCDLOG("clear feature: ep%d", hcd_endpoint_get_number(ep));

  struct usb_hcd_device *dev = ep->device;

  err = HCD_TRANSFER_CONTROL(&dev->pipe0, USB_DIRECTION_OUT, NULL, 0, rq.raw, &num_bytes);
  if (err) {
    HCDERR("failed to clear halt on ep 1");
  }
  return err;
}

int hcd_endpoint_set_feature(struct usb_hcd_endpoint *ep, int feature)
{
  int err;
  int num_bytes;
  struct usb_device_request rq = {
    .request_type = USB_RQ_TYPE_ENDPOINT_SET_FEATURE,
    .request      = USB_RQ_SET_FEATURE,
    .value        = feature,
    .index        = hcd_endpoint_get_number(ep),
    .length       = 0
  };
  struct usb_hcd_device *dev = ep->device;

  err = HCD_TRANSFER_CONTROL(&dev->pipe0, USB_DIRECTION_OUT, NULL, 0, rq.raw, &num_bytes);
  if (err) {
    HCDERR("failed to clear halt on ep 1");
  }
  return err;
}
