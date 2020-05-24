#pragma once

#include <drivers/usb/hcd.h>

int usb_mass_init(struct usb_hcd_device* dev);

int usb_mass_set_log_level(int level);
