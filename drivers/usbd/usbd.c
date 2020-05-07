#include <drivers/usb/usbd.h>
#include <drivers/usb/hcd.h>
#include <error.h>
#include <common.h>

int usbd_init()
{
  int err;
  err = usb_hcd_init();
  if (err != ERR_OK) {
    printf("Failed to init USB HCD, err: %d\r\n", err);
    goto out_err;
  }
//  err = usb_hcd_start();
//  if (err != ERR_OK) {
//    printf("Failed to start USB HCD, err: %d\r\n", err);
//    goto out_err;
//  }
out_err:
  return err;
}

void usbd_print_device_tree()
{
}
