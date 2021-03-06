#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <error.h>
#include "vchiq/vchiq_arm.h"
#include "bcm2835-camera/bcm2835-camera.h"

VCHIQ_STATE_T vchiq_state;

int vchiq_init(void)
{
  int err;
 // struct bm2835_mmal_dev cam_dev;

  err = vchiq_platform_init(&vchiq_state);
  if (err != ERR_OK)
    return err;

 // err = bcm2835_mmal_probe(&cam_dev);
  return err;
}
