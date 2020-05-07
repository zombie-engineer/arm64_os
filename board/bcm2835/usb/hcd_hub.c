#include "hcd_hub.h"
#include <delays.h>
#include <bits_api.h>

DECL_STATIC_SLOT(struct usb_hcd_device_class_hub, usb_hcd_device_class_hub, 12)

static int usb_hcd_hub_device_to_string_r(struct usb_hcd_device_class_hub *hub, const char *prefix, char *buf, int bufsz, int depth)
{
  int n = 0;
  n = snprintf(buf + n, bufsz - n, "%shub:"__endline, prefix);
  return n;
}

int usb_hcd_hub_device_to_string(struct usb_hcd_device_class_hub *hub, const char *prefix, char *buf, int bufsz)
{
  return usb_hcd_hub_device_to_string_r(hub, prefix, buf, bufsz, 0);
}

struct usb_hcd_device_class_hub *usb_hcd_allocate_hub()
{
  struct usb_hcd_device_class_hub *hub;
  hub = usb_hcd_device_class_hub_alloc();
  return hub;
}

void usb_hcd_deallocate_hub(struct usb_hcd_device_class_hub *h)
{
  struct usb_hcd_device *child, *tmp;
  list_for_each_entry_safe(child, tmp, &h->children, hub_children) {
    list_del_init(&child->hub_children);
    usb_hcd_deallocate_device(child);
  }
  usb_hcd_device_class_hub_release(h);
}

int usb_hub_enumerate_port_reset(usb_hub_t *h, int port)
{
  /*
   * port reset procedure by USB 2.0 spec
   */
  int err = ERR_GENERIC;
  int reset_try = 0;
  int wait_enabled_try = 0;
	struct usb_hub_port_status port_status ALIGNED(4);

  for (reset_try = 0; reset_try < 3; ++reset_try) {
    err = usb_hub_port_set_feature(h, port, USB_HUB_FEATURE_RESET);
    CHECK_ERR_SILENT();

    for (wait_enabled_try = 0; wait_enabled_try < 3; ++wait_enabled_try) {
      wait_msec(20);
      err = usb_hub_port_get_status(h, port, &port_status);
      if (err != ERR_OK) {
        HUBPORTERR("failed to read status");
        continue;
      }

      if (port_status.change.reset_changed) {
        HUBPORTDBG("RESET_CHANGED");
        err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_RESET_CHANGE);
        if (err != ERR_OK) {
          HUBPORTERR("failed to clear status");
          goto out_err;
        }
      }
      if (port_status.change.enabled_changed)
        HUBPORTDBG("port: %d ENABLED CHANGED", port);
      if (port_status.status.enabled) {
        HUBPORTDBG("port: %d ENABLED", port);
        goto out_err;
      }
    } 
  }
out_err:
  return err;
}

static int usb_hub_enumerate_conn_changed(usb_hub_t *h, int port)
{
  int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);
  struct usb_hcd_device *port_dev = NULL;

  err = usb_hub_port_get_status(h, port, &port_status);
  CHECK_ERR_SILENT();
  HUBPORTLOG("status: %04x:%04x", port_status.status, port_status.change);

  err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_CONNECTION_CHANGE);
  CHECK_ERR_SILENT();
  HUBPORTLOG("CONNECTION_CHANGE cleared");
  
  err = usb_hub_enumerate_port_reset(h, port);
  CHECK_ERR_SILENT();
  HUBPORTLOG("reset and enabled successfully");

  port_dev = usb_hcd_allocate_device();
  if (!port_dev) {
    HUBPORTERR("failed to allocate device");
    goto out_err;
  }
  port_dev->state = USB_DEVICE_STATE_POWERED;
  port_dev->location.hub = h->d;
  port_dev->location.hub_port = port;

  err = usb_hub_port_get_status(h, port, &port_status);
  CHECK_ERR_SILENT();
  HUBPORTLOG("status: %04x:%04x", port_status.status, port_status.change);

	if (port_status.status.high_speed)
    port_dev->pipe0.speed = USB_SPEED_HIGH;
  else if (port_status.status.low_speed) {
		port_dev->pipe0.speed = USB_SPEED_LOW;
		port_dev->pipe0.ls_node_point = h->d->pipe0.address;
		port_dev->pipe0.ls_node_port = port;
	}
	else 
    port_dev->pipe0.speed = USB_SPEED_FULL;

  HUBPORTLOG("device speed: %s(%d)", usb_speed_to_string(port_dev->pipe0.speed));

  err = usb_hcd_enumerate_device(port_dev);
  if (err != ERR_OK) {
    int saved_err = err;
    HCDERR("failed to enumerate device");
    //usb_deallocate_device(port_dev);
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_ENABLE);
    if (err != ERR_OK)
      HUBERR("failed to disable port %d", port);
    err = saved_err;
    goto out_err;
  }

