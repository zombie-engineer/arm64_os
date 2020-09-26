#include <drivers/usb/usbd.h>
#include <drivers/usb/hcd.h>
#include <drivers/usb/hcd_hub.h>
#include <error.h>
#include <common.h>
#include <sched.h>

int usbd_init(void)
{
  int err;
  err = usb_hcd_init();
  if (err != ERR_OK) {
    printf("Failed to init USB HCD, err: %d\r\n", err);
  }
  return err;
}

static void usbd_print_device_summary(const char *padding, const char *subpadding, const struct usb_topo_addr *ta, struct usb_hcd_device *d)
{
  char topoaddr[32];
  usb_topo_addr_to_string(topoaddr, sizeof(topoaddr), ta);
  printf("%s%s: addr:%02d id:%04x:%04x class:%s(%d) conf:%d" __endline,
    padding,
    topoaddr,
    d->address,
    d->descriptor.id_vendor,
    d->descriptor.id_product,
    d->class ? usb_hcd_device_class_to_string(d->class->device_class) : "NONE",
    d->class ? d->class->device_class : 0,
    d->descriptor.num_configurations);

  printf("%s%smanufacturer: '%s', product: '%s', serial: '%s'" __endline,
    padding,
    subpadding,
    d->string_manufacturer, d->string_product, d->string_serial);
}

static void usbd_print_device_recursive(struct usb_hcd_device *d, int depth)
{
  usb_hub_t *h;
  int i, e;
  struct usb_topo_addr ta;
  struct usb_hcd_device *child;
  char padding[128];
  char *subpadding = "  ";
  prefix_padding_to_string(" ", ' ', depth, 2, padding, sizeof(padding));
  usb_hcd_get_topo_addr(d, &ta);
  usbd_print_device_summary(padding, subpadding, &ta, d);

  for (i = 0; i < d->num_interfaces; ++i) {
    struct usb_hcd_interface *iface = &d->interfaces[i];
    const char *class_string = usb_full_class_to_string(
      iface->descriptor.class,
      iface->descriptor.subclass,
      iface->descriptor.protocol);
    printf("%s%sinterface:%s" __endline, padding, subpadding, class_string);
    for (e = 0; e < iface->descriptor.endpoint_count; ++e) {
      struct usb_hcd_endpoint *ep = hcd_interface_get_endpoint(iface, e);
      if (ep)
        printf("%s%sep%d,dir:%s,type:%s,mps:%d,interval:%d"__endline, padding, subpadding, 
          ep->descriptor.endpoint_address & 0xf,
          usb_direction_to_string((ep->descriptor.endpoint_address >> 7) & 1),
          usb_transfer_type_to_string(ep->descriptor.attributes & 3),
          ep->descriptor.max_packet_size,
          ep->descriptor.interval
        );
      else
        printf("%s%sep"__endline, padding, subpadding);
    }
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

void usbd_print_device_tree(void)
{
  usbd_print_device_recursive(root_hub, 0 /* recursive depth */);
}

void usbd_monitor(void)
{
  struct usb_hcd_device *d1, *d2;
  usb_hub_t *h1, *h2;

  struct usb_topo_addr ta1 = {
    .ports = { 0, 0 },
    .depth = 2
  };

  struct usb_topo_addr ta2 = {
    .ports = { 0, 0, 0 },
    .depth = 3
  };

  d1 = usbd_find_device_by_topo_addr(&ta1);
  if (!d1) {
    printf("failed to find d1\n");
    while(1);
  }

  d2 = usbd_find_device_by_topo_addr(&ta2);
  if (!d2) {
    printf("failed to find d2\n");
    while(1);
  }

  usbd_print_device_summary("-", "--", &ta1, d1);
  usbd_print_device_summary("-", "--", &ta2, d2);
  BUG(!d1->class || d1->class->device_class != USB_HCD_DEVICE_CLASS_HUB, "found device not a hub");
  BUG(!d2->class || d2->class->device_class != USB_HCD_DEVICE_CLASS_HUB, "found device not a hub");
  h1 = usb_hcd_device_to_hub(d1);
  h2 = usb_hcd_device_to_hub(d2);

  while(1) {
    wait_on_timer_ms(1000);
    usb_hub_probe_ports(h1);
    // usbd_print_device_tree();
    usb_hub_probe_ports(h2);
  }
}

void usb_hcd_get_topo_addr(const struct usb_hcd_device *d, struct usb_topo_addr *ta)
{
  int i = -1;
  const struct usb_hcd_device *dtmp = d;
  char ports[ARRAY_SIZE(ta->ports)];
  // printf("usb_hcd_get_topo_addr\n");
  while(dtmp) {
    // printf("dtmp:%p, parent:%p\n", dtmp, dtmp->location.hub);
    ports[++i] = dtmp->location.hub_port;
    dtmp = dtmp->location.hub;
  }
  ta->depth = 0;
  while(i + 1) {
    // printf("ta->ports:%d\n", i);
    ta->ports[ta->depth++] = ports[i--];
  }
}

struct usb_hcd_device *usbd_find_device_by_topo_addr(const struct usb_topo_addr *addr)
{
  int i;
  usb_hub_t *hub;
  struct usb_hcd_device *d = root_hub;
  printf("root_hub: %p, class: %p\n", d, d->class);

  for (i = 0; i + 1 < addr->depth; ++i) {
    puts("-");
    if (!d->class || d->class->device_class != USB_HCD_DEVICE_CLASS_HUB) {
      puts("!");
      return NULL;
    }
    hub = usb_hcd_device_to_hub(d);
    d = usb_hub_get_device_at_port(hub, addr->ports[i]);
    if (!d) {
      puts("<");
      return NULL;
    }
  }
  return d;
}

int usb_topo_addr_to_string(char *buf, int bufsz, const struct usb_topo_addr *ta)
{
  int n = 0;
  int i;
  // printf("usb_topo_addr_to_string: ta:depth:%d\n", ta->depth);
  for (i = 0; i + 1 < ta->depth; ++i) {
    n += snprintf(buf + n, bufsz - n, "%02d.", ta->ports[i]);
    // printf("usb_topo_addr_to_string: ta:depth:%d-\n", ta->depth);
  }

  n += snprintf(buf + n, bufsz - n, "%02d", ta->ports[i]);
  // printf("usb_topo_addr_to_string: ta:depth:%d--\n", ta->depth);
  return n;
}

void usb_iter_devices(int (*fn)(struct usb_hcd_device *, void *), void *fn_arg)
{
  usb_hcd_iter_devices(fn, fn_arg);
}
