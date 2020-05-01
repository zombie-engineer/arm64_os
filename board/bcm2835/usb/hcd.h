#pragma once
#include <list.h>
#include <usb/usb.h>
#include <memory/static_slot.h>

#define USB_HCD_DEVICE_CLASS_UNDEFINED 0
#define USB_HCD_DEVICE_CLASS_HUB       1
#define USB_HCD_DEVICE_CLASS_HID       2

struct usb_hcd_device_class_base {
  int device_class;
};

struct usb_hcd_device_location {
  struct usb_hcd_device *hub;
  int hub_port;
};

struct usb_hcd_endpoint {
  struct usb_endpoint_descriptor descriptor ALIGNED(4);
};

struct usb_hcd_interface {
  struct usb_interface_descriptor descriptor;
  struct usb_hcd_endpoint endpoints[USB_MAX_ENDPOINTS_PER_DEVICE] ALIGNED(4);
};

struct usb_hcd_device {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_device);
  int id;
  int state;
  int configuration;
  int configuration_string;
  struct usb_hcd_device_location location;
  struct usb_hcd_pipe pipe0;
  struct usb_hcd_pipe_control pipe0_control;

  struct usb_device_descriptor descriptor ALIGNED(4);
  struct usb_hcd_interface interfaces[USB_MAX_INTERFACES_PER_DEVICE] ALIGNED (4);
  struct usb_hcd_device_class_base *class; 
};

struct usb_hcd_device_class_hub {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_device_class_hub);
  struct usb_hcd_device_class_base base;
	struct usb_hub_descriptor descriptor;
  struct list_head children;
};

