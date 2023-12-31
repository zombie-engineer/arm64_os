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

int usb_hid_get_desc(struct usb_hcd_device *dev, int hid_index, void *descriptor_addr, int descriptor_length)
{
  int err;
  int num_bytes;
  uint64_t rq;
  HCDDEBUG("usb_hid_get_desc: hid_index:%d, descriptor_length:%d", hid_index, descriptor_length);

  rq = USB_DEV_RQ_MAKE(GET_INTERFACE, GET_DESCRIPTOR, USB_DESCRIPTOR_TYPE_HID_REPORT << 8, hid_index, descriptor_length);
  memset(descriptor_addr, 0xff, descriptor_length);
  err = HCD_TRANSFER_CONTROL(&dev->pipe0,
    USB_DIRECTION_IN,
    descriptor_addr,
    descriptor_length,
    rq,
    &num_bytes);
  CHECK_ERR("failed to read descriptor header");
out_err:
  return err;
}

int usb_hid_set_idle(struct usb_hcd_device *dev, int idx)
{
  int err;
  int num_bytes;
  uint64_t rq;
  HCDLOG("usb_hid_set_idle");

  rq = USB_DEV_RQ_MAKE(CLASS_SET_INTERFACE, HID_SET_IDLE, 0, 0, 0);
  err = HCD_TRANSFER_CONTROL(&dev->pipe0, USB_DIRECTION_OUT, 0, 0, rq, &num_bytes);
  CHECK_ERR("failed to set idle");
out_err:
  return err;
}

int usb_hid_mouse_get_report(struct usb_hcd_device *dev, int endpoint_num)
{
  int err;
  int num_bytes;
  char buf[64] ALIGNED(4);
  struct usb_hcd_interface *i;
  struct usb_hcd_endpoint *ep;

  i = hcd_device_get_interface(dev, 0);
  BUG(!i, "interface is NULL");
  ep = hcd_interface_get_endpoint(i, endpoint_num);
  BUG(!ep, "endpoint is NULL");

  memset(buf, 0x66, sizeof(buf));
  err = hcd_transfer_interrupt(&ep->pipe, buf, 7, &num_bytes);
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
  char buf[256] ALIGNED(64);
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
  return err;
}

