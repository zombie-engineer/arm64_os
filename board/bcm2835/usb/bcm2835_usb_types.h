#pragma once
#include <types.h>
#include <memory/static_slot.h>
#include <compiler.h>
#include "bcm2835_usb_contants.h"
#include <usb/usb.h>

struct usb_hcd_pipe {
  int max_packet_size;
  int speed;
  int endpoint;
  int address;
  int ls_node_port;
  int ls_node_point;
};

struct usb_hcd_pipe_control {
  int transfer_type;
  int channel;
  int direction;
};
