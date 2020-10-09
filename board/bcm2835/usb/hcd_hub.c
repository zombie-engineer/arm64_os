#include <drivers/usb/hcd_hub.h>
#include <delays.h>
#include <bits_api.h>
#include <usb/usb_timings.h>

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

struct usb_hcd_device_class_hub *usb_hcd_hub_create()
{
  struct usb_hcd_device_class_hub *hub;
  hub = usb_hcd_device_class_hub_alloc();
  if (hub) {
    hub->base.device_class = USB_HCD_DEVICE_CLASS_HUB;
    memset(&hub->descriptor, 0, sizeof(hub->descriptor));
    INIT_LIST_HEAD(&hub->children);
  }
  return hub;
}

void usb_hcd_hub_destroy(struct usb_hcd_device_class_hub *h)
{
  struct usb_hcd_device *child, *tmp;
  list_for_each_entry_safe(child, tmp, &h->children, hub_children) {
    list_del_init(&child->hub_children);
    usb_hcd_deallocate_device(child);
  }
  usb_hcd_device_class_hub_release(h);
}

struct usb_hcd_device *usb_hub_get_device_at_port(usb_hub_t *h, int port)
{
  struct usb_hcd_device *d;
  list_for_each_entry(d, &h->children, hub_children) {
    if (d->location.hub_port == port)
      return d;
  }
  return NULL;
}

int usb_hub_port_reset(usb_hub_t *h, int port)
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
    wait_msec(TDRST_MS);

    for (wait_enabled_try = 0; wait_enabled_try < 3; ++wait_enabled_try) {
      err = usb_hub_port_get_status(h, port, &port_status);
      if (err != ERR_OK) {
        HUBPORTERR("failed to read status");
        continue;
      }
      wait_msec(TDRST_MS);

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

static int usb_hub_port_initialize(usb_hub_t *h, int port, struct usb_hub_port_status *port_status)
{
  int err = ERR_OK;
	// struct usb_hub_port_status port_status ALIGNED(4);
  struct usb_hcd_device *port_dev = NULL;

  // err = usb_hub_port_get_status(h, port, &port_status);
  // CHECK_ERR_SILENT();
  // HUBPORTDBG("status: %04x:%04x", port_status.status, port_status.change);

  // err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_CONNECTION_CHANGE);
  // CHECK_ERR_SILENT();
  HUBPORTDBG("CONNECTION_CHANGE cleared");

  // err = usb_hub_port_reset(h, port);
  // CHECK_ERR_SILENT();

  port_dev = usb_hcd_allocate_device();
  if (!port_dev) {
    HUBPORTERR("failed to allocate device");
    goto out_err;
  }
  port_dev->state = USB_DEVICE_STATE_POWERED;
  port_dev->location.hub = h->d;
  port_dev->location.hub_port = port;

  err = usb_hub_port_get_status(h, port, port_status);
  CHECK_ERR_SILENT();
  HUBPORTDBG("status: %04x:%04x", port_status->status, port_status->change);

	if (port_status->status.high_speed)
    port_dev->pipe0.device_speed = USB_SPEED_HIGH;
  else if (port_status->status.low_speed)
		port_dev->pipe0.device_speed = USB_SPEED_LOW;
	else
    port_dev->pipe0.device_speed = USB_SPEED_FULL;

  HUBPORTDBG("device speed: %s", usb_speed_to_string(port_dev->pipe0.device_speed));

  if (!port_status->status.high_speed) {
		port_dev->pipe0.ls_hub_address = h->d->address;
		port_dev->pipe0.ls_hub_port = port;
  }

  err = usb_hcd_enumerate_device(port_dev);
  if (err != ERR_OK) {
    int saved_err = err;
    HUBPORTERR("failed to enumerate device");
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_ENABLE);
    if (err != ERR_OK)
      HUBPORTERR("failed to disable port %d", port);
    err = saved_err;
    goto out_err;
  }

  list_add(&port_dev->hub_children, &h->children);
  HUBPORTDBG("hub: %p, adding device %p to port, h->children:%p", h, port_dev, &h->children);

out_err:
  if (err != ERR_OK) {
    HUBPORTERR("failed to handle CONNECTION_CHANGED status");
    if (port_dev)
      usb_hcd_deallocate_device(port_dev);
  }
  return err;
}

static int usb_hub_port_disconnected(usb_hub_t *h, int port)
{
  int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);
  struct usb_hcd_device *port_dev = NULL;

  err = usb_hub_port_get_status(h, port, &port_status);
  BUG(err != ERR_OK, "get_port_status failed at disonnection");
  HUBPORTLOG("disconnecting status: %04x:%04x", port_status.status, port_status.change);

  err = usb_hub_port_clear_feature(h, port, USB_PORT_STATUS_CH_BIT_CONNECTED_CHANGED);
  BUG(err != ERR_OK, "clear_port_status failed at disonnection");
  HUBPORTLOG("CONNECTION_CHANGE cleared");

  port_dev = usb_hub_get_device_at_port(h, port);
  BUG(!port_dev, "disconnected device not present in hub children list");
  usb_hcd_deallocate_device(port_dev);
  HUBPORTLOG("device disconnected");
  return err;
}

