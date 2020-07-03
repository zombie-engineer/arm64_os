#include "dwc2_xfer_control.h"
#include <common.h>
#include "dwc2_log.h"
#include "dwc2_channel.h"
#include <usb/usb.h>
#include <stringlib.h>

DECL_STATIC_SLOT(struct dwc2_xfer_control, dwc2_xfer_control, 16);

struct dwc2_xfer_control *dwc2_xfer_control_create()
{
  struct dwc2_xfer_control *ctl;
  ctl = dwc2_xfer_control_alloc();
  DWCDEBUG2("dwc2_xfer_control allocated:%p", ctl);
  return ctl;
}

void dwc2_xfer_control_destroy(struct dwc2_xfer_control *ctl)
{
  dwc2_xfer_control_release(ctl);
  DWCDEBUG2("dwc2_xfer_control freed:%p", ctl);
}

void dwc2_xfer_control_init(void)
{
  STATIC_SLOT_INIT_FREE(dwc2_xfer_control);
}

