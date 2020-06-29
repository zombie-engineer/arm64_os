#pragma once
#include <usb/usb.h>
#include <usb/usb_printers.h>

/*
 * USB device request helpers
 */

#define __PUTBITS64(__val, __mask, __shift) (((uint64_t)__val & __mask) << __shift)

struct usb_device_request {
  union {
    struct {
      uint8_t request_type;
      uint8_t request;
      uint16_t value;
      uint16_t index;
      uint16_t length;
    };
    uint64_t raw;
  };
} PACKED;


#define USB_DEVICE_REQUEST_TYPE_HOST2DEV 0
#define USB_DEVICE_REQUEST_TYPE_DEV2HOST 1

#define USB_DEVICE_REQUEST_TYPE_STANDARD 0
#define USB_DEVICE_REQUEST_TYPE_CLASS    1
#define USB_DEVICE_REQUEST_TYPE_VENDOR   2

#define USB_DEVICE_REQUEST_TYPE_DEVICE    0
#define USB_DEVICE_REQUEST_TYPE_INTERFACE 1
#define USB_DEVICE_REQUEST_TYPE_ENDPOINT  2
#define USB_DEVICE_REQUEST_TYPE_OTHER     3

#define USB_DEVICE_REQUEST_TYPE(__dir, __type, __recipient)\
  (char)( ((USB_DEVICE_REQUEST_TYPE_ ## __dir & 1) << 7)\
  | ((USB_DEVICE_REQUEST_TYPE_ ## __type & 3) << 5)\
  | ((USB_DEVICE_REQUEST_TYPE_ ## __recipient & 0xf)))

#define USB_DEV_RQ_MAKE(__type, __rq, __val, __idx, __len)\
  ( __PUTBITS64(USB_RQ_TYPE_ ## __type, 0xff  ,  0)\
   |__PUTBITS64(USB_RQ_      ## __rq  , 0xffff,  8)\
   |__PUTBITS64(                __val , 0xffff, 16)\
   |__PUTBITS64(                __idx , 0xffff, 32)\
   |__PUTBITS64(                __len , 0xffff, 48))

#define USB_DEV_RQ_MAKE_GET_DESCRIPTOR(__desc_type, __desc_idx, __idx, __len)\
  USB_DEV_RQ_MAKE(GET_DESCRIPTOR, GET_DESCRIPTOR,\
    ((__desc_type & 0xff) << 8|(__desc_idx & 0xff)), __idx, __len)

#define USB_DEV_HUB_RQ_MAKE_GET_DESCRIPTOR(__desc_type, __desc_idx, __idx, __len)\
  USB_DEV_RQ_MAKE(HUB_GET_HUB_DESCRIPTOR, GET_DESCRIPTOR,\
    ((__desc_type & 0xff) << 8|(__desc_idx & 0xff)), __idx, __len)

#define USB_DEV_HID_RQ_MAKE_GET_DESCRIPTOR(__desc_type, __desc_idx, __idx, __len)\
  USB_DEV_RQ_MAKE(GET_INTERFACE, GET_DESCRIPTOR,\
    ((__desc_type & 0xff) << 8|(__desc_idx & 0xff)), __idx, __len)

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
      subtype = usb_descriptor_type_to_string(value_hi);
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

static inline void usb_rq_print(uint64_t rq, const char *tag)
{
  printf("usb_device_request:%s,type:%02x,rq:%02x,value:%04x,idx:%04x,len:%04x",
      tag,
      rq,
      USB_DEV_RQ_GET_TYPE(rq),
      USB_DEV_RQ_GET_RQ(rq),
      USB_DEV_RQ_GET_VALUE(rq),
      USB_DEV_RQ_GET_INDEX(rq),
      USB_DEV_RQ_GET_LENGTH(rq));
}
