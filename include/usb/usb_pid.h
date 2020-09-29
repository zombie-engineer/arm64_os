#pragma once

typedef enum {
  USB_PID_DATA0 = 0,
  USB_PID_DATA1 = 1,
  USB_PID_DATA2 = 2,
  USB_PID_MDATA = 3,
  USB_PID_SETUP = 4,
  USB_PID_UNSET = 0xff
} usb_pid_t;

static inline const char *usb_pid_t_to_string(usb_pid_t p)
{
  switch(p) {
    case USB_PID_DATA0: return "DATA0";
    case USB_PID_DATA1: return "DATA1";
    case USB_PID_DATA2: return "DATA2";
    case USB_PID_MDATA: return "MDATA";
    case USB_PID_SETUP: return "SETUP";
    case USB_PID_UNSET: return "UNSET";
  }
  return "USB_PID_UNKNOWN";
}
