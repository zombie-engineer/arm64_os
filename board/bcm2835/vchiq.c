#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <error.h>
#include "vchiq/vchiq_arm.h"
#include "bcm2835-camera/bcm2835-camera.h"

int vchiq_init(void)
{
  return vchiq_platform_init();
}
