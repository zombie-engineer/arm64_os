#pragma once

#include <drivers/usb/hcd.h>
#include <drivers/usb/usb_dev_rq.h>
#include <memory/static_slot.h>
#include "common.h"

#define USB_CONTROL_MSG_TIMEOUT_MS 10

#define _FMT_PREFIX_HUB          "[USB hub:%02d] "
#define _FMT_PREFIX_HUB_ERR      "[USB hub:%02d err:%d] "
#define _FMT_PREFIX_HUB_PORT     "[USB hub:%02d port:%02d] "
#define _FMT_PREFIX_HUB_PORT_ERR "[USB hub:%02d port:%02d err:%d] "

#define _FMT_ARG_HUB           h->d->address
#define _FMT_ARG_HUB_ERR       h->d->address, err
#define _FMT_ARG_HUB_PORT      h->d->address, port
#define _FMT_ARG_HUB_PORT_ERR  h->d->address, port, err

#define IF_DBG if (usb_hcd_log_level)

#define _PRNT(__t, __fmt, ...)\
  printf(_FMT_PREFIX_ ## __t __fmt __endline, _FMT_ARG_ ## __t, ## __VA_ARGS__)

#define HUBLOG(__fmt, ...)            _PRNT(HUB         , __fmt, ##__VA_ARGS__)
#define HUBDBG(__fmt, ...)     IF_DBG _PRNT(HUB         , __fmt, ##__VA_ARGS__)
#define HUBERR(__fmt, ...)            _PRNT(HUB_ERR     , __fmt, ##__VA_ARGS__)
#define HUBPORTLOG(__fmt, ...)        _PRNT(HUB_PORT    , __fmt, ##__VA_ARGS__)
#define HUBPORTDBG(__fmt, ...) IF_DBG _PRNT(HUB_PORT    , __fmt, ##__VA_ARGS__)
#define HUBPORTERR(__fmt, ...)        _PRNT(HUB_PORT_ERR, __fmt, ##__VA_ARGS__)

typedef struct usb_hcd_device_class_hub {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_device_class_hub);
  struct usb_hcd_device *d;
  struct usb_hcd_device_class_base base;
	struct usb_hub_descriptor descriptor;
  struct list_head children;
} usb_hub_t;

struct usb_hcd_device_class_hub *usb_hcd_allocate_hub();
void usb_hcd_deallocate_hub(struct usb_hcd_device_class_hub *h);

void usb_hcd_hub_init();

int usb_hub_enumerate(struct usb_hcd_device *dev);

/*
 * port given in 0-based form
 */
int usb_hub_enumerate_port_reset(usb_hub_t *h, int port);

static inline usb_hub_t *usb_hcd_device_to_hub(struct usb_hcd_device *d)
{
  return container_of(d->class, usb_hub_t, base);
}

int usb_hcd_hub_device_to_string(usb_hub_t *h, const char *prefix, char *buf, int bufsz);

#define DECL_RQ(__type, __rq, __value, __index, __len)\
  uint64_t rq = USB_DEV_RQ_MAKE(__type, __rq, __value, __index, __len)

#define HUBFN(__direction, __rq_type, __rq, __rq_value, __rq_index, __rq_size, __dst)\
  int err;\
  int num_bytes;\
  DECL_PCTL(CONTROL, __direction, 0);\
  DECL_RQ(HUB_ ## __rq_type, __rq, __rq_value, __rq_index, __rq_size);\
	err = HCD_TRANSFER_CONTROL(&h->d->pipe0, &pctl, __dst, __rq_size, rq, USB_CONTROL_MSG_TIMEOUT_MS, &num_bytes);\
  if (err != ERR_OK) {\
    HUBERR("request '"#__rq_type "-" #__rq "' v:%d,i:%d,l:%d failed", __rq_value, __rq_index, __rq_size);\
    err = ERR_GENERIC;\
    goto out_err;\
  }\
  if (num_bytes != __rq_size) {\
    HUBERR("request '"#__rq_type "-" #__rq "' v:%d,i:%d,l:%d size mismatch: wanted %d, got %d",\
      __rq_value, __rq_index, __rq_size, __rq_size, num_bytes);\
    err = ERR_GENERIC;\
  }\
out_err:\
  return err

static inline int usb_hub_set_feature(usb_hub_t *h, int feature)
{
  HUBFN(OUT, SET_HUB_FEATURE, SET_FEATURE, feature, 0, 0, NULL);
}

static inline int usb_hub_clear_feature(usb_hub_t *h, int feature)
{
  HUBFN(OUT, SET_HUB_FEATURE, CLEAR_FEATURE, feature, 0, 0, NULL);
}

static inline int usb_hub_get_status(usb_hub_t *h, struct usb_hub_status *status)
{
  HUBFN(IN, GET_HUB_STATUS, GET_STATUS, 0, 0, sizeof(*status), status);
}

/*
 * Hub port transmission functions.
 * These do the actual request transmissions.
 * Note that port indices are globally 0-based, but for transmission we need
 * 1-based port values, so we explicitly do the conversion here.
 * By design this should be the only place port numbers are converted.
 */
#define BASE0_TO_BASE1(__port) (__port + 1)

static inline int usb_hub_port_set_feature(usb_hub_t *h, int port, int feature)
{
  HUBFN(OUT, SET_PORT_FEATURE, SET_FEATURE, feature, BASE0_TO_BASE1(port), 0, NULL);
}

static inline int usb_hub_port_clear_feature(usb_hub_t *h, int port, int feature)
{
  HUBFN(OUT, SET_PORT_FEATURE, CLEAR_FEATURE, feature, BASE0_TO_BASE1(port), 0, NULL);
}

static inline int usb_hub_port_get_status(usb_hub_t *h, int port, struct usb_hub_port_status *status)
{
  HUBFN(IN, GET_PORT_STATUS, GET_STATUS, 0, BASE0_TO_BASE1(port), sizeof(*status), status);
}

static inline int usb_hub_port_power_on(usb_hub_t *h, int port)
{
  return usb_hub_port_set_feature(h, port, USB_HUB_FEATURE_PORT_POWER);
}
