#include <drivers/usb/hcd_hub.h>
#include <delays.h>
#include <bits_api.h>
#include <usb/usb_timings.h>

DECL_STATIC_SLOT(struct usb_hcd_device_class_hub, usb_hcd_device_class_hub, 12)

/*
 * USB2.0 specification. 9.2.0.6.1 Request Processing Timing
 * Upper limit for any command to be processed is 5 seconds.
 */
#define REQUEST_PROCESS_TIMEOUT 5000

/*
 * On a timeouted reset we retry couple of times but give an interval of 500 milliseconds
 */
#define RESET_RETRY_INTERVAL 500

#define MAX_RESET_RETRIES 3

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

static inline usb_speed_t hub_port_status_to_speed(struct usb_hub_port_status* port_status)
{
  if (port_status->status.high_speed)
    return USB_SPEED_HIGH;
  if (port_status->status.low_speed)
  		return USB_SPEED_LOW;
	return USB_SPEED_FULL;
}

static inline void hub_port_status_updated(usb_hub_t *h, int port,
  struct usb_hcd_device *d, struct usb_hub_port_status* port_status)
{
  usb_speed_t speed = hub_port_status_to_speed(port_status);
  hcd_device_set_speed(d, speed);
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

static inline int usb_hub_port_clear_changed(usb_hub_t *h, int port,
  struct usb_hub_port_status *port_status)
{
  int err;
  if (port_status->change.connected_changed) {
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_CONNECTION_CHANGE);
    CHECK_ERR("Failed to clear feature CONNECTED_CHANGE");
  }
  if (port_status->change.enabled_changed) {
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_ENABLE_CHANGE);
    CHECK_ERR("Failed to clear feature ENABLE_CHANGE");
  }
  if (port_status->change.suspended_changed) {
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_SUSPEND_CHANGE);
    CHECK_ERR("Failed to clear feature SUSPEND_CHANGE");
  }
  if (port_status->change.overcurrent_changed) {
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_OVERCURRENT_CHANGE);
    CHECK_ERR("Failed to clear feature OVERCURRENT_CHANGE");
  }
  if (port_status->change.reset_changed) {
    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_RESET_CHANGE);
    CHECK_ERR("Failed to clear feature RESET_CHANGE");
  }
out_err:
  return err;
}


int usb_hub_read_port_status(usb_hub_t *h, int port, const char *tag, bool clear_change,
  struct usb_hub_port_status *port_status)
{
  int err;
  char buf[512];
  err = usb_hub_port_get_status(h, port, port_status);
  CHECK_ERR("GET_PORT_STATUS request failed");
  usb_status_to_string(port_status, buf, sizeof(buf));
  HUBPORTLOG("get_port_status(%s):err:%d,%s", tag, err, buf);
  if (clear_change) {
    err = usb_hub_port_clear_changed(h, port, port_status);
    CHECK_ERR("Failed to clear port status change");
  }
out_err:
  return err;
}

hub_port_reset_status_t usb_hub_port_reset(usb_hub_t *h, int port, struct usb_hcd_device *port_dev)
{
  /*
   * port reset procedure by USB 2.0 spec
   */
  int err = ERR_GENERIC;

	struct usb_hub_port_status port_status ALIGNED(4);
  uint64_t tnow;

  err = usb_hub_port_set_feature(h, port, USB_HUB_FEATURE_RESET);
  CHECK_ERR("Failed to initiate RESET state at hub port");

  tnow = get_boottime_msec();
  do {
    err = usb_hub_read_port_status(h, port, "after reset", 1, &port_status);
    CHECK_ERR("failed to read hub port status during reset completion awaiting");

    if (port_status.change.enabled_changed || port_status.status.reset || port_status.status.enabled) {
      if (port_status.status.reset) {
        err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_RESET);
        CHECK_ERR("Failed to clear RESET feature");
        err = usb_hub_read_port_status(h, port, "after reset", 1, &port_status);
        CHECK_ERR("failed to read hub port status during reset completion awaiting");
      }
      if (port_status.change.enabled_changed) {
        err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_ENABLE_CHANGE);
        CHECK_ERR("Failed to clear ENABLE_CHANGED feature");
      }
      if (port_dev)
        hub_port_status_updated(h, port, port_dev, &port_status);

      wait_msec(TDRST_MS);
      err = usb_hub_read_port_status(h, port, "after reset", 1, &port_status);
      CHECK_ERR("Failed to check port status after RESET");
      BUG(port_status.status.reset, "Failed to clear RESET status");
      return HUB_PORT_RESET_STATUS_SUCCESS;
    }
  } while(tnow + REQUEST_PROCESS_TIMEOUT < get_boottime_msec());
  return HUB_PORT_RESET_STATUS_TIMEOUT;

out_err:
  return HUB_PORT_RESET_STATUS_ERROR;
}

