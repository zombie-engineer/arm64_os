#pragma once
#include <types.h>

typedef struct dwc2_pipe_desc {
  union {
    struct {
      uint64_t device_address  :  7;
      uint64_t ep_address      :  4;
      uint64_t ep_type         :  2;
      uint64_t speed           :  2;
      uint64_t max_packet_size : 11;
      uint64_t dwc_channel     :  3;
      uint64_t hub_address     :  7;
      uint64_t hub_port        :  7;
    };
    uint64_t raw;
  } u;
} dwc2_pipe_desc_t;

#define PIPE_DESC_INIT(__hcd_pipe) {\
    .u = {\
      .device_address  = __hcd_pipe->address, \
      .ep_address      = __hcd_pipe->ep,\
      .ep_type         = __hcd_pipe->ep_type,\
      .speed           = __hcd_pipe->speed,\
      .max_packet_size = __hcd_pipe->max_packet_size,\
      .hub_address     = __hcd_pipe->ls_hub_address,\
      .hub_port        = __hcd_pipe->ls_hub_port\
    }\
  }

#define DECL_PIPE_DESC(__name, __hcd_pipe) \
  dwc2_pipe_desc_t __name = PIPE_DESC_INIT(__hcd_pipe)
