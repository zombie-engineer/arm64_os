#pragma once
#include <drivers/usb/hcd.h>

int usb_hcd_submit_cm(
  struct usb_hcd_pipe *pipe,
  struct usb_hcd_pipe_control *pctl,
  void *buf,
  int buf_sz,
  uint64_t rq,
  int timeout,
  int *out_num_bytes);
