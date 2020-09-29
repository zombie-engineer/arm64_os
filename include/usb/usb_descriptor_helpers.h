#pragma once
#include <usb/usb.h>

static inline int usb_interface_descriptor_get_number(struct usb_interface_descriptor *d)
{
  return d->number;
}

static inline int usb_endpoint_descriptor_get_number(struct usb_endpoint_descriptor *d)
{
  return d->endpoint_address & 0xf;
}

static inline int usb_endpoint_descriptor_get_direction(struct usb_endpoint_descriptor *d)
{
  return (d->endpoint_address >> 7) & 1;
}

static inline int usb_endpoint_descriptor_get_max_packet_size(struct usb_endpoint_descriptor *d)
{
  return d->max_packet_size;
}

static inline int usb_endpoint_descriptor_get_type(struct usb_endpoint_descriptor *d)
{
  return d->attributes & 3;
}

static inline int usb_endpoint_descriptor_get_sync_type(struct usb_endpoint_descriptor *d)
{
  return (d->attributes >> 2) & 3;
}

static inline int usb_endpoint_descriptor_get_use_type(struct usb_endpoint_descriptor *d)
{
  return (d->attributes >> 4) & 3;
}

static inline int usb_endpoint_descriptor_get_interval(struct usb_endpoint_descriptor *d)
{
  return d->interval;
}

static inline int usb_endpoint_descriptor_get_attributes(struct usb_endpoint_descriptor *d)
{
  return d->attributes;
}
