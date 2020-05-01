#pragma once
#include <types.h>
#include <compiler.h>

#define USB_RQ_TYPE_DEV_CLEAR_FEATURE   0b10000000
#define USB_RQ_TYPE_IFACE_CLEAR_FEATURE 0b10000001
#define USB_RQ_TYPE_EP_CLEAR_FEATURE    0b10000010
#define USB_RQ_TYPE_GET_CONFIGURATION   0b10000000
#define USB_RQ_TYPE_GET_DESCRIPTOR      0b10000000
#define USB_RQ_TYPE_GET_INTERFACE       0b10000001
#define USB_RQ_TYPE_GET_STATUS_DEV      0b10000000
#define USB_RQ_TYPE_GET_STATUS_IF       0b10000001
#define USB_RQ_TYPE_GET_STATUS_EP       0b10000010
#define USB_RQ_TYPE_GET_STATUS_OTHER    0b10000011
#define USB_RQ_TYPE_SET_ADDRESS         0b00000000
#define USB_RQ_TYPE_SET_CONFIGURATION   0b00000000
#define USB_RQ_TYPE_SET_DESCRIPTOR      0b00000000
#define USB_RQ_TYPE_DEV_SET_FEATURE     0b00000000
#define USB_RQ_TYPE_IFACE_SET_FEATURE   0b00000001
#define USB_RQ_TYPE_EP_SET_FEATURE      0b00000010
#define USB_RQ_TYPE_SET_INTERFACE       0b00000001
#define USB_RQ_TYPE_SYNCH_FRAME         0b00000010

#define USB_RQ_HUB_TYPE_CLEAR_HUB_FEATURE  0b00100000
#define USB_RQ_HUB_TYPE_CLEAR_PORT_FEATURE 0b00100011
#define USB_RQ_HUB_TYPE_CLEAR_TT_BUFFER    0b00100011
#define USB_RQ_HUB_TYPE_GET_HUB_DESCRIPTOR 0b10100000
#define USB_RQ_HUB_TYPE_GET_HUB_STATUS     0b10100000
#define USB_RQ_HUB_TYPE_GET_PORT_STATUS    0b10100011
#define USB_RQ_HUB_TYPE_RESET_TT           0b00100011
#define USB_RQ_HUB_TYPE_SET_HUB_DESCRIPTOR 0b00100000
#define USB_RQ_HUB_TYPE_SET_HUB_FEATURE    0b00100000
#define USB_RQ_HUB_TYPE_SET_PORT_FEATURE   0b00100011
#define USB_RQ_HUB_TYPE_GET_TT_STATE       0b10100011
#define USB_RQ_HUB_TYPE_STOP_TT            0b00100011

#define USB_MAX_HID_PER_DEVICE 4
#define USB_MAX_CHILDREN_PER_DEVICE 10
#define USB_LANG_ID_EN_US 0x409

/* PID type TOKEN */
#define USB_PID_TYPE_OUT   0b0001
#define USB_PID_TYPE_IN    0b1001
#define USB_PID_TYPE_SOF   0b0101
#define USB_PID_TYPE_SETUP 0b1101
/* PID type DATA */
#define USB_PID_TYPE_DATA0 0b0011
#define USB_PID_TYPE_DATA1 0b1011
#define USB_PID_TYPE_DATA2 0b0111
#define USB_PID_TYPE_MDATA 0b1111
/* PID type HANDSHAKE */
#define USB_PID_TYPE_ACK   0b0010
#define USB_PID_TYPE_NAK   0b1010
#define USB_PID_TYPE_STALL 0b1110
#define USB_PID_TYPE_NYET  0b0110
/* PID type SPECIAL */
#define USB_PID_TYPE_PRE   0b1100
#define USB_PID_TYPE_ERR   0b1100
#define USB_PID_TYPE_SPLIT 0b1000
#define USB_PID_TYPE_PING  0b0100

#define USB_PACKET_SIZE_8  0
#define USB_PACKET_SIZE_16 1
#define USB_PACKET_SIZE_32 2
#define USB_PACKET_SIZE_64 3

