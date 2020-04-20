#include <board/bcm2835/bcm2835_usb.h>
#include "board_map.h"
#include "bcm2835_usb_registers.h"
#include <reg_access.h>
#include <common.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>

#define USB_CORE_CTRL (reg32_t)(USB_BASE + 0x0c)

// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/Arm32_64_USB/rpi-usb.h
int bcm2835_usb_power_on()
{
  int err;
  uint32_t exists = 0, power_on = 1;
  err = mbox_set_power_state(MBOX_DEVICE_ID_USB, &power_on, 1, &exists);
  if (err) {
    printf("bcm2835_usb_power_on:mbox call failed:%d\r\n", err);
    return ERR_GENERIC;
  }
  if (!exists) {
    puts("bcm2835_usb_power_on:after mbox call:device does not exist\r\n");
    return ERR_GENERIC;
  }
  if (!power_on) {
    puts("bcm2835_usb_power_on:after mbox call:device still not powered on\r\n");
    return ERR_GENERIC;
  }
  puts("bcm2835_usb_power_on:device powered on\r\n");
  return ERR_OK;
}

int bcm2835_usb_power_off()
{
  int err;
  uint32_t exists = 0, power_on = 0;
  err = mbox_set_power_state(MBOX_DEVICE_ID_USB, &power_on, 1, &exists);
  if (err) {
    printf("bcm2835_usb_power_off:mbox call failed:%d\r\n", err);
    return ERR_GENERIC;
  }
  if (!exists) {
    puts("bcm2835_usb_power_off:after mbox call:device does not exist\r\n");
    return ERR_GENERIC;
  }
  if (power_on) {
    puts("bcm2835_usb_power_off:after mbox call:device still powered on\r\n");
    return ERR_GENERIC;
  }
  puts("bcm2835_usb_power_off:device powered off\r\n");
  return ERR_OK;
}

int bcm2835_usb_init()
{
  int err;
  uint32_t core, vendor_id, user_id;

  vendor_id = read_reg(USB_GSNPSID);
  user_id = read_reg(USB_GUID);
  core = read_reg(USB_GUSBCFG);
  printf("bcm2835_usb_init:vendor_id:%08x,user_id:%08x\r\n", vendor_id, user_id);
  printf("bcm2835_usb_init:core_ctrl:%08x\r\n", core);
  err = bcm2835_usb_power_on();
  return err;
}
