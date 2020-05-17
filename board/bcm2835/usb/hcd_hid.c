#include <drivers/usb/hcd_hid.h>
#include <drivers/usb/usb_dev_rq.h>
#include <stringlib.h>
#include <common.h>
#include <mem_access.h>
#include "hid_parser.h"

DECL_STATIC_SLOT(struct usb_hcd_hid_interface, usb_hcd_hid_interface, 12)
DECL_STATIC_SLOT(struct usb_hcd_device_class_hid, usb_hcd_device_class_hid, 12)

struct usb_hcd_hid_interface *usb_hcd_allocate_hid_interface()
{
  struct usb_hcd_hid_interface *i;
  i = usb_hcd_hid_interface_alloc();
  if (i) {
    memset(&i->descriptor, 0, sizeof(i->descriptor));
    INIT_LIST_HEAD(&i->hid_interfaces);
  }
  return i;
}

struct usb_hcd_device_class_hid *usb_hcd_allocate_hid()
{
  struct usb_hcd_device_class_hid *hid;
  hid = usb_hcd_device_class_hid_alloc();
  hid->base.device_class = USB_HCD_DEVICE_CLASS_HID;
  INIT_LIST_HEAD(&hid->hid_interfaces);
  return hid;
}

void usb_hcd_deallocate_hid(struct usb_hcd_device_class_hid *h)
{
  usb_hcd_device_class_hid_release(h);
}

void usb_hcd_hid_init()
{
  STATIC_SLOT_INIT_FREE(usb_hcd_device_class_hid);
  STATIC_SLOT_INIT_FREE(usb_hcd_hid_interface);
}

int usb_hid_get_desc(struct usb_hcd_device *dev, int hid_index, void *buf, int bufsz)
{
  int err;
  int num_bytes;
	struct usb_hcd_pipe_control pctl;
  uint64_t rq;
  HCDLOG("usb_hid_get_desc: %d %d", hid_index, bufsz);

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  rq = USB_DEV_RQ_MAKE(GET_INTERFACE, GET_DESCRIPTOR, USB_DESCRIPTOR_TYPE_HID_REPORT << 8, hid_index, bufsz);
  memset(buf, 0xff, sizeof(buf));

  err = hcd_transfer_control(&dev->pipe0, &pctl, 
      buf, bufsz, rq, 1000, &num_bytes);
  if (err) {
    HCDERR("failed to read descriptor header");
    goto out_err;
  }
out_err:
  return err;
}

int usb_hid_set_idle(struct usb_hcd_device *dev, int idx)
{
  int err;
  int num_bytes;
	struct usb_hcd_pipe_control pctl;
  uint64_t rq;
  HCDLOG("usb_hid_set_idle");

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_OUT;

  rq = USB_DEV_RQ_MAKE(CLASS_SET_INTERFACE, HID_SET_IDLE, 0, 0, 0);
  err = hcd_transfer_control(&dev->pipe0, &pctl, 0, 0, rq, 1000, &num_bytes);
  CHECK_ERR("failed to set idle");
out_err:
  return err;
}

int usb_hid_mouse_get_report(struct usb_hcd_device *dev, int ep)
{
  int err;
  int num_bytes;
  char buf[64] ALIGNED(4);

  struct usb_hcd_pipe pipe = {
    .address = dev->pipe0.address,
    .endpoint = ep,
    .speed = dev->pipe0.speed,
    .max_packet_size = dev->pipe0.max_packet_size,
    .ls_hub_port = dev->pipe0.ls_hub_port,
    .ls_hub_address = dev->pipe0.ls_hub_address
  };

  memset(buf, 0x66, sizeof(buf));
  err = hcd_transfer_interrupt(&pipe, buf, 7, 1000, &num_bytes);
  CHECK_ERR("a");
  hexdump_memory(buf, 8);
out_err:
  return err;
}

int usb_hid_enumerate(struct usb_hcd_device *dev)
{
  int err = 0;
  int hid_index = 0;
  int desc_length;
  char buf[256] ALIGNED(4); 
  struct usb_hcd_device_class_hid *h;
  struct usb_hcd_hid_interface *i;
  h = usb_hcd_device_to_hid(dev);
  HCDLOG("enumerating device %p, address:%d, num_interfaces: %d",
    dev, dev->address, dev->num_interfaces);

  memset(buf, 0xfc, sizeof(buf));
  list_for_each_entry(i, &h->hid_interfaces, hid_interfaces) {
    desc_length = get_unaligned_16_le(&i->descriptor.length);
    err = usb_hid_get_desc(dev, hid_index, buf, desc_length);
    if (err)
      HCDERR("failed to get report descriptor %d", hid_index);
    else {
      hexdump_memory(buf, desc_length);
      usb_hid_parse_report_descriptor(buf, desc_length);
    }
    hid_index++;
  }
out_err:
  return err;
}