static int usb_hub_port_connected(usb_hub_t *h, int port)
{
  int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);
  struct usb_hcd_device *port_dev = NULL;

  err = usb_hub_port_get_status(h, port, &port_status);
  CHECK_ERR_SILENT();
  HUBPORTDBG("status: %04x:%04x", port_status.status, port_status.change);

  err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_CONNECTION_CHANGE);
  CHECK_ERR_SILENT();
  HUBPORTDBG("CONNECTION_CHANGE cleared");

  err = usb_hub_port_reset(h, port, port_dev);
  CHECK_ERR_SILENT();

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
  HUBPORTDBG("status: %04x:%04x", port_status.status, port_status.change);

	if (port_status.status.high_speed)
    port_dev->pipe0.device_speed = USB_SPEED_HIGH;
  else if (port_status.status.low_speed)
		port_dev->pipe0.device_speed = USB_SPEED_LOW;
	else
    port_dev->pipe0.device_speed = USB_SPEED_FULL;

  HUBPORTDBG("device speed: %s", usb_speed_to_string(port_dev->pipe0.device_speed));

  if (!port_status.status.high_speed) {
		port_dev->pipe0.ls_hub_address = h->d->address;
		port_dev->pipe0.ls_hub_port = port;
  }

//  err = usb_hcd_enumerate_device(port_dev);
//  if (err != ERR_OK) {
//    int saved_err = err;
//    HUBPORTERR("failed to enumerate device");
//    err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_ENABLE);
//    if (err != ERR_OK)
//      HUBPORTERR("failed to disable port %d", port);
//    err = saved_err;
//    goto out_err;
//  }

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

  err = usb_hub_port_clear_feature(h, port, USB_HUB_FEATURE_CONNECTION_CHANGE);
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

