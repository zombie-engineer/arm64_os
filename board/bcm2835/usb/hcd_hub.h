#pragma once

#include "hcd.h"
#include "usb_dev_rq.h"
#include "common.h"
#include "hcd_submit.h"
#include "hcd_constants.h"

#define HUBLOG(fmt, ...) printf("[USB HUB:%02d] " fmt __endline, h->d->address, ## __VA_ARGS__)
#define HUBPORTLOG(fmt, ...) printf("[USB HUB:%02d.%d] " fmt __endline, h->d->address, port, ## __VA_ARGS__)
#define HUBERR(fmt, ...) printf("[USB HUB:%d ERR] err: %d: " fmt __endline, h->d->address, err, ## __VA_ARGS__)

typedef struct usb_hcd_device_class_hub {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_device_class_hub);
  struct usb_hcd_device *d;
  struct usb_hcd_device_class_base base;
	struct usb_hub_descriptor descriptor;
  struct list_head children;
} usb_hub_t;

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
	err = usb_hcd_submit_cm(&h->d->pipe0, &pctl, __dst, __rq_size, rq, USB_CONTROL_MSG_TIMEOUT_MS, &num_bytes);\
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
