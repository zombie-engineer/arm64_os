#pragma once
#include <list.h>
#include <usb/usb.h>
#include <usb/usb_printers.h>
#include <memory/static_slot.h>

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

#define USB_MAX_INTERFACES_PER_DEVICE 8
#define USB_MAX_ENDPOINTS_PER_DEVICE 16

extern int usb_hcd_print_debug;
struct usb_hcd_device;
extern struct usb_hcd_device *root_hub;

#define HCDLOGPREFIX(__log_level) "[USBHCD "#__log_level"] "

#define HCDLOG(__fmt, ...)     printf(HCDLOGPREFIX(INFO) __fmt __endline, ##__VA_ARGS__)
#define HCDERR(__fmt, ...)     logf(HCDLOGPREFIX(ERR), "err: %d: "__fmt, err, ##__VA_ARGS__)
#define HCDDEBUG(__fmt, ...)   if (usb_hcd_print_debug)\
                               printf(HCDLOGPREFIX(DBG) __fmt __endline, ##__VA_ARGS__)
#define HCDWARN(__fmt, ...)    logf(HCDLOGPREFIX(WARN), __fmt, ##__VA_ARGS__)

struct usb_hcd_pipe {
  int address;
  int endpoint;
  int speed;
  int max_packet_size;
  int ls_hub_port;
  int ls_hub_address;
};

struct usb_hcd_pipe_control {
  int transfer_type;
  int channel;
  int direction;
};

#define DECL_PCTL(__ep_type, __ep_dir, __channel)\
	struct usb_hcd_pipe_control pctl = {\
		.channel = __channel,\
		.transfer_type = USB_ENDPOINT_TYPE_ ## __ep_type,\
		.direction = USB_DIRECTION_ ## __ep_dir,\
	};

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
  struct usb_hcd_pipe_control pipe0_control;

  /*
   * This device is added to a parent's hub list of it's children
   */
  struct list_head hub_children;

  struct usb_device_descriptor descriptor ALIGNED(4);
  struct usb_hcd_interface interfaces[USB_MAX_INTERFACES_PER_DEVICE] ALIGNED (4);
  struct usb_hcd_device_class_base *class; 

  char string_manufacturer[64];
  char string_product[64];
  char string_serial[64];
  char string_configuration[64];
};

struct usb_hcd_device *usb_hcd_allocate_device();

void usb_hcd_deallocate_device(struct usb_hcd_device *d);

int usb_hcd_enumerate_device(struct usb_hcd_device *dev);

int usb_hcd_device_to_string(struct usb_hcd_device *dev, const char *prefix, char *buf, int bufsz);

const char *usb_hcd_device_class_to_string(int c);

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

int usb_hcd_submit_cm(
  struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl,
  void *buf,
  int buf_sz,
  uint64_t rq,
  int timeout,
  int *out_num_bytes);

int usb_hcd_init();
int usb_hcd_start();
