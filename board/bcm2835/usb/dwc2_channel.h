#pragma once
#include <compiler.h>
#include <usb/usb_pid.h>
#include <usb/usb.h>
#include <bug.h>
#include <common.h>
#include <drivers/usb/hcd.h>

struct dwc2_channel {
  /*
   * id - channel number. dwc2 has multiple channels:
   * HAINT/HAINTMSK registers show 1 bit per channel information on
   * which channel has fired an interrupt. HAINT = 0x00000003 - bits 0 and 1
   * are set => channels 0 and 1 have interrupts.
   *
   * registers USB_HCCHAR0 (+0x500) to USB_HCDMA0 (+0x514) are channel 0
   * controls that explicitly regulate channel 0,
   * registers USB_HCCHAR1 (+0x520) to USB_HCDMA1 (+0x534) are channel 1
   * controls that explicitly regulate channel 1, etc.
   *
   * Number of channels is read from USB_GHWCFG2 (+0x48) bits [14:17],
   * right now most of the code uses hardcoded value of 6 channels
   *
   */
#define DWC2_INVALID_CHANNEL -1
  int id;

  /*
   * pipe - pipe, used for programming channel control registers.
   */
  struct usb_hcd_pipe *pipe;

  /*
   * ctl - DWC2 specific control, that stores transfer state like number of bytes
   * left to transfer, dma address and some options like split.
   */
  struct dwc2_xfer_control *ctl;
};

static inline struct usb_hcd_pipe *dwc2_channel_get_pipe_safe(struct dwc2_channel *c)
{
  BUG(c->pipe == NULL, "dwc2_channel_get_pipe: pipe is NULL");
  return c->pipe;
}

static inline int dwc2_channel_get_max_packet_size(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->max_packet_size;
}

static inline int dwc2_channel_get_direction(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->endpoint_dir;
}

static inline int dwc2_channel_get_ep_num(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->endpoint_num;
}

static inline int dwc2_channel_get_ep_type(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->endpoint_type;
}

static inline int dwc2_channel_get_device_address(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->device_address;
}

static inline bool dwc2_channel_is_speed_high(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->device_speed == USB_SPEED_HIGH;
}

static inline bool dwc2_channel_is_speed_low(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->device_speed == USB_SPEED_LOW;
}

static inline bool dwc2_channel_get_pipe_speed(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->device_speed;
}

static inline usb_pid_t dwc2_channel_get_next_pid(struct dwc2_channel *c)
{
  return dwc2_channel_get_pipe_safe(c)->next_pid;
}

static inline void dwc2_channel_set_next_pid(struct dwc2_channel *c, usb_pid_t next_pid)
{
  dwc2_channel_get_pipe_safe(c)->next_pid = next_pid;
}

static inline bool dwc2_channel_split_mode(struct dwc2_channel *c)
{
  return !dwc2_channel_is_speed_high(c);
}

struct dwc2_channel *dwc2_channel_create();

void dwc2_channel_destroy(struct dwc2_channel *c);

struct dwc2_channel *dwc2_channel_get_by_id(int ch_id);

void dwc2_channel_bind_xfer_control(struct dwc2_channel *c, struct dwc2_xfer_control *ctl);

void dwc2_channel_unbind_xfer_control(struct dwc2_channel *c);

void dwc2_channel_init(void);
