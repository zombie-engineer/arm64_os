#include "root_hub.h"
#include <common.h>
#include <stringlib.h>
#include <reg_access.h>
#include <bits_api.h>
#include <delays.h>
#include "usb_dev_rq.h"
#include "dwc2_regs.h"
#include "dwc2_regs_bits.h"

#define USB_ROOT_HUB_STRING_IDX_PRODUCT      1
#define USB_ROOT_HUB_STRING_IDX_MANUFACTURER 2
#define USB_ROOT_HUB_STRING_IDX_SERIAL       3
#define USB_ROOT_HUB_STRING_PRODUCT          u"root_hub"
#define USB_ROOT_HUB_STRING_MANUFACTURER     u"broadcom"
#define USB_ROOT_HUB_STRING_SERIAL           u"seri"
#define USB_SPEC_2_0                         0x200
#define USB_DEV_RELEASE_NUM                  0x100

#define RHLOG(fmt, ...)\
  printf("[RH INFO]: "fmt "\r\n", ## __VA_ARGS__)

#define RHERR(fmt, ...)\
  printf("[RH ERR]: "fmt "\r\n", ## __VA_ARGS__)

#define RHWARN(fmt, ...)\
  printf("[RH WARN]: "fmt "\r\n", ## __VA_ARGS__)

int usb_root_hub_device_number = 0;

static ALIGNED(4) struct usb_hub_descriptor usb_root_hub_descriptor = {
	.header = {
	  .length = sizeof(usb_root_hub_descriptor),
    .descriptor_type = USB_DESCRIPTOR_TYPE_HUB,
  },
	.port_count = 1,
  .attributes = { 
    .raw16 = USB_HUB_MAKE_ATTR(
      USB_HUB_ATTR_POWER_SW_MODE_GANGED,
      USB_HUB_ATTR_NOT_COMPOUND,
      USB_HUB_ATTR_OVER_CURRENT_PROT_GLOBAL,
      USB_HUB_ATTR_THINKTIME_00,
      USB_HUB_ATTR_PORT_INDICATOR_NO), 
    },
	.power_good_delay = 0,
  .maximum_hub_power = 0,
	.device_removable = (1<<1), /* Port 1 is non-removale */
	.port_power_ctrl_mask = USB_HUB_PORT_PWD_MASK_DEFAULT
};

static ALIGNED(4) struct root_hub_configuration usb_root_hub_configuration = {
	.cfg = {
		.header.length = sizeof(struct usb_configuration_descriptor),
		.header.descriptor_type = USB_DESCRIPTOR_TYPE_CONFIGURATION,
		.total_length = sizeof(struct root_hub_configuration),
		.num_interfaces = 1,
		.configuration_value = 1,
		.iconfiguration = 2,
		.attributes = USB_CFG_MAKE_ATTR(0, 1),
	},
	.iface = {
		.header = {
			.length = sizeof(struct usb_interface_descriptor),
			.descriptor_type = USB_DESCRIPTOR_TYPE_INTERFACE,
		},
		.number = 0,
		.alt_setting = 0,
		.endpoint_count = 1,
		.class = USB_IFACE_CLASS_HUB,
		.subclass = 0,
		.protocol = 0,
		.string_index = 0,
	},
	.ep = {
		.header = {
			.length = sizeof(struct usb_endpoint_descriptor),
			.descriptor_type = USB_DESCRIPTOR_TYPE_ENDPOINT,
		},
		.endpoint_address = USB_EP_MAKE_ADDR(1, IN),
		.attributes = USB_EP_MAKE_ATTR(INTERRUPT, IGNORE, IGNORE),
		.max_packet_size = 64,
		.interval = 0xff,
	},
};

static ALIGNED(4) struct usb_device_descriptor usb_root_hub_device_descriptor = {
	.length = sizeof(struct usb_device_descriptor),
	.descriptor_type = USB_DESCRIPTOR_TYPE_DEVICE,
	.bcd_usb = USB_SPEC_2_0,
	.device_class = USB_DEVICE_CLASS_HUB,
	.device_subclass = 0,
	.device_protocol = 0,
	.max_packet_size_0 = 64,
	.id_vendor = 0,
	.id_product = 0,
	.bcd_device = USB_DEV_RELEASE_NUM,
	.i_manufacturer = USB_ROOT_HUB_STRING_IDX_MANUFACTURER,
	.i_product = USB_ROOT_HUB_STRING_IDX_PRODUCT,
	.i_serial_number = USB_ROOT_HUB_STRING_IDX_SERIAL,
	.num_configurations = 1,
};

static struct usb_root_hub_string_descriptor0 usb_rh_string0 ALIGNED(4) = {
  .h = {
    .length = sizeof(usb_rh_string0),
    .descriptor_type = USB_DESCRIPTOR_TYPE_STRING
  },
  .lang_id = USB_LANG_ID_EN_US
};

struct usb_string_descriptor usb_rh_string_product = {
	.header = {
		.length = sizeof(USB_ROOT_HUB_STRING_PRODUCT) + 2,
		.descriptor_type = USB_DESCRIPTOR_TYPE_STRING,
	},
	.data = USB_ROOT_HUB_STRING_PRODUCT
};

struct usb_string_descriptor usb_rh_string_manufact = {
	.header = {
		.length = sizeof(USB_ROOT_HUB_STRING_PRODUCT) + 2,
		.descriptor_type = USB_DESCRIPTOR_TYPE_STRING,
	},
	.data = USB_ROOT_HUB_STRING_MANUFACTURER
};

struct usb_string_descriptor usb_rh_string_serial = {
	.header = {
		.length = sizeof(USB_ROOT_HUB_STRING_PRODUCT) + 2,
		.descriptor_type = USB_DESCRIPTOR_TYPE_STRING,
	},
	.data = USB_ROOT_HUB_STRING_SERIAL
};

static inline void usb_root_hub_reply(void *dst, int dst_sz, void *reply, int reply_sz, int *out_num_bytes)
{
  if (reply_sz) {
    if (reply_sz > dst_sz) {
      RHWARN("truncating reply_size from %d to %d", reply_sz, dst_sz);
      reply_sz = dst_sz;
    }
    memcpy(dst, reply, reply_sz);
  }
  *out_num_bytes = reply_sz;
}

static int usb_rh_get_descriptor(uint64_t rq, void *buf, int buf_sz, int *out_num_bytes)
{
  int err = ERR_OK;
  int reply_sz = 0;
  void *reply = NULL;
  int rq_value = USB_DEV_RQ_GET_VALUE(rq);
  int desc_type = (rq_value >> 8) & 0xff;
  int desc_idx = rq_value & 0xff;

  switch(desc_type) {
    case USB_DESCRIPTOR_TYPE_HUB:
      reply_sz = sizeof(usb_root_hub_descriptor);
      reply = &usb_root_hub_descriptor;
      break;
    case USB_DESCRIPTOR_TYPE_STRING:
      RHLOG("GET_STRING:%d", desc_idx);
      switch (desc_idx) {
        /* string index 0 returns supported lang ids */
        case 0:
          reply_sz = sizeof(usb_rh_string0);
          reply = &usb_rh_string0;
          break;
        case USB_ROOT_HUB_STRING_IDX_PRODUCT:
          reply_sz = usb_rh_string_product.header.length;
          reply = &usb_rh_string_product;
          break;
        case USB_ROOT_HUB_STRING_IDX_MANUFACTURER:
          reply_sz = usb_rh_string_manufact.header.length;
          reply = &usb_rh_string_manufact;
          break;
        case USB_ROOT_HUB_STRING_IDX_SERIAL:
          reply_sz = usb_rh_string_serial.header.length;
          reply = &usb_rh_string_serial;
          break;
        default:
          RHERR("requested string %d does not exit", desc_idx);
          err = ERR_INVAL_ARG;
          break;
      } 
      break;
    case USB_DESCRIPTOR_TYPE_DEVICE:
      reply_sz = sizeof(struct usb_device_descriptor);
      reply = &usb_root_hub_device_descriptor;
      break;
    case USB_DESCRIPTOR_TYPE_CONFIGURATION:
      reply_sz = sizeof(usb_root_hub_configuration);
      reply = &usb_root_hub_configuration;
      break;
    default:
      err = ERR_INVAL_ARG;
      RHERR("unknown descriptor type requested: value: %02x", rq_value);
    break;
  }
  usb_root_hub_reply(buf, buf_sz, reply, reply_sz, out_num_bytes);
  return err;
}

static int hub_get_port_status(struct usb_hub_port_status *s)
{
  uint32_t r = read_reg(USB_HPRT);
  RHLOG("port_status:");
  print_usb_hprt(r);
  puts("\r\n");

  s->status = USB_HUB_MAKEPORT_STATUS(
      USB_HPRT_GET_CONN_STS(r), 
      USB_HPRT_GET_ENA(r), 
      USB_HPRT_GET_SUSP(r), 
      USB_HPRT_GET_OVR_CURR_ACT(r), 
      USB_HPRT_GET_RST(r), 
      USB_HPRT_GET_PWR(r), 
      (USB_HPRT_GET_SPD(r) == USB_SPEED_LOW),
      (USB_HPRT_GET_SPD(r) == USB_SPEED_HIGH),
      USB_HPRT_GET_TST_CTL(r), 
      0
  );
  s->changed = USB_HUB_MAKEPORT_STATUS(
      USB_HPRT_GET_CONN_DET(r), 
      0, //USB_HPRT_GET_EN_CHNG(r), 
      0,
      USB_HPRT_GET_OVR_CURR_CHNG(r), 
      0, 0, 0, 0, 0, 0);
  return ERR_OK;
}

static int usb_rh_get_status(uint64_t rq, void *buf, int buf_sz, int *out_num_bytes)
{
  int err = ERR_OK;
  uint32_t status;
  int reply_sz = 0;
  int type = USB_DEV_RQ_GET_TYPE(rq);
  int index = USB_DEV_RQ_GET_INDEX(rq);
  struct usb_hub_port_status port_status ALIGNED(4);
  void *reply = NULL;

  switch (type) {
    case USB_RQ_HUB_TYPE_GET_HUB_STATUS:
      status = USB_HUB_MAKE_STATUS(USB_HUB_STATUS_LOCAL_POWER_LOST, USB_HUB_STATUS_NO_OVERCURRENT);
      reply = &status;
      reply_sz = sizeof(status);
      break;
    case USB_RQ_HUB_TYPE_GET_PORT_STATUS:
      if (index == 1) {
        err = hub_get_port_status(&port_status);
        if (err == ERR_OK) {
          reply = &port_status;
          reply_sz = sizeof(port_status);
        }
      } else
        RHERR("non existing port index %d requested for status", index);
      break;
    default:
      RHERR("unknown get status request type: %x", type);
      err = ERR_INVAL_ARG;
      break;
  }
  usb_root_hub_reply(buf, buf_sz, reply, reply_sz, out_num_bytes);
  return err;
}

static inline int usb_rh_rq_set_addr(uint64_t rq, void *buf, int buf_sz, int *out_num_bytes)
{
	usb_root_hub_device_number = USB_DEV_RQ_GET_VALUE(rq);
  return ERR_OK;
}

static int usb_rh_rq_set_feature(uint64_t rq, void *buf, int buf_sz, int *out_num_bytes)
{
  int err = ERR_OK;
  int type = USB_DEV_RQ_GET_TYPE(rq);
  int value = USB_DEV_RQ_GET_VALUE(rq);
  int r;
  RHLOG("type:%08x,val:%08x", type, value);
  switch (type) {
    case USB_RQ_HUB_TYPE_SET_HUB_FEATURE:
      /* Skipping */
      break;
    case USB_RQ_HUB_TYPE_SET_PORT_FEATURE:
      switch (value) {
        case USB_HUB_FEATURE_PORT_POWER:
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_SET_U32(r, USB_HPRT_PWR);
          write_reg(USB_HPRT, r);
          break;
        case USB_HUB_FEATURE_RESET:
          r = read_reg(USB_PCGCR);
          RHLOG("USB_PCGCR:%08x\r\n", r);
          USB_PCGCR_CLR_EN_SLP_CLK_GATE(r);
          USB_PCGCR_CLR_STOP_PCLK(r);
          write_reg(USB_PCGCR, r);
          wait_msec(10);
          RHLOG("r:%08x\n", r);
          write_reg(USB_PCGCR, 0);
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_SET_U32(r, USB_HPRT_RST);
          write_reg(USB_HPRT, r);
          RHLOG("USB_PCGCR:%08x\r\n", r);
          break;
        default:
          RHWARN("Unsupported request value: %04x\n", value);
          break;
      }
      break;
    default:
      RHWARN("Unsupported feature type: %02x\n", type);
      break;
  }
  return err;
}

static int usb_rh_rq_clear_feature(uint64_t rq, void *buf, int buf_sz, int *out_num_bytes)
{
  int err = ERR_OK;
  int type = USB_DEV_RQ_GET_TYPE(rq);
  int value = USB_DEV_RQ_GET_VALUE(rq);
  uint32_t r;
  switch (type) {
    case USB_RQ_HUB_TYPE_SET_HUB_FEATURE:
      /* Skipping */
      break;
    case USB_RQ_HUB_TYPE_SET_PORT_FEATURE:
      switch (value) {
        case USB_HUB_FEATURE_PORT_POWER:
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_CLEAR_U32(r, USB_HPRT_PWR);
          write_reg(USB_HPRT, r);
          break;
        case USB_HUB_FEATURE_ENABLE:
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_SET_U32(r, USB_HPRT_ENA);
          write_reg(USB_HPRT, r);
          break;
        case USB_HUB_FEATURE_ENABLE_CHANGE:
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_SET_U32(r, USB_HPRT_EN_CHNG);
          write_reg(USB_HPRT, r);
          break;
        case USB_HUB_FEATURE_RESET_CHANGE:
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          RHLOG("USB_HUB_FEATURE_RESET_CHANGE:%08x", r);
          break;
        case USB_HUB_FEATURE_SUSPEND_CHANGE:
          /* power and clock register to 0 */
          write_reg(USB_PCGCR, 0);
					wait_msec(5);
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_SET_U32(r, USB_HPRT_RES);
          write_reg(USB_HPRT, r);
					wait_msec(100);
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_CLEAR_U32(r, USB_HPRT_SUSP);
          BIT_CLEAR_U32(r, USB_HPRT_RES);
          write_reg(USB_HPRT, r);
          break;
        case USB_HUB_FEATURE_OVERCURRENT_CHANGE:
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_SET_U32(r, USB_HPRT_OVR_CURR_CHNG);
          write_reg(USB_HPRT, r);
          break;
        case USB_HUB_FEATURE_CONNECTION_CHANGE:
          r = read_reg(USB_HPRT) & USB_HPRT_BIT_CLEAR;
          BIT_SET_U32(r, USB_HPRT_CONN_DET);
          write_reg(USB_HPRT, r);
          RHLOG("USB_HUB_FEATURE_CONNECTION_CHANGE: %08x\r\n", r);
          break;
        default:
          RHWARN("Unsupported request value: %04x\n", value);
          break;
      }
      break;
    default:
      RHWARN("Unsupported feature type: %02x\n", type);
      break;
  }
  return err;
}

int usb_root_hub_process_req(uint64_t rq, void *buf, int buf_sz, int *out_num_bytes)
{
  int err;
  char rq_desc[256];
  int request = USB_DEV_RQ_GET_RQ(rq);
  usb_rq_get_description(rq, rq_desc, sizeof(rq_desc));
  RHLOG("%s", rq_desc);

  switch (request)
  {
    case USB_RQ_GET_STATUS:
      err = usb_rh_get_status(rq, buf, buf_sz, out_num_bytes);
      break;
    case USB_RQ_SET_FEATURE:
      err = usb_rh_rq_set_feature(rq, buf, buf_sz, out_num_bytes);
      break;
    case USB_RQ_CLEAR_FEATURE:
      err = usb_rh_rq_clear_feature(rq, buf, buf_sz, out_num_bytes);
      break;
    case USB_RQ_SET_ADDRESS:
      err = usb_rh_rq_set_addr(rq, buf, buf_sz, out_num_bytes);
      break;
    case USB_RQ_GET_DESCRIPTOR:
      err = usb_rh_get_descriptor(rq, buf, buf_sz, out_num_bytes);
      break;
    case USB_RQ_SET_CONFIGURATION:
      /* ignore it */
      err = ERR_OK;
      break;
    case USB_RQ_SET_DESCRIPTOR:
      err = ERR_NOT_IMPLEMENTED;
      break;
    case USB_RQ_GET_CONFIGURATION:
      err = ERR_NOT_IMPLEMENTED;
      break;
    case USB_RQ_GET_INTERFACE:
      err = ERR_NOT_IMPLEMENTED;
      break;
    case USB_RQ_SET_INTERFACE:
      err = ERR_NOT_IMPLEMENTED;
      break;
    case USB_RQ_SYNCH_FRAME:
      err = ERR_NOT_IMPLEMENTED;
      break;
    default:
      err = ERR_INVAL_ARG;
      RHERR("unknown usb rq: %d", request);
      break;
  }
  RHLOG("root hub control message processed with status: %d, bytes:%d", err, *out_num_bytes);
  return err;
}
