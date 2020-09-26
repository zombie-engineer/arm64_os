#pragma once

int usbd_init(void);

void usbd_print_device_tree(void);

void usbd_monitor(void);

struct usb_hcd_device;

/*
 * Topological or hierarchial address of a device, that enables search,
 * by specifying recursive port numbers.
 * USB hierarchy is backed by a physical composition of hubs connected to
 * parent hub ports, so it is possible to address to a specific device by
 * it's address starting from the port number on the root hub and walking
 * down the series of other hub ports until the required device is attached.
 */
struct usb_topo_addr {
  /*
   * array of port numbers starting from root hub port.
   */
  char ports[16];

  /*
   * actual number of valid entries in ports array until the end device.
   */
  int depth;
};

void usb_hcd_get_topo_addr(const struct usb_hcd_device *d, struct usb_topo_addr *ta);

int usb_topo_addr_to_string(char *buf, int bufsz, const struct usb_topo_addr *ta);

struct usb_hcd_device *usbd_find_device_by_topo_addr(const struct usb_topo_addr *ta);

/*
 * List all usb devices and apply function fn to each device
 */
#define USB_ITER_CONTINUE 0
#define USB_ITER_STOP     1
void usb_iter_devices(int (*fn)(struct usb_hcd_device *, void *), void *fn_arg);
