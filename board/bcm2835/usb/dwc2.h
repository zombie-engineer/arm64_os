#pragma once
#include <compiler.h>
#include <types.h>
#include <usb/usb_pid.h>

typedef struct dwc2_pipe_desc {
  union {
    struct {
      uint64_t device_address  :  7;
      uint64_t ep_address      :  4;
      uint64_t ep_type         :  2;
      uint64_t ep_direction    :  1;
      uint64_t speed           :  2;
      uint64_t max_packet_size : 11;
      uint64_t dwc_channel     :  3;
      uint64_t hub_address     :  7;
      uint64_t hub_port        :  7;
    };
    uint64_t raw;
  } u;
} dwc2_pipe_desc_t;

/*
 * Return value: length of a printed string
 */
int dwc2_pipe_desc_to_string(dwc2_pipe_desc_t desc, char *buf, int bufsz);

typedef enum {
  DWC2_STATUS_ACK = 0,
  DWC2_STATUS_NAK,
  DWC2_STATUS_NYET,
  DWC2_STATUS_STALL,
  DWC2_STATUS_TIMEOUT,
  DWC2_STATUS_ERR,
} dwc2_transfer_status_t;

dwc2_transfer_status_t dwc2_transfer(dwc2_pipe_desc_t pipe, void *buf, int bufsz, usb_pid_t *pid, int *out_num_bytes);

int dwc2_init_channels();

int dwc2_set_log_level(int log_level);
int dwc2_get_log_level();
