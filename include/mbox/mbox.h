#pragma once
#include <types.h>

extern volatile uint32_t mbox_buffer[36];

#define MBOX_REQUEST 0

#define MBOX_CH_POWER 0
#define MBOX_CH_FB    1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS  4
#define MBOX_CH_BTNS  5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_COUNT 7
#define MBOX_CH_PROP  8

#define MBOX_CLOCK_ID_RESERVED 0
#define MBOX_CLOCK_ID_EMMC     1
#define MBOX_CLOCK_ID_UART     2
#define MBOX_CLOCK_ID_ARM      3
#define MBOX_CLOCK_ID_CORE     4
#define MBOX_CLOCK_ID_V3D      5
#define MBOX_CLOCK_ID_H264     6
#define MBOX_CLOCK_ID_ISP      7
#define MBOX_CLOCK_ID_SDRAM    8
#define MBOX_CLOCK_ID_PIXEL    9
#define MBOX_CLOCK_ID_PWM      10
#define MBOX_CLOCK_ID_EMMC2    11


// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface

#define MBOX_TAG_GET_FIRMWARE_REV      0x00001
#define MBOX_TAG_GET_BOARD_MODEL       0x10001
#define MBOX_TAG_GET_BOARD_REV         0x10002
#define MBOX_TAG_GET_MAC_ADDR          0x10003
#define MBOX_TAG_GET_BOARD_SERIAL      0x10004
#define MBOX_TAG_GET_ARM_MEMORY        0x10005
#define MBOX_TAG_GET_VC_MEMORY         0x10006
#define MBOX_TAG_GET_CLOCKS            0x10007
#define MBOX_TAG_ALLOCATE_BUFFER       0x40001
#define MBOX_TAG_BLANK_SCREEN          0x40002
#define MBOX_TAG_GET_POWER_STATE       0x20001
#define MBOX_TAG_SET_POWER_STATE       0x28001
#define MBOX_TAG_GET_CLOCK_STATE       0x30001
#define MBOX_TAG_SET_CLOCK_STATE       0x38001
#define MBOX_TAG_GET_CLOCK_RATE        0x30002
#define MBOX_TAG_GET_MAX_CLOCK_RATE    0x30004
#define MBOX_TAG_GET_MIN_CLOCK_RATE    0x30007
#define MBOX_TAG_GET_TURBO             0x30009
#define MBOX_TAG_SET_TURBO             0x30009
#define MBOX_TAG_SET_CLOCK_RATE        0x38002
#define MBOX_TAG_GET_PHYS_WIDTH_HEIGHT 0x40003
#define MBOX_TAG_SET_PHYS_WIDTH_HEIGHT 0x48003
#define MBOX_TAG_GET_VIRT_WIDTH_HEIGHT 0x40004
#define MBOX_TAG_SET_VIRT_WIDTH_HEIGHT 0x48004
#define MBOX_TAG_GET_DEPTH             0x40005
#define MBOX_TAG_SET_DEPTH             0x48005
#define MBOX_TAG_GET_PIXEL_ORDER       0x40006
#define MBOX_TAG_SET_PIXEL_ORDER       0x48006
#define MBOX_TAG_GET_ALPHA_MODE        0x40007
#define MBOX_TAG_SET_ALPHA_MODE        0x48007
#define MBOX_TAG_GET_PITCH             0x40008
#define MBOX_TAG_GET_VIRT_OFFSET       0x40009
#define MBOX_TAG_SET_VIRT_OFFSET       0x48009
#define MBOX_TAG_SET_VCHIQ_INIT        0x48010
#define MBOX_TAG_GET_COMMAND_LINE      0x50001

#define MBOX_TAG_LAST      0

#define MBOX_DEVICE_ID_SD      0
#define MBOX_DEVICE_ID_UART0   1
#define MBOX_DEVICE_ID_UART1   2
#define MBOX_DEVICE_ID_USB     3
#define MBOX_DEVICE_ID_I2C0    4
#define MBOX_DEVICE_ID_I2C1    5
#define MBOX_DEVICE_ID_I2C2    6
#define MBOX_DEVICE_ID_SPI     7
#define MBOX_DEVICE_ID_CCP2TX  8

int mbox_prop_call_no_lock();
int mbox_prop_call();

int mbox_call_blocking(int mbox_channel);

void mbox_init(void);
