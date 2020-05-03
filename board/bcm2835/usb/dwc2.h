#pragma once
#include <compiler.h>
#include <types.h>


typedef struct dwc2_pipe_desc {
  union {
    struct {
      uint64_t device_address  :  7;
      uint64_t ep_address      :  4;
      uint64_t ep_type         :  2;
      uint64_t ep_direction    :  1;
      uint64_t low_speed       :  1;
      uint64_t max_packet_size : 11;
      uint64_t dwc_channel     :  3;
    };
    uint64_t raw;
  } u;
} dwc2_pipe_desc_t;

/*
 * Return value: length of a printed string
 */
int dwc2_pipe_desc_to_string(dwc2_pipe_desc_t desc, char *buf, int bufsz);

int dwc2_transfer(dwc2_pipe_desc_t pipe, void *buf, int bufsz, int pid, int *out_num_bytes);

int dwc2_init_channels();
