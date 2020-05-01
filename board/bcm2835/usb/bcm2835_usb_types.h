#pragma once
#include <types.h>
#include <memory/static_slot.h>
#include <compiler.h>
#include "bcm2835_usb_contants.h"
#include <usb/usb.h>

struct bcm2835_usb_device;
struct bcm2835_usb_hub_device;

struct bcm2835_usb_parent {
  uint8_t number;
  uint8_t port_number;
  uint16_t reserved;
};

struct bcm2835_usb_config_control {
  uint8_t index;
  uint8_t string_index;
  uint8_t status;
  uint8_t reserved;
};

struct bcm2835_usb_pipe {
  int max_packet_size;
  int speed;
  int endpoint;
  int number;
  int ls_node_port;
  int ls_node_point;
};

struct bcm2835_usb_pipe_control {
  int transfer_type;
  int channel;
  int direction;
};

struct bcm2835_usb_device {
  struct list_head STATIC_SLOT_OBJ_FIELD(bcm2835_usb_device);
  struct bcm2835_usb_parent parent_hub;
  struct bcm2835_usb_pipe pipe0;
  struct bcm2835_usb_pipe_control pipe0_control;
  struct bcm2835_usb_config_control config;
  uint8_t max_interface;

  struct usb_interface_descriptor ifaces[USB_MAX_INTERFACES_PER_DEVICE] ALIGNED(4);
  struct usb_endpoint_descriptor eps[USB_MAX_INTERFACES_PER_DEVICE][USB_MAX_ENDPOINTS_PER_DEVICE] ALIGNED(4);
  struct usb_device_descriptor descriptor ALIGNED(4);
  uint8_t payload_type;
  
  union {
    struct bcm2835_usb_hub_device *hub;
    struct usb_hid_device *hid;
    struct mass_storage_device *mass;
  } payload;
};

struct bcm2835_usb_hub_device {
  struct list_head STATIC_SLOT_OBJ_FIELD(bcm2835_usb_hub_device);
	int num_children;
	struct bcm2835_usb_device *children[USB_MAX_CHILDREN_PER_DEVICE];
	struct usb_hub_descriptor descriptor;
};

struct bcm2835_usb_channel {
  uint32_t chr; 
  uint32_t splt;
  uint32_t intr;
  uint32_t intrmsk;
  uint32_t xfer;
  uint32_t dma;
};

struct bcm2835_usb_xfer_ctrl {
  int split_tries;
  int split_max_retries;
};

struct xfer_control {
  int max_pack_sz;
  int pid;
  int ep;
  int ep_dir;
  int ep_type;
  int low_speed;
  int dev_addr;
  int hub_addr;
  int port_addr;
  const void *src;
  int src_sz;
};