#define USB_HUB_CHK_AND_CLR(__fld, __feature)\
	if (port_status.change.__fld) {\
		HUBPORTDBG("'"#__fld"' detected. clearing...");\
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_ ## __feature);\
    CHECK_ERR_SILENT();\
  }

static inline bool timer_expired(uint64_t now_ms, uint64_t expires_at_ms)
{
  uint64_t delta = expires_at_ms - now_ms;
  if (delta == 0)
    return true;
  return ((delta >> 63) & 1) == 1;
}

#define CONNECTION_STATUS_CONNECTED 1
#define CONNECTION_STATUS_DISCONNECTED 2
#define CONNECTION_STATUS_CONNECTED_CHANGED 4
#define CONNECTION_STATUS_DISCONNECTED_CHANGED 8

int hub_port_wait_status(
  usb_hub_t *h,
  int port,
  int delay_ms_min,
  int delay_ms_max,
  int status_events,
  struct usb_hub_port_status *s)
{
  int err = ERR_OK;;
  char buf[256];
  uint64_t now = get_boottime_msec();
  uint64_t end = now + delay_ms_max;
  printf("%llu: starting to wait for %d milliseconds until %llu\r\n", now, delay_ms_max, end);


  while(1) {
    wait_msec(delay_ms_min);
    err = usb_hub_port_get_status(h, port, s);
    CHECK_ERR("usb_hub_port_check_connection: failed to get status");
    usb_status_to_string(s, buf, sizeof(buf));
    HUBPORTLOG("port_status(%s): port:%d, err:%d, %s", "wait", port, err, buf);
    if (status_events & CONNECTION_STATUS_CONNECTED_CHANGED) {
      if (s->change.connected_changed && s->status.connected)
        return ERR_OK;
    }

    if (status_events & CONNECTION_STATUS_DISCONNECTED_CHANGED) {
      if (s->change.connected_changed && !s->status.connected)
        return ERR_OK;
    }

    now = get_boottime_msec();
    if (timer_expired(now, end)) {
      //  printf("timeout:now:%d, end:%d\r\n", (int)now, (int)end);
      err = ERR_TIMEOUT;
      goto out_err;
    }
  }
out_err:
  return err;
}

int usb_hub_port_check_connection(usb_hub_t *h, int port)
{
	int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);

  err = usb_hub_port_get_status(h, port, &port_status);
  CHECK_ERR("usb_hub_port_check_connection: failed to get status");

  HUBPORTLOG("status:%04x:%04x", port_status.status.raw, port_status.change.raw);
	if (port_status.change.connected_changed) {
    HUBPORTLOG("CONNECTED status changed to %s",
      port_status.status.connected ? "CONNECTED" :"DISCONNECTED");

    if (port_status.status.connected) {
      wait_msec(50);
      err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_CONNECTION_CHANGE);
      CHECK_ERR_SILENT();
      HUBPORTLOG("CONNECTION_CHANGE cleared");
  	  err = usb_hub_port_initialize(h, port, &port_status);
      CHECK_ERR("failed to handle connection at port %d", port);
    } else {
      err = usb_hub_port_disconnected(h, port);
      CHECK_ERR("failed to handle device disconnection at port %d", port);
    }

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

int usb_hub_read_all_port_status(usb_hub_t *h, const char *tag)
{
  char buf[512];
  int err, port;
  struct usb_hub_port_status port_status ALIGNED(4);

  for (port = 0; port < h->descriptor.port_count; ++port) {
    memset(buf, 0, sizeof(buf));
    err = usb_hub_port_get_status(h, port, &port_status);
    usb_status_to_string(&port_status, buf, sizeof(buf));

    HUBPORTLOG("get_port_status(%s): port:%d,err:%d,%s", tag, port, err, buf);
  }
  return err;
}

static int usb_hub_change_port_power(usb_hub_t *h, int port, int power_on,
  struct usb_hub_port_status *port_status)
{
  int err;
  uint32_t delay;
  delay = hcd_hub_get_power_on_delay(h) * 2;

  HUBPORTLOG("powering %s port", power_on ? "on" : "off");
  if (power_on)
    err = usb_hub_port_set_feature(h, port, USB_HUB_FEATURE_PORT_POWER);
  else
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_PORT_POWER);
  CHECK_ERR("failed to power %s port", power_on ? "on" : "off");

  wait_msec(600);
  err = hub_port_wait_status(h, port, delay, delay * 5,
    power_on ? CONNECTION_STATUS_CONNECTED_CHANGED : CONNECTION_STATUS_DISCONNECTED_CHANGED,
    port_status);
  CHECK_ERR("failed to wait until disconnected.");
  HUBPORTLOG("Port has been powered %s", power_on ? "on" : "off");

  if (port_status->change.connected_changed) {
    wait_msec(10);
    err = usb_hub_port_clear_feature(h, port, USB_PORT_STATUS_CH_BIT_CONNECTED_CHANGED);
    CHECK_ERR("failed to clear feature 'conn_changed'.");
  }
  if (port_status->change.enabled_changed) {
    err = usb_hub_port_clear_feature(h, port, USB_PORT_STATUS_CH_BIT_ENABLED_CHANGED);
    CHECK_ERR("failed to clear feature 'ena_changed'.");
  }
out_err:
    return err;
}