out_err:
  HCDLOG("completed with status:%d", err);
  return err;
}

#define USB_HUB_CHK_AND_CLR(__fld, __feature)\
	if (port_status.change.__fld) {\
		HUBPORTDBG("'"#__fld"' detected. clearing...");\
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_ ## __feature);\
    CHECK_ERR_SILENT();\
  }

int usb_hub_port_check_connection(usb_hub_t *h, int port)
{
	int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);

  err = usb_hub_port_get_status(h, port, &port_status);
  CHECK_ERR_SILENT();

  HUBPORTDBG("status:%04x:%04x", port_status.status.raw, port_status.change.raw);
	if (port_status.change.connected_changed) {
    HUBPORTDBG("CONNECTED status changed");
  	err = usb_hub_enumerate_conn_changed(h, port);
    CHECK_ERR_SILENT();
    //HCDERR("LOGIC ERROR: port status not CONNECTED CHANGED");
    // return 0;
  }

  USB_HUB_CHK_AND_CLR(enabled_changed     , ENABLE_CHANGE);
  USB_HUB_CHK_AND_CLR(suspended_changed   , SUSPEND_CHANGE);
  USB_HUB_CHK_AND_CLR(overcurrent_changed , OVERCURRENT_CHANGE);
  USB_HUB_CHK_AND_CLR(reset_changed       , RESET_CHANGE);
out_err:
  HUBPORTDBG("check connection status: %d", err);
  return err;
}

int usb_hub_enumerate(struct usb_hcd_device *dev)
{
  int port, err, num_bytes;
  struct usb_hub_status status ALIGNED(4);
  struct usb_hcd_device_class_hub *h = NULL;
  HUBDBG("=============================================================");
  HUBDBG("===================== ENUMERATE HUB =========================");
  HUBDBG("=============================================================");
  h = usb_hcd_allocate_hub();

  if (!h) {
    HCDERR("failed to allocate hub device object");
    err = ERR_BUSY;
    goto out_err;
  }

  h->d = dev;
  dev->class = &h->base;

  GET_DESC(&dev->pipe0, HUB, 0, 0, &h->descriptor, sizeof(h->descriptor));

  err = usb_hub_get_status(h, &status);
  CHECK_ERR("failed to read hub port status. Enumeration will not continue");

	HUBDBG("powering on all %d ports", h->descriptor.port_count);
  for (port = 0; port < h->descriptor.port_count; ++port) {
	  HUBPORTDBG("powering on port");
    err = usb_hub_port_power_on(h, port);
    if (err != ERR_OK) {
      HUBPORTERR("failed to power on, skipping");
      continue;
    }
  }
  wait_msec(h->descriptor.power_good_delay * 2);

  for (port = 0; port < h->descriptor.port_count; ++port) {
    HUBPORTDBG("check connection");
    err = usb_hub_port_check_connection(h, port);
    if (err != ERR_OK) {
      HUBPORTERR("failed check connection, skipping");
      continue;
    }
  }

out_err:
  if (err != ERR_OK && h) {
    dev->class = NULL;
    usb_hcd_deallocate_hub(h);
  }

  HUBDBG("=============================================================");
  HUBDBG("=================== ENUMERATE HUB END =======================");
  HUBDBG("=============================================================");
  return err;
}

void usb_hcd_hub_init()
{
  STATIC_SLOT_INIT_FREE(usb_hcd_device_class_hub);
}
