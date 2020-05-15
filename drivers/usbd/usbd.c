#include <drivers/usb/usbd.h>
#include <drivers/usb/hcd.h>
#include <drivers/usb/hcd_hub.h>
#include <error.h>
#include <common.h>

int usbd_init()
{
  int err;
  err = usb_hcd_init();
  if (err != ERR_OK) {
    printf("Failed to init USB HCD, err: %d\r\n", err);
    goto out_err;
  }
//  err = usb_hcd_start();
//  if (err != ERR_OK) {
//    printf("Failed to start USB HCD, err: %d\r\n", err);
//    goto out_err;
//  }
out_err:
  return err;
}

static void usbd_print_device_recursive(struct usb_hcd_device *d, int depth)
{
  usb_hub_t *h;
  int i;
  struct usb_hcd_device *child;
  char padding[128];
  char *subpadding = "  ";
  prefix_padding_to_string(" ", ' ', depth, 2, padding, sizeof(padding)); 

  printf("%sport:%02d: USB device: addr=%02d id=%04x:%04x class=%s(%d)" __endline,
    padding,
    d->location.hub_port,
    d->address,
    d->descriptor.id_vendor,
    d->descriptor.id_product, 
    d->class ? usb_hcd_device_class_to_string(d->class->device_class) : "NONE",
    d->class ? d->class->device_class : 0);

  printf("%s%smanufacturer: '%s', product: '%s', serial: '%s'" __endline,
    padding,
    subpadding,
    d->string_manufacturer, d->string_product, d->string_serial);

  for (i = 0; i < d->num_interfaces; ++i) {
    struct usb_hcd_interface *iface = &d->interfaces[i];
    const char *class_string = usb_full_class_to_string(
      iface->descriptor.class,
      iface->descriptor.subclass,
      iface->descriptor.protocol);
    printf("%s%sinterface:%s" __endline, padding, subpadding, class_string);
  }
  if (d->class) {
    switch(d->class->device_class) {
      case USB_HCD_DEVICE_CLASS_HUB:
        h = usb_hcd_device_to_hub(d);
        printf("%s%snum_ports=%d" __endline, padding, 
          subpadding,
          h->descriptor.port_count);
        for (i = 0; i < h->descriptor.port_count; ++i) {
          list_for_each_entry(child, &h->children, hub_children) {
            if (child->location.hub_port == i)
              usbd_print_device_recursive(child, depth + 1);
          }
        }
      default:
        break;
    }
  }
  puts(__endline);
}

void usbd_print_device_tree()
{
  usbd_print_device_recursive(root_hub, 0 /* recursive depth */);
}