int usb_hub_port_check_connection(usb_hub_t *h, int port)
{
	int err = ERR_OK;
	struct usb_hub_port_status port_status ALIGNED(4);

  err = usb_hub_port_get_status(h, port, &port_status);
  CHECK_ERR("usb_hub_port_check_connection: failed to get status");

  HUBPORTDBG("status:%04x:%04x", port_status.status.raw, port_status.change.raw);
	if (port_status.change.connected_changed) {
    HUBPORTLOG("CONNECTED status changed to %s",
      port_status.status.connected ? "CONNECTED" :"DISCONNECTED");

    if (port_status.status.connected) {
  	  err = usb_hub_port_connected(h, port);
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
  int err, port;
  struct usb_hub_port_status port_status ALIGNED(4);

  for (port = 0; port < h->descriptor.port_count; ++port)
    err = usb_hub_read_port_status(h, port, tag, 0, &port_status);
  return err;
}

int usb_hub_port_ensure_powered(usb_hub_t *h, int port,
  struct usb_hcd_device *port_dev)
{
  int err;
  struct usb_hub_port_status port_status ALIGNED(4);
  uint64_t tnow;
  int delay_msec;

  delay_msec = h->descriptor.power_good_delay * 2;
  err = usb_hub_read_port_status(h, port, "check powered", 1, &port_status);
  CHECK_ERR("Failed to check if port is powered");
  if (!port_status.status.powered) {
    tnow = get_boottime_msec();
    err = usb_hub_port_power_on(h, port);
    CHECK_ERR("failed to power on, skipping");

    do {
      err = usb_hub_read_port_status(h, port, "powering", 1, &port_status);
      if (err)
        HUBPORTERR("Failed to read port status");
    } while(tnow + delay_msec > get_boottime_msec());
  }

  wait_msec(delay_msec);

  if (err == ERR_OK) {
    port_dev->state = USB_DEVICE_STATE_POWERED;
    hub_port_status_updated(h, port, port_dev, &port_status);
    HUBPORTLOG("device speed after power on: %s", usb_speed_to_string(port_dev->pipe0.device_speed));
  }

out_err:
  return err;
}

enum {
  ENUM_STATE_STABILIZATION_DEBOUNCE = 0,
  ENUM_STATE_FIRST_PORT_RESET,
  ENUM_STATE_FIRST_DESCRIPTOR_REQUEST,
  ENUM_STATE_SECOND_PORT_RESET,
  ENUM_STATE_SET_ADDRESS,
  ENUM_STATE_SECOND_DESCRIPTOR_REQUEST,
  ENUM_STATE_CONFIGURATION_DESCRIPTOR_REQUEST,
  ENUM_STATE_CONFIGURE_CLASS,
  ENUM_STATE_REPORT_TO_TREE,
  ENUM_STATE_COMPLETED
};

int usb_hub_enumerate_port(usb_hub_t *h, int port)
{
  int err = ERR_OK;
  int num_retries = 0;
  int enum_state;
  hub_port_reset_status_t rst_status;
  struct usb_hcd_device *port_dev = NULL;

  port_dev = usb_hcd_allocate_device();
  if (!port_dev) {
    HUBPORTERR("failed to allocate device");
    goto out_err;
  }
  port_dev->location.hub = h->d;
  port_dev->location.hub_port = port;

  enum_state = ENUM_STATE_STABILIZATION_DEBOUNCE;

  while(enum_state != ENUM_STATE_COMPLETED) {
    switch(enum_state) {
      case ENUM_STATE_STABILIZATION_DEBOUNCE:
        err = usb_hub_port_ensure_powered(h, port, port_dev);
        CHECK_ERR("Failed to enumerate port, failed to establish power status");
        enum_state = ENUM_STATE_FIRST_PORT_RESET;
        break;
      case ENUM_STATE_FIRST_PORT_RESET:
reset_retry:
        rst_status = usb_hub_port_reset(h, port, port_dev);
        if (rst_status == HUB_PORT_RESET_STATUS_ERROR) {
          err = ERR_GENERIC;
          HUBPORTERR("Failed to perform first reset");
          goto out_err;
        }
        if (rst_status == HUB_PORT_RESET_STATUS_TIMEOUT) {
          HUBPORTERR("Timeout at first reset");
          goto handle_reset_timeout;
        }
        enum_state = ENUM_STATE_FIRST_DESCRIPTOR_REQUEST;
        break;
      case ENUM_STATE_FIRST_DESCRIPTOR_REQUEST:
        err = usb_hcd_get_descriptor_first(port_dev);
        CHECK_ERR("Failed at first get descriptor");
        enum_state = ENUM_STATE_SECOND_PORT_RESET;
        break;
      case ENUM_STATE_SECOND_PORT_RESET:
        rst_status = usb_hub_port_reset(h, port, port_dev);
        if (rst_status == HUB_PORT_RESET_STATUS_ERROR) {
          err = ERR_GENERIC;
          HUBPORTERR("Failed to perform first reset");
          goto out_err;
        }
        if (rst_status == HUB_PORT_RESET_STATUS_TIMEOUT) {
          HUBPORTERR("Timeout at first reset");
          goto handle_reset_timeout;
        }
        enum_state = ENUM_STATE_SET_ADDRESS;
        break;
      case ENUM_STATE_SET_ADDRESS:
        err = usb_hcd_to_addressed_state(port_dev);
        CHECK_ERR("Failed to set address");
        enum_state = ENUM_STATE_SECOND_DESCRIPTOR_REQUEST;
        break;
      case ENUM_STATE_SECOND_DESCRIPTOR_REQUEST:
        err = usb_hcd_get_descriptor_second(port_dev);
        CHECK_ERR("Failed at first get descriptor");
        enum_state = ENUM_STATE_CONFIGURATION_DESCRIPTOR_REQUEST;
        break;
      case ENUM_STATE_CONFIGURATION_DESCRIPTOR_REQUEST:
        err = usb_hcd_to_configured_state(port_dev);
        CHECK_ERR("Failed set configuration");
        enum_state = ENUM_STATE_CONFIGURE_CLASS;
        break;
      case ENUM_STATE_CONFIGURE_CLASS:
        err = usb_hcd_configure_class(port_dev);
        CHECK_ERR("Failed to configure device class");
        enum_state = ENUM_STATE_REPORT_TO_TREE;
        break;
      case ENUM_STATE_REPORT_TO_TREE:
        list_add(&port_dev->hub_children, &h->children);
        HUBPORTDBG("hub: %p, adding device %p to port, h->children:%p", h, port_dev, &h->children);
        enum_state = ENUM_STATE_COMPLETED;
        break;
      default:
        HUBPORTLOG("Enumeration logic failed");
        err = ERR_GENERIC;
        goto out_err;
    }
  }

out_err:
  if (err != ERR_OK) {
    if (port_dev)
      usb_hcd_deallocate_device(port_dev);
  }
  return err;

handle_reset_timeout:
  if (num_retries++ == MAX_RESET_RETRIES) {
    HUBPORTERR("Failed to enumerate device at port due to timeout");
    err = ERR_TIMEOUT;
    goto out_err;
  }
  wait_msec(RESET_RETRY_INTERVAL);
  goto reset_retry;
}

#define MAX_PORTS 8

int usb_hub_enumerate_ports(usb_hub_t *h)
{
  int port, err;
  // uint32_t delay;
  err = ERR_OK;
  // volatile int do_retry = 0;
  // struct usb_hub_port_status port_status[MAX_PORTS] ALIGNED(4);
  struct usb_hub_status hub_status ALIGNED(4);

  err = usb_hub_get_status(h, &hub_status);
  HUBLOG("hub status: %08x", hub_status.u.raw32);

	// HUBDBG("powering on all %d ports", h->descriptor.port_count);
  for (port = 0; port < h->descriptor.port_count; ++port) {
    err = usb_hub_enumerate_port(h, port);
    if (err = ERR_TIMEOUT) {
      err = ERR_OK;
    }
  }
  return err;
}

int usb_hub_probe_ports(usb_hub_t *h)
{
  int port, err;

  for (port = 0; port < h->descriptor.port_count; ++port) {
    HUBPORTDBG("check connection");
    err = usb_hub_port_check_connection(h, port);
    if (err != ERR_OK) {
      HUBPORTERR("failed check connection, skipping");
      err = ERR_OK;
      continue;
    }
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
  CHECK_ERR("failed to read hub status. Enumeration will not continue");
  err = usb_hub_enumerate_ports(h);
  CHECK_ERR("failed to power on hub ports");
  // err = usb_hub_probe_ports(h);
 //  CHECK_ERR("failed to probe hub");

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
