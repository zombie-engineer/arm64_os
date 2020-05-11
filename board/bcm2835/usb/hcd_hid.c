#include <drivers/usb/hcd_hid.h>
#include <drivers/usb/usb_dev_rq.h>
#include <stringlib.h>
#include <common.h>
#include <mem_access.h>

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
  printf("usb_hcd_allocate_hid\r\n");
  hid = usb_hcd_device_class_hid_alloc();
  hid->base.device_class = USB_HCD_DEVICE_CLASS_HID;
  INIT_LIST_HEAD(&hid->hid_interfaces);
  return hid;
}

void usb_hcd_deallocate_hid(struct usb_hcd_device_class_hid *h)
{
  printf("usb_hcd_deallocate_hid\r\n");
  usb_hcd_device_class_hid_release(h);
}

void usb_hcd_hid_init()
{
  STATIC_SLOT_INIT_FREE(usb_hcd_device_class_hid);
  STATIC_SLOT_INIT_FREE(usb_hcd_hid_interface);
}

int usb_hid_get_desc(struct usb_hcd_device *dev, int hid_index, int sz)
{
  int err;
  int num_bytes;
  char buf[256] ALIGNED(4);
	struct usb_hcd_pipe_control pctl;
  uint64_t rq;
  printf("usb_hid_get_desc: %d %d\r\n", hid_index, sz);

  pctl.channel = 0;
  pctl.transfer_type = USB_ENDPOINT_TYPE_CONTROL;
  pctl.direction = USB_DIRECTION_IN;

  rq = USB_DEV_RQ_MAKE(GET_INTERFACE, GET_DESCRIPTOR, USB_DESCRIPTOR_TYPE_HID_REPORT << 8, hid_index, sz);
  memset(buf, 0xff, sizeof(buf));

  err = usb_hcd_submit_cm(&dev->pipe0, &pctl, 
      buf, sz, rq, 1000, &num_bytes);
  if (err) {
    HCDERR("failed to read descriptor header");
    goto out_err;
  }
  hexdump_memory(buf, sz);

out_err:
  return err;
}

int usb_hid_enumerate(struct usb_hcd_device *dev)
{
  int err = 0;
  int hid_index = 0;
  char buffer[256] ALIGNED(4); 
  struct usb_hcd_device_class_hid *h;
  struct usb_hcd_hid_interface *i;
  h = usb_hcd_device_to_hid(dev);

  memset(buffer, 0xfc, sizeof(buffer));
  list_for_each_entry(i, &h->hid_interfaces, hid_interfaces) {
    err = usb_hid_get_desc(dev, hid_index, 
      get_unaligned_16_le(&i->descriptor.length));
    hid_index++;
  }
  // GET_DESC(&dev->pipe0, HID_REPORT, 0, 0, &buffer, 65);// sizeof(config_desc));
 /// hexdump_memory(buffer, sizeof(buffer));
//out_err:
  return err;
}