#define USB_HUB_FEATURE_CONNECTION         0
#define USB_HUB_FEATURE_ENABLE             1
#define USB_HUB_FEATURE_SUSPEND            2
#define USB_HUB_FEATURE_OVERCURRENT        3
#define USB_HUB_FEATURE_RESET              4
#define USB_HUB_FEATURE_PORT_POWER         8
#define USB_HUB_FEATURE_LOWSPEED           9
#define USB_HUB_FEATURE_HIGHSPEED          10
#define USB_HUB_FEATURE_CONNECTION_CHANGE  16
#define USB_HUB_FEATURE_ENABLE_CHANGE      17
#define USB_HUB_FEATURE_SUSPEND_CHANGE     18
#define USB_HUB_FEATURE_OVERCURRENT_CHANGE 19
#define USB_HUB_FEATURE_RESET_CHANGE       20

#define USB_DEVICE_CLASS_IN_IFACE        0x00
#define USB_DEVICE_CLASS_COMMUNICATIONS  0x02
#define USB_DEVICE_CLASS_HUB             0x09
#define USB_DEVICE_CLASS_DIAGNOSTIC      0xdc
#define USB_DEVICE_CLASS_MISC            0xef
#define USB_DEVICE_CLASS_VENDOR_SPEC     0xff

#define USB_DESCRIPTOR_TYPE_DEVICE            1
#define USB_DESCRIPTOR_TYPE_CONFIGURATION     2
#define USB_DESCRIPTOR_TYPE_STRING            3
#define USB_DESCRIPTOR_TYPE_INTERFACE         4
#define USB_DESCRIPTOR_TYPE_ENDPOINT          5
#define USB_DESCRIPTOR_TYPE_QUALIFIER         6
#define USB_DESCRIPTOR_TYPE_OTHERSPEED_CONFIG 7
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER   8
#define USB_DESCRIPTOR_TYPE_HID               33
#define USB_DESCRIPTOR_TYPE_HID_REPORT        34
#define USB_DESCRIPTOR_TYPE_HID_PHYSICAL      35
#define USB_DESCRIPTOR_TYPE_HUB               41

#define USB_RQ_GET_STATUS        0
#define USB_RQ_CLEAR_FEATURE     1
#define USB_RQ_SET_FEATURE       3
#define USB_RQ_SET_ADDRESS       5
#define USB_RQ_GET_DESCRIPTOR    6
#define USB_RQ_SET_DESCRIPTOR    7
#define USB_RQ_GET_CONFIGURATION 8
#define USB_RQ_SET_CONFIGURATION 9
#define USB_RQ_GET_INTERFACE     10
#define USB_RQ_SET_INTERFACE     11
#define USB_RQ_SYNCH_FRAME       12

#define USB_HID_REQUEST_TYPE_GET_REPORT   1
#define USB_HID_REQUEST_TYPE_GET_IDLE     2
#define USB_HID_REQUEST_TYPE_GET_PROTOCOL 3
#define USB_HID_REQUEST_TYPE_SET_REPORT   9
#define USB_HID_REQUEST_TYPE_SET_IDLE     10
#define USB_HID_REQUEST_TYPE_SET_PROTOCOL 11

#define USB_RQ_SETUP_DATA_DIR_HOST2DEVICE 0
#define USB_RQ_SETUP_DATA_DIR_DEVICE2HOST 1

#define USB_RQ_SETUP_TYPE_STANDARD 0
#define USB_RQ_SETUP_TYPE_CLASS    1
#define USB_RQ_SETUP_TYPE_VENDOR   2
#define USB_RQ_SETUP_TYPE_RESERVED 3

#define USB_RQ_SETUP_RECIPIENT_DEVICE    0
#define USB_RQ_SETUP_RECIPIENT_INTERFACE 1
#define USB_RQ_SETUP_RECIPIENT_ENDPOINT  2
#define USB_RQ_SETUP_RECIPIENT_OTHER     3

