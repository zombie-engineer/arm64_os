#pragma once
#include <memory/static_slot.h>
#include <completion.h>
#include <types.h>

#define DWC2_TRANSFER_STATUS_STARTED 0
#define DWC2_TRANSFER_STATUS_SPLIT_HANDLED 1

struct dwc2_channel;

struct dwc2_xfer_control {
  struct list_head STATIC_SLOT_OBJ_FIELD(dwc2_xfer_control);
  struct dwc2_channel *c;
  int status;
  int direction;
  int transfer_size;
  int split_start;
  uint64_t dma_addr_base;
  uint64_t dma_addr;
  completion_fn completion;
  void *completion_arg;
};

struct dwc2_xfer_control *dwc2_xfer_control_create();
void dwc2_xfer_control_destroy(struct dwc2_xfer_control *);
void dwc2_xfer_control_init(void);
