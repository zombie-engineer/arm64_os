#pragma once
#include <usb/usb.h>

/*
 * USB device request helpers
 */

#define USB_DEV_RQ_MAKE(type, rq, val, idx, len)\
  ((((uint64_t)type & 0x00ff) <<  0)\
  |(((uint64_t)rq   & 0x00ff) <<  8)\
  |(((uint64_t)val  & 0xffff) << 16)\
  |(((uint64_t)idx  & 0xffff) << 32)\
  |(((uint64_t)len  & 0xffff) << 48))

#define USB_DEV_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, idx, len)\
  USB_DEV_RQ_MAKE(\
    USB_RQ_TYPE_GET_DESCRIPTOR,\
    USB_RQ_GET_DESCRIPTOR,\
    ((desc_type & 0xff) << 8|(desc_idx & 0xff)), idx, len)

#define USB_DEV_HUB_RQ_MAKE_GET_DESCRIPTOR(desc_type, desc_idx, idx, len)\
  USB_DEV_RQ_MAKE(\
    USB_RQ_HUB_TYPE_GET_HUB_DESCRIPTOR,\
    USB_RQ_GET_DESCRIPTOR,\
    ((desc_type & 0xff) << 8|(desc_idx & 0xff)), idx, len)


#define USB_DEV_RQ_GET_TYPE(r)   ((r    )&0xff)
#define USB_DEV_RQ_GET_RQ(r)     ((r>>8 )&0xff)
#define USB_DEV_RQ_GET_VALUE(r)  ((r>>16)&0xffff)
#define USB_DEV_RQ_GET_INDEX(r)  ((r>>32)&0xffff)
#define USB_DEV_RQ_GET_LENGTH(r) ((r>>48)&0xffff)

static const char *usb_req_to_string(int req)
{
#define DECL_CASE(r) case USB_RQ_##r: return #r
  switch (req) {
    DECL_CASE(GET_STATUS);
    DECL_CASE(CLEAR_FEATURE);
    DECL_CASE(SET_FEATURE);
    DECL_CASE(SET_ADDRESS);
    DECL_CASE(GET_DESCRIPTOR);
    DECL_CASE(SET_DESCRIPTOR);
    DECL_CASE(GET_CONFIGURATION);
    DECL_CASE(SET_CONFIGURATION);
    DECL_CASE(GET_INTERFACE);
    DECL_CASE(SET_INTERFACE);
    DECL_CASE(SYNCH_FRAME);
    default: return "UNKNOWN";
  }
#undef DECL_CASE
}


static const char *usb_desc_type_to_string(int t)
{
#define DECL_CASE(type) case USB_DESCRIPTOR_TYPE_##type: return #type
  switch(t) {
    DECL_CASE(DEVICE);
    DECL_CASE(CONFIGURATION);
    DECL_CASE(STRING);
    DECL_CASE(INTERFACE);
    DECL_CASE(ENDPOINT);
    DECL_CASE(QUALIFIER);
    DECL_CASE(OTHERSPEED_CONFIG);
    DECL_CASE(INTERFACE_POWER);
    DECL_CASE(HID);
    DECL_CASE(HID_REPORT);
    DECL_CASE(HID_PHYSICAL);
    DECL_CASE(HUB);
    default: return "UNKNOWN";
  }
#undef DECL_CASE
}

static const char *usb_feature_to_string(int t)
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

static inline void usb_rq_get_description(uint64_t rq, char *buf, int buf_sz)
{
  int type   = USB_DEV_RQ_GET_TYPE(rq);
  int req    = USB_DEV_RQ_GET_RQ(rq);
  int value  = USB_DEV_RQ_GET_VALUE(rq);
  int index  = USB_DEV_RQ_GET_INDEX(rq);
  int length = USB_DEV_RQ_GET_LENGTH(rq);
  int value_hi = (value >> 8) & 0xff;
  int value_lo = value & 0xff;
  const char *subtype = NULL;

  switch(req) {
    case USB_RQ_GET_DESCRIPTOR:
      subtype = usb_desc_type_to_string(value_hi);
      break;
    case USB_RQ_CLEAR_FEATURE:
    case USB_RQ_SET_FEATURE:
      subtype = usb_feature_to_string(value);
      break;
    default:
      break;
  } 

  snprintf(buf, buf_sz, "%016x %s %s t:%02x r:%02x v:%04x(%d,%d) i:%04x l:%04x",
    rq, 
    usb_req_to_string(req),
    subtype ? subtype : "",
    type, 
    req,
    value, value_hi, value_lo,
    index, 
    length);
}

