#pragma once

#include <drivers/usb/hcd.h>
#include <memory/static_slot.h>

struct usb_hcd_hid_interface {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_hid_interface);
  struct list_head hid_interfaces;
	struct usb_hid_descriptor descriptor;
};

typedef struct usb_hcd_device_class_hid {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_device_class_hid);
  struct usb_hcd_device *d;
  struct usb_hcd_device_class_base base;
  struct list_head hid_interfaces;
} usb_hid_t;

static inline usb_hid_t *usb_hcd_device_to_hid(struct usb_hcd_device *d)
{
  return container_of(d->class, usb_hid_t, base);
}

struct usb_hcd_device_class_hid *usb_hcd_allocate_hid();

struct usb_hcd_hid_interface *usb_hcd_allocate_hid_interface();
void usb_hcd_deallocate_hid(struct usb_hcd_device_class_hid *h);

int usb_hid_enumerate(struct usb_hcd_device *dev);

void usb_hcd_hid_init();
