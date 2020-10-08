#pragma once
#include <usb/usb.h>

extern int usb_root_hub_device_number;

struct root_hub_configuration {
	struct usb_configuration_descriptor cfg;
	struct usb_interface_descriptor iface;
	struct usb_endpoint_descriptor ep;
} PACKED;

struct usb_root_hub_string_descriptor0 {
  struct usb_descriptor_header h;
  uint16_t lang_id;
} PACKED;

int usb_root_hub_process_req(uint64_t rq, void *buf, int buf_sz, int *out_num_bytes);

void root_hub_get_port_status(struct usb_hub_port_status *s);
