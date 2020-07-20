#pragma once
#include <usb/usb.h>
#include <common.h>
#include <stringlib.h>

static inline void print_usb_hub_descriptor(struct usb_hub_descriptor *h)
{
  printf("hub_desc:%p:type:%d,len:%d,numports:%d,attr:%02x-%02x,pwrdelay:%d,maxpow:%d,rem:%x,msk:%02x\r\n",
    h,
    h->header.descriptor_type,
    h->header.length,
    h->port_count,
    h->attributes.hi, h->attributes.lo,
    h->power_good_delay,
    h->maximum_hub_power,
    h->device_removable,
    h->port_power_ctrl_mask);
}

static inline void print_usb_device_descriptor(struct usb_device_descriptor *d)
{
  printf("dev_desc:%p:type:%d,len:%d,usb:%d,clss:%d,sclss:%d,mps:%d\r\n",
    d,
    d->descriptor_type,
    d->length,
    d->bcd_usb,
    d->device_class,
    d->device_subclass,
    d->max_packet_size_0);

  printf("-------:ven:%04x,prod:%04x,dev:%x,manu:%d,produ:%d,seri:%d,num_conf:%d\r\n",
    d->id_vendor,
    d->id_product,
    d->bcd_device,
    d->i_manufacturer,
    d->i_product,
    d->i_serial_number,
    d->num_configurations);
}

static inline void print_usb_configuration_desc(const struct usb_configuration_descriptor *c)
{
  printf("cfg_desc:%p:type:%d,len:%d,tlen:%d,inum:%d,conf:%d,iconf:%d,attr:%x,maxpwr:%d\r\n",
    c,
    c->header.descriptor_type,
    c->header.length,
    c->total_length,
    c->num_interfaces,
    c->configuration_value,
    c->iconfiguration,
    c->attributes,
    c->max_power);
}


static inline void print_usb_endpoint_desc(struct usb_endpoint_descriptor *e)
{
  printf("ep_desc:%p:type:%d,len:%d,ep_addr:%02x,attr:%02x,mpsz:%d,intv:%d\r\n",
    e,
    e->header.descriptor_type,
    e->header.length,
    e->endpoint_address,
    e->attributes,
    e->max_packet_size,
    e->interval
    );
}

static inline void print_usb_interface_desc(struct usb_interface_descriptor *i)
{
  printf("iface_desc:%p:type:%d,len:%d,num:%d,alt:%d,ep_cnt:%d\r\n",
    i,
    i->header.descriptor_type,
    i->header.length,
    i->number,
    i->alt_setting,
    i->endpoint_count);
  printf("iface_desc:--:class:%d,subclass:%d,proto:%d,string:%d\r\n",
    i->class,
    i->subclass,
    i->protocol,
    i->string_index);
}

/*
 * Get device subclass name by triple "class,subclass,protocol"
 * https://www.usb.org/defined-class-codes
 */
static inline const char *usb_full_class_to_string(int class, int subclass, int proto)
{
  switch(class) {
    case USB_INTERFACE_CLASS_MASSSTORAGE:
      return "MASS_STORAGE_DEVICE";
    case USB_INTERFACE_CLASS_HID:
      switch (subclass) {
        case 0:
          switch (proto) {
            case 0:  return "HID_NO_SUBCLASS_NO_PROTO";
            case 1:  return "HID_NO_SUBCLASS_KEYBOARD";
            case 2:  return "HID_NO_SUBCLASS_MOUSE"   ;
            default: return "HID_NO_SUBCLASS_UNKNOWN" ;
          }
        case 1:
          switch (proto) {
            case 0:  return "HID_BOOT_SUBCLASS_NO_PROTO";
            case 1:  return "HID_BOOT_SUBCLASS_KEYBOARD";
            case 2:  return "HID_BOOT_SUBCLASS_MOUSE"   ;
            default: return "HID_BOOT_SUBCLASS_UNKNOWN" ;
          }
        default:
          switch (proto) {
            case 0:  return "UNKNOWN_HID_NO_PROTO";
            case 1:  return "UNKNOWN_HID_KEYBOARD";
            case 2:  return "UNKNOWN_HID_MOUSE"   ;
            default: return "UNKNOWN_HID_UNKNOWN" ;
          }
      }
    case USB_DEVICE_CLASS_HUB:
      switch (subclass) {
        case 0:
          switch (proto) {
            case 0: return "HUB_FULLSPEED";
            case 1: return "HUB_HIGHSPEED_SINGLE_TT";
            case 2: return "HUB_HIGHSPEED_MULTI_TT";
            default: return "UNKNOWN_HUB";
          }
        default: return "UNKNOWN_HUB";
      }
    case USB_DEVICE_CLASS_VENDOR_SPEC: return "VENDOR_SPECIFIC";
    default: return "UNKNOWN_CLASS";
  }
}

