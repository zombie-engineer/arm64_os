#pragma once
#include <list.h>
#include <usb/usb.h>
#include <usb/usb_pid.h>
#include <usb/usb_printers.h>
#include <memory/static_slot.h>
#include <usb/usb_descriptor_helpers.h>

/*
 * Looks like the device class is not really needed
 * because it a bit duplicates device/interfaces classes
 * in USB descritors spec. But device class in usb device
 * descriptor does not have HID, so this abstraction
 * of HCD device a bit different that the one in
 * descriptors.
 */
#define USB_HCD_DEVICE_CLASS_UNDEFINED 0
#define USB_HCD_DEVICE_CLASS_HUB       1
#define USB_HCD_DEVICE_CLASS_HID       2
#define USB_HCD_DEVICE_CLASS_MASS      3

#define USB_MAX_INTERFACES_PER_DEVICE 8
#define USB_MAX_ENDPOINTS_PER_DEVICE 16

extern int usb_hcd_log_level;
struct usb_hcd_device;
extern struct usb_hcd_device *root_hub;

#define HCDLOGPREFIX(__log_level) "[USBHCD "#__log_level"] "

#define HCDLOG(__fmt, ...)     printf(HCDLOGPREFIX(INFO) __fmt __endline, ##__VA_ARGS__)
#define HCDERR(__fmt, ...)     logf(HCDLOGPREFIX(ERR), "%s: err: %d: "__fmt, __func__, err, ##__VA_ARGS__)
#define HCDDEBUG(__fmt, ...)   if (usb_hcd_log_level)\
                               printf(HCDLOGPREFIX(DBG) "%s: " __fmt __endline, __func__, ##__VA_ARGS__)
#define HCDDEBUG2(__fmt, ...)  if (usb_hcd_log_level > 1)\
                                 printf(HCDLOGPREFIX(DBG) "%s: " __fmt __endline, __func__, ##__VA_ARGS__)
#define HCDWARN(__fmt, ...)    logf(HCDLOGPREFIX(WARN), "%s: "__fmt, __func__, ##__VA_ARGS__)

struct usb_hcd_pipe {
  int device_address;
  usb_speed_t device_speed;
  int endpoint_num;
  int endpoint_type;
  int endpoint_dir;
  int max_packet_size;
  int ls_hub_port;
  int ls_hub_address;
  int channel;
  usb_pid_t next_pid;
};

struct usb_hcd_device_class_base {
  int device_class;
};

struct usb_hcd_device_location {
  struct usb_hcd_device *hub;
  int hub_port;
};

struct usb_hcd_endpoint {
  struct usb_hcd_device *device;
  struct usb_endpoint_descriptor descriptor ALIGNED(4);
  struct usb_hcd_pipe pipe;
};

static inline int hcd_endpoint_get_number(struct usb_hcd_endpoint *ep)
{
  return usb_endpoint_descriptor_get_number(&ep->descriptor);
}

static inline int hcd_endpoint_get_direction(struct usb_hcd_endpoint *ep)
{
  return usb_endpoint_descriptor_get_direction(&ep->descriptor);
}

static inline int hcd_endpoint_get_max_packet_size(struct usb_hcd_endpoint *ep)
{
  return usb_endpoint_descriptor_get_max_packet_size(&ep->descriptor);
}

static inline int hcd_endpoint_get_type(struct usb_hcd_endpoint *ep)
{
  return usb_endpoint_descriptor_get_type(&ep->descriptor);
}

int hcd_endpoint_clear_feature(struct usb_hcd_endpoint *ep, int feature);

int hcd_endpoint_set_feature(struct usb_hcd_endpoint *ep, int feature);

int hcd_endpoint_get_status(struct usb_hcd_endpoint *ep, void *status);

struct usb_hcd_interface {
  struct usb_interface_descriptor descriptor;
  struct usb_hcd_endpoint endpoints[USB_MAX_ENDPOINTS_PER_DEVICE] ALIGNED(4);
};

static inline int hcd_interface_get_num_endpoints(struct usb_hcd_interface *i)
{
  return i->descriptor.endpoint_count;
}

static inline struct usb_hcd_endpoint *hcd_interface_get_endpoint(struct usb_hcd_interface *i, int index)
{
  if (hcd_interface_get_num_endpoints(i) <= index)
    return NULL;
  return &i->endpoints[index];
}

struct usb_hcd_device {
  struct list_head STATIC_SLOT_OBJ_FIELD(usb_hcd_device);
  struct list_head usb_devices;

  /*
   * unique address one of 127 addresses, assigned at transition
   * to ADDRESSED state
   * */
  int address;
  int state;

#define USB_DEVICE_STATE_DETACHED   0
#define USB_DEVICE_STATE_ATTACHED   1
#define USB_DEVICE_STATE_POWERED    2
#define USB_DEVICE_STATE_DEFAULT    3
#define USB_DEVICE_STATE_ADDRESSED  4
#define USB_DEVICE_STATE_CONFIGURED 5
#define USB_DEVICE_STATE_SUSPENDED  6

  int configuration;
  int configuration_string;
  struct usb_hcd_device_location location;
  struct usb_hcd_pipe pipe0;

  /*
   * This device is added to a parent's hub list of it's children
   */
  struct list_head hub_children;

  struct usb_device_descriptor descriptor ALIGNED(4);

  int num_interfaces;
  struct usb_hcd_interface interfaces[USB_MAX_INTERFACES_PER_DEVICE] ALIGNED (4);
  struct usb_hcd_device_class_base *class;

  char string_manufacturer[64];
  char string_product[64];
  char string_serial[64];
  char string_configuration[64];
};

static inline int hcd_device_get_class(struct usb_hcd_device *d)
{
  return d->class ? d->class->device_class : -1;
}

static inline int hcd_endpoint_get_address(struct usb_hcd_endpoint *ep)
{
  return ep->device->address;
}

static inline int hcd_device_get_num_interfaces(struct usb_hcd_device *d)
{
  return d->num_interfaces;
}

static inline int hcd_device_get_state(struct usb_hcd_device *d)
{
  return d->state;
}

static inline int usb_hcd_get_interface_class(struct usb_hcd_device *d, int interface_num)
{
  if (interface_num >= d->num_interfaces)
    return -1;
  return d->interfaces[interface_num].descriptor.class;
}

static inline int hcd_device_get_vendor_id(struct usb_hcd_device *d)
{
  return d->descriptor.id_vendor;
}

static inline int hcd_device_get_product_id(struct usb_hcd_device *d)
{
  return d->descriptor.id_product;
}

static inline struct usb_hcd_interface *hcd_device_get_interface(struct usb_hcd_device *d, int index)
{
  if (d->num_interfaces <= index)
    return NULL;
  return &d->interfaces[index];
}

struct usb_hcd_device *usb_hcd_allocate_device();

void usb_hcd_deallocate_device(struct usb_hcd_device *d);

int usb_hcd_enumerate_device(struct usb_hcd_device *dev);

int usb_hcd_device_to_string(struct usb_hcd_device *dev, const char *prefix, char *buf, int bufsz);

const char *usb_hcd_device_class_to_string(int c);

void hcd_device_set_speed(struct usb_hcd_device *d, usb_speed_t speed);

void usb_hcd_print_device(struct usb_hcd_device *dev);

int usb_hcd_get_descriptor(struct usb_hcd_pipe *p, int desc_type, int desc_idx, int lang_id,
    void *buf,
    int buf_sz,
    int *num_bytes);

#define CHECK_ERR_SILENT() if (err != ERR_OK) goto out_err

#define CHECK_ERR(__msg, ...)\
  if (err != ERR_OK) {\
    HCDLOG("err %d: "__msg, err, ##__VA_ARGS__);\
    goto out_err;\
  }

#define __TO_DESC_HDR(__buf) ((struct usb_descriptor_header *)__buf)

#define __CHECK_DESC_TYPE(__buf, __type)\
  if (__TO_DESC_HDR(__buf)->descriptor_type != USB_DESCRIPTOR_TYPE_## __type) {\
    HCDERR("descriptor mismatch: got %d, wanted %d",\
      usb_descriptor_type_to_string(__TO_DESC_HDR(__buf)->descriptor_type),\
      usb_descriptor_type_to_string(USB_DESCRIPTOR_TYPE_## __type));\
    err = ERR_GENERIC;\
    goto out_err;\
  }

#define GET_DESC(__pipe, __desc_type, __desc_idx, __index, __dst, __dst_sz)\
  err = usb_hcd_get_descriptor(__pipe, USB_DESCRIPTOR_TYPE_## __desc_type, __desc_idx, __index, __dst, __dst_sz, &num_bytes);\
  CHECK_ERR("failed to get descriptor "#__desc_type);\
  __CHECK_DESC_TYPE(__dst, __desc_type);

int hcd_transfer_control_blocking(
  struct usb_hcd_pipe *pipe,
  int direction,
  void *addr,
  int transfer_size,
  uint64_t rq,
  int *out_num_bytes);

typedef enum {
  HCD_TRANSFER_STAGE_SETUP_PACKET = 0,
  HCD_TRANSFER_STAGE_DATA_PACKET,
  HCD_TRANSFER_STAGE_ACK_PACKET,
  HCD_TRANSFER_STAGE_COMPLETED
} hcd_tranfer_stage_t;

struct hcd_transfer_status {
  hcd_tranfer_stage_t stage;
  int direction;
  void *addr;
  int transfer_size;
  void *priv;
  int err;
};

int hcd_transfer_control(
  struct usb_hcd_pipe *pipe,
  int direction,
  void *addr,
  int transfer_size,
  uint64_t rq,
  int *out_num_bytes);

#define HCD_TRANSFER_CONTROL(__p, __d, __a, __sz, __rq, __o)\
  hcd_transfer_control(__p, __d, __a, __sz, __rq, __o)

int hcd_transfer_interrupt(
  struct usb_hcd_pipe *pipe,
  void *buf,
  int buf_sz,
  int *out_num_bytes);

/*
 * manage bulk transfer
 * direction: IN/OUT
 * buf: source/desctination
 * sz: transfer size
 */
int hcd_transfer_bulk(
  struct usb_hcd_endpoint *ep,
  void *buf,
  int sz,
  int *out_num_bytes);

int usb_hcd_init();

int usb_hcd_start();

int usb_hcd_set_log_level(int);

void usb_hcd_print_intr_regs(void);

#define USB_HCD_ITER_CONTINUE 0
#define USB_HCD_ITER_STOP     1
void usb_hcd_iter_devices(int (*fn)(struct usb_hcd_device *, void *), void *fn_arg);

int usb_hcd_to_addressed_state(struct usb_hcd_device *d);

int usb_hcd_parse_configuration(struct usb_hcd_device *d, const void *config, int config_size);

int usb_hcd_get_descriptor_first(struct usb_hcd_device *d);

int usb_hcd_get_descriptor_second(struct usb_hcd_device *d);

int usb_hcd_to_configured_state(struct usb_hcd_device *d);

int usb_hcd_configure_class(struct usb_hcd_device *d);

int usb_hcd_allocate_device_address(void);
