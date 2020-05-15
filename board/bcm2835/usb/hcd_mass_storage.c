#include <drivers/usb/usb_mass_storage.h>
#include <common.h>

#define __MASS_PREFIX "[USB_MASS: %d] "
#define __MASS_ARGS 0

#define MASS_LOG(__fmt, ...) printf(__MASS_PREFIX __fmt __endline, __MASS_ARGS, ##__VA_ARGS__)


void usb_mass_storage_init(struct usb_hcd_device* dev)
{
  MASS_LOG("usb_mass_storage_init");
}