static inline const char *usb_device_class_to_string(int t)
{
  switch(t) {
#define __CASE(__f) case USB_INTERFACE_CLASS_## __f: return #__f
    __CASE(RESERVED);
    __CASE(AUDIO);
    __CASE(COMMUNICATIONS);
    __CASE(HID);
    __CASE(PHYSICAL);
    __CASE(IMAGE);
    __CASE(PRINTER);
    __CASE(MASSSTORAGE);
    __CASE(HUB);
    __CASE(CDCDATA);
    __CASE(SMARTCARD);
    __CASE(CONTENTSECURITY);
    __CASE(VIDEO);
    __CASE(PERSONALHEALTHCARE);
    __CASE(AUDIOVIDEO);
    __CASE(DIAGNOSTICDEVICE);
    __CASE(WIRELESSCONTROLLER);
    __CASE(MISCELLANEOUS);
    __CASE(APPLICATIONSPECIFIC);
    __CASE(VENDORSPECIFIC);
    default: return "UNKNOWN";
#undef __CASE
  }
}

static inline const char *usb_feature_to_string(int t)
{
#define DECL_CASE(__f) case USB_HUB_FEATURE_## __f: return #__f
  switch(t) {
    DECL_CASE(CONNECTION);
    DECL_CASE(ENABLE);
    DECL_CASE(SUSPEND);
    DECL_CASE(OVERCURRENT);
    DECL_CASE(RESET);
    DECL_CASE(PORT_POWER);
    DECL_CASE(LOWSPEED);
    DECL_CASE(HIGHSPEED);
    DECL_CASE(CONNECTION_CHANGE);
    DECL_CASE(ENABLE_CHANGE);
    DECL_CASE(SUSPEND_CHANGE);
    DECL_CASE(OVERCURRENT_CHANGE);
    DECL_CASE(RESET_CHANGE);
    default: return "UNKNOWN";
  }
#undef DECL_CASE
}

static inline const char *usb_descriptor_type_to_string(int t)
{
#define __CASE(__t) case USB_DESCRIPTOR_TYPE_ ##__t: return #__t;
  switch(t) {
    __CASE(DEVICE);
    __CASE(CONFIGURATION);
    __CASE(STRING);
    __CASE(INTERFACE);
    __CASE(ENDPOINT);
    __CASE(QUALIFIER);
    __CASE(OTHERSPEED_CONFIG);
    __CASE(INTERFACE_POWER);
    __CASE(HID);
    __CASE(HID_REPORT);
    __CASE(HID_PHYSICAL);
    __CASE(HUB);
    default: return "UNKNOWN";
#undef  __CASE
  }
}

static inline const char *usb_endpoint_type_to_string(int t)
{
#define __CASE(__t) case USB_ENDPOINT_TYPE_ ##__t: return #__t;
  switch(t) {
    __CASE(CONTROL);
    __CASE(ISOCHRONOUS);
    __CASE(BULK);
    __CASE(INTERRUPT);
    default: return "UNKNOWN";
  }
#undef  __CASE
}

static inline const char *usb_endpoint_synch_type_to_string(int t)
{
#define __CASE(__t) case USB_EP_SYNC_TYPE_ ##__t: return #__t;
  switch(t) {
    __CASE(NONE);
    __CASE(ASYNC);
    __CASE(ADAPTIVE);
    __CASE(SYNC);
    default: return "UNKNOWN";
  }
#undef  __CASE
}

static inline const char *usb_endpoint_usage_type_to_string(int t)
{
#define __CASE(__t) case USB_EP_USAGE_TYPE_ ##__t: return #__t;
  switch(t) {
    __CASE(DATA);
    __CASE(FEEDBACK);
    __CASE(XDFEEDBACK);
    __CASE(RESERVED);
    default: return "UNKNOWN";
  }
#undef  __CASE
}

static inline const char *usb_endpoint_type_to_short_string(int t)
{
#define __CASE(__t, __short) case USB_ENDPOINT_TYPE_ ##__t: return __short
  switch(t) {
    __CASE(CONTROL    , "CTRL");
    __CASE(ISOCHRONOUS, "ISOC");
    __CASE(BULK       , "BULK");
    __CASE(INTERRUPT  , "INTR");
    default    : return "UNKN";
  }
#undef  __CASE
}

static inline const char *usb_direction_to_string(int d)
{
  if (d == USB_DIRECTION_OUT)
    return "OUT";
  if (d == USB_DIRECTION_IN)
    return "IN";
  return "UNK";
}

static inline const char *usb_speed_to_string(int s)
{
  switch (s) {
    case USB_SPEED_HIGH: return "HIGH";
    case USB_SPEED_FULL: return "FULL";
    case USB_SPEED_LOW : return "LOW";
    default: return "UNDEFINED";
  }
}