static inline int usb_hub_get_port_status(usb_hub_t *h, int port, const char *tag,
  struct usb_hub_port_status *port_status)
{
  int err;
  char buf[256];
  err = usb_hub_port_get_status(h, port, port_status);
  CHECK_ERR("usb_hub_probe_one_port: failed to get status");
  usb_status_to_string(port_status, buf, sizeof(buf));
  HUBPORTLOG("port_status(%s): port:%d, err:%d, %s", tag, port, err, buf);
out_err:
  return err;
}

static inline int hub_get_status(usb_hub_t *h, const char *tag,
  struct usb_hub_status *hub_status)
{
  int err;
  err = usb_hub_get_status(h, hub_status);
  CHECK_ERR("usb_hub_probe_one_port: failed to get status");
  HUBLOG("status(%s): overcurrent:%d, err:%d", tag, err, hub_status->status.overcurrent);
out_err:
  return err;
}

static int usb_hub_probe_one_port(usb_hub_t *h, int port)
{
  int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);
  struct usb_hub_status hub_status ALIGNED(4);

  HUBPORTLOG("Configuring port");
  usb_hub_get_port_status(h, port, "probe_start", &port_status);
  hub_get_status(h, "probe_start", &hub_status);

  if (port_status.status.powered) {
    usb_hub_change_port_power(h, port, 0, &port_status);
    CHECK_ERR("Failed to power off port");
  }
  usb_hub_change_port_power(h, port, 1, &port_status);
  CHECK_ERR("failed to power on port");

  if (port_status.status.connected) {
    err = usb_hub_port_reset(h, port);
    CHECK_ERR("failed to reset port");

    hub_get_status(h, "after_reset", &hub_status);
	  err = usb_hub_port_initialize(h, port, &port_status);
    CHECK_ERR("failed to initialize port");
  }
out_err:
  return err;
}

int usb_hub_power_on_ports(usb_hub_t *h)
{
  int port, err;

  err = usb_hub_read_all_port_status(h, "before power on");

  for (port = 0; port < h->descriptor.port_count; ++port) {
    err = usb_hub_probe_one_port(h, port);
    if (err != ERR_OK)
      HUBPORTERR("failed to probe port. skipping to next port");
  }
  return err;
}

int usb_hub_enumerate(struct usb_hcd_device *dev)
{
  int err, num_bytes;
  struct usb_hcd_device_class_hub *h = NULL;
  struct usb_hub_status status ALIGNED(4);
  HCDDEBUG("=============================================================");
  HCDDEBUG("===================== ENUMERATE HUB =========================");
  HCDDEBUG("=============================================================");
  HCDLOG("HUB: %04x:%04x, mps:%d", dev->descriptor.id_vendor, dev->descriptor.id_product,
    dev->descriptor.max_packet_size_0);

  h = usb_hcd_hub_create();

  if (!h) {
    err = ERR_BUSY;
    HCDERR("failed to allocate hub device object");
    goto out_err;
  }

  h->d = dev;
  dev->class = &h->base;
  GET_DESC(&h->d->pipe0, HUB, 0, 0, &h->descriptor, sizeof(h->descriptor));

  err = usb_hub_get_status(h, &status);
  CHECK_ERR("failed to read hub port status. Enumeration will not continue");
  err = usb_hub_power_on_ports(h);
  CHECK_ERR("failed to power on hub ports");

out_err:
  if (err != ERR_OK && h) {
    dev->class = NULL;
    usb_hcd_hub_destroy(h);
  }

  HCDDEBUG("=============================================================");
  HCDDEBUG("=================== ENUMERATE HUB END =======================");
  HCDDEBUG("=============================================================");
  return err;
}

void usb_hcd_hub_init()
{
  STATIC_SLOT_INIT_FREE(usb_hcd_device_class_hub);
}
