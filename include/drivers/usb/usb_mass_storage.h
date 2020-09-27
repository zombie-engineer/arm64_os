#pragma once

#include <drivers/usb/hcd.h>
#include <memory/static_slot.h>

typedef struct usb_hcd_device_class_mass {
  /*
   * static slot list head
   */
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_device_class_mass);

  /*
   * pointer to owning generic HCD device
   */
  struct usb_hcd_device *d;

  /*
   * owner generic HCD device has reference to base field
   */
  struct usb_hcd_device_class_base base;

  struct usb_hcd_endpoint *ep_out;
  struct usb_hcd_endpoint *ep_in;
} hcd_mass_t;

static inline hcd_mass_t *usb_hcd_device_to_mass(struct usb_hcd_device *d)
{
  return container_of(d->class, hcd_mass_t, base);
}

int usb_mass_init(struct usb_hcd_device* dev);

int usb_mass_set_log_level(int level);

struct usb_hcd_device_class_mass *usb_hcd_allocate_mass();

void usb_hcd_deallocate_mass(struct usb_hcd_device_class_mass *h);

void usb_hcd_mass_init();

int usb_mass_read(hcd_mass_t *m, uint32_t offset, void *data_dst, int data_sz);