#define USB_SETUP_PACKET_TYPE(dir, type, recipient)\
  ((USB_RQ_SETUP_DATA_DIR_## dir&1)<<7)|\
  ((USB_RQ_SETUP_TYPE_ ## type&3)<<5)|\
  ((USB_RQ_SETUP_RECIPIENT_ ## recipient&0x1f))

#define USB_SETUP_PACKET_VALUE(desc_type, desc_idx)\
  ((USB_DESCRIPTOR_TYPE_## desc_type & 0xff)<<8)|(desc_idx & 0xff)

#define USB_SETUP_PACKET_VALUE_GET_TYPE(val)\
  ((val >> 8) & 0xff)

#define USB_SETUP_PACKET_VALUE_GET_IDX(val)\
  (val & 0xff)

#define USB_HUB_STATUS_CONNECTED_CHANGED   0
#define USB_HUB_STATUS_ENABLED_CHANGED     1
#define USB_HUB_STATUS_SUSPENDED_CHANGED   2
#define USB_HUB_STATUS_OVERCURRENT_CHANGED 3
#define USB_HUB_STATUS_RESET_CHANGED       4

#define USB_CFG_MAKE_ATTR(remote_wakeup, self_powered) ((remote_wakeup << 5)|(self_powered << 6))

#define USB_HUB_ATTR_POWER_SW_MODE_GANGED     0b00
#define USB_HUB_ATTR_POWER_SW_MODE_INDIVIDUAL 0b01

#define USB_HUB_ATTR_NOT_COMPOUND             0
#define USB_HUB_ATTR_IS_COMPOUND              1

#define USB_HUB_ATTR_OVER_CURRENT_PROT_GLOBAL     0b00
#define USB_HUB_ATTR_OVER_CURRENT_PROT_INDIVIDUAL 0b01
#define USB_HUB_ATTR_OVER_CURRENT_PROT_NONE       0b10

#define USB_HUB_ATTR_THINKTIME_00 0b00
#define USB_HUB_ATTR_THINKTIME_01 0b01
#define USB_HUB_ATTR_THINKTIME_10 0b10
#define USB_HUB_ATTR_THINKTIME_11 0b11

#define USB_HUB_ATTR_PORT_INDICATOR_NO  0b0
#define USB_HUB_ATTR_PORT_INDICATOR_YES 0b1

#define USB_HUB_MAKE_ATTR(power_sw_mode, is_compound, overcurrent_prot, thinktime, port_indicator)\
  ((power_sw_mode&3)|((is_compound&1)<<2)|((overcurrent_prot&3)<<3)|((thinktime&3)<<5)|((port_indicator&1)<<7))

#define USB_HUB_PORT_PWD_MASK_DEFAULT 0x1b
  
#define USB_PORT_STATUS_CH_BIT_CONNECTED_CHANGED   0
#define USB_PORT_STATUS_CH_BIT_ENABLED_CHANGED     1
#define USB_PORT_STATUS_CH_BIT_SUSPENDED_CHANGED   2
#define USB_PORT_STATUS_CH_BIT_OVERCURRENT_CHANGED 3
#define USB_PORT_STATUS_CH_BIT_RESET_CHANGED       4

#define USB_PORT_STATUS_BIT_CONNECTED   0
#define USB_PORT_STATUS_BIT_ENABLED     1
#define USB_PORT_STATUS_BIT_SUSPENDED   2
#define USB_PORT_STATUS_BIT_OVERCURRENT 3
#define USB_PORT_STATUS_BIT_RESET       4
#define USB_PORT_STATUS_BIT_POWER       8
#define USB_PORT_STATUS_BIT_LOWSPEED    9
#define USB_PORT_STATUS_BIT_HIGHSPEED  10
#define USB_PORT_STATUS_BIT_TESTMODE   11
#define USB_PORT_STATUS_BIT_INDICATOR  12

#define USB_HUB_MAKEPORT_STATUS(con, en, sus, ovrc, rst, pwr, ls, hs, tst, ind)\
  (((con  & 1) << USB_PORT_STATUS_BIT_CONNECTED)\
  |((en   & 1) << USB_PORT_STATUS_BIT_ENABLED)\
  |((sus  & 1) << USB_PORT_STATUS_BIT_SUSPENDED)\
  |((ovrc & 1) << USB_PORT_STATUS_BIT_OVERCURRENT)\
  |((rst  & 1) << USB_PORT_STATUS_BIT_RESET)\
  |((pwr  & 1) << USB_PORT_STATUS_BIT_POWER)\
  |((ls   & 1) << USB_PORT_STATUS_BIT_LOWSPEED)\
  |((hs   & 1) << USB_PORT_STATUS_BIT_HIGHSPEED)\
  |((tst  & 1) << USB_PORT_STATUS_BIT_TESTMODE)\
  |((ind  & 1) << USB_PORT_STATUS_BIT_INDICATOR))
  
struct usb_hub_port_full_status {
  uint16_t status;
  uint16_t change;
} PACKED;

struct usb_descriptor_header {
  uint8_t  length;
  uint8_t  descriptor_type;
} PACKED;

struct usb_device_descriptor {
  uint8_t  length;
  uint8_t  descriptor_type;
  uint16_t bcd_usb;
  uint8_t  device_class;
  uint8_t  device_subclass;
  uint8_t  device_protocol;
  uint8_t  max_packet_size_0;
  uint16_t id_vendor;
  uint16_t id_product;
  uint16_t bcd_device;
  uint8_t  i_manufacturer;
  uint8_t  i_product;
  uint8_t  i_serial_number;
  uint8_t  num_configurations;
} PACKED;

struct usb_configuration_descriptor {
  struct usb_descriptor_header header;
  uint16_t total_length;
  uint8_t  num_interfaces;
  uint8_t  configuration_value;
  uint8_t  iconfiguration;
  uint8_t  attributes;
  uint8_t  max_power;
} PACKED;

struct usb_other_speed_configuration_descriptor {
  struct usb_descriptor_header header;
  uint16_t total_length;
  uint8_t  num_interfaces;
  uint8_t  configuration_value;
  uint8_t  string_index;
  uint8_t  attributes;
  uint8_t  max_power;
} PACKED;

#define USB_INTERFACE_CLASS_RESERVED             0x00
#define USB_INTERFACE_CLASS_AUDIO                0x01
#define USB_INTERFACE_CLASS_COMMUNICATIONS       0x02
#define USB_INTERFACE_CLASS_HID                  0x03
#define USB_INTERFACE_CLASS_PHYSICAL             0x05
#define USB_INTERFACE_CLASS_IMAGE                0x06
#define USB_INTERFACE_CLASS_PRINTER              0x07
#define USB_INTERFACE_CLASS_MASSSTORAGE          0x08
#define USB_INTERFACE_CLASS_HUB                  0x09
#define USB_INTERFACE_CLASS_CDCDATA              0x0a
#define USB_INTERFACE_CLASS_SMARTCARD            0x0b
#define USB_INTERFACE_CLASS_CONTENTSECURITY      0x0d
#define USB_INTERFACE_CLASS_VIDEO                0x0e
#define USB_INTERFACE_CLASS_PERSONALHEALTHCARE   0x0f
#define USB_INTERFACE_CLASS_AUDIOVIDEO           0x10
#define USB_INTERFACE_CLASS_DIAGNOSTICDEVICE     0xdc
#define USB_INTERFACE_CLASS_WIRELESSCONTROLLER   0xe0
#define USB_INTERFACE_CLASS_MISCELLANEOUS        0xef
#define USB_INTERFACE_CLASS_APPLICATIONSPECIFIC  0xfe
#define USB_INTERFACE_CLASS_VENDORSPECIFIC       0xff

#define USB_SPEED_HIGH 0
#define USB_SPEED_FULL 1
#define USB_SPEED_LOW  2

#define USB_EP_TRANSFER_TYPE_CONTROL     0
#define USB_EP_TRANSFER_TYPE_ISOCHRONOUS 1
#define USB_EP_TRANSFER_TYPE_BULK        2
#define USB_EP_TRANSFER_TYPE_INTERRUPT   3

#define USB_EP_USAGE_TYPE_DATA       0b00
#define USB_EP_USAGE_TYPE_FEEDBACK   0b01
#define USB_EP_USAGE_TYPE_XDFEEDBACK 0b10
#define USB_EP_USAGE_TYPE_RESERVED   0b11
#define USB_EP_USAGE_TYPE_IGNORE     0b00

#define USB_EP_SYNC_TYPE_NONE        0b00
#define USB_EP_SYNC_TYPE_ASYNC       0b01
#define USB_EP_SYNC_TYPE_ADAPTIVE    0b10
#define USB_EP_SYNC_TYPE_SYNC        0b11
#define USB_EP_SYNC_TYPE_IGNORE      0b00

#define USB_EP_MAKE_ADDR(num, dir) ((num & 0xf) | (USB_DIRECTION_ ## dir << 7))
#define USB_EP_MAKE_ATTR(xfer_type, sync_type, use_type)\
  ((USB_EP_TRANSFER_TYPE_ ## xfer_type & 3)|((USB_EP_SYNC_TYPE_ ## sync_type & 3)<<2)|((USB_EP_USAGE_TYPE_ ## use_type & 3)<<4))

#define USB_DIRECTION_OUT 0
#define USB_DIRECTION_IN  1


#define USB_HUB_STATUS_LOCAL_POWER_GOOD 0
#define USB_HUB_STATUS_LOCAL_POWER_LOST 1

#define USB_HUB_STATUS_NO_OVERCURRENT 0
#define USB_HUB_STATUS_OVERCURRENT    1

#define USB_HUB_MAKE_STATUS(local_power_source, overcurrent) ((local_power_source&1)|((overcurrent&1)<1))

struct usb_hid_descriptor {
	struct usb_descriptor_header header;
  uint16_t hid_version;
  uint8_t  country_code;
	uint8_t  descriptor_count;
  uint8_t  type;
  uint16_t length;
} PACKED;

struct usb_interface_descriptor {
  struct usb_descriptor_header header;
  uint8_t number;
  uint8_t alt_setting;
  uint8_t endpoint_count;
  uint8_t class;
  uint8_t subclass;
  uint8_t protocol;
  uint8_t string_index;
} PACKED;

struct usb_endpoint_descriptor {
  struct usb_descriptor_header header;
  uint8_t  endpoint_address;
  uint8_t  attributes;
  uint16_t max_packet_size;
  uint8_t  interval;
} PACKED;

struct usb_string_descriptor {
  struct usb_descriptor_header header;
  uint16_t data[];
} PACKED;

struct usb_hub_descriptor {
  struct usb_descriptor_header header;
  uint8_t  port_count;
  union {
    struct {
      uint8_t hi;
      uint8_t lo;
    };
    uint16_t raw16;
  } attributes;
  uint8_t  power_good_delay;
  uint8_t  maximum_hub_power;
  uint8_t  device_removable;
  uint8_t  port_power_ctrl_mask;
} PACKED;

struct usb_device_rq{
  uint8_t  type;
  uint8_t  request;
  uint16_t value;
  uint16_t index;
  uint16_t length;
} PACKED;

struct usb_hid_device {
	struct usb_hid_descriptor descriptor[USB_MAX_HID_PER_DEVICE];
	uint8_t hid_interface[USB_MAX_HID_PER_DEVICE];
	uint8_t max_hid;
} PACKED;

static inline int size_to_usb_packet_size(uint32_t size)
{
  if (size <= 8)
    return USB_PACKET_SIZE_8;
  if (size <= 16)
    return USB_PACKET_SIZE_16;
  if (size <= 32)
    return USB_PACKET_SIZE_32;
  return USB_PACKET_SIZE_64;
}
int UsbInitialise();

