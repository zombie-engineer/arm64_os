#pragma once

extern volatile unsigned int mbox[36];

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

#define MBOX_TAG_GET_FIRMWARE_REV      0x00001
#define MBOX_TAG_GET_BOARD_MODEL       0x10001
#define MBOX_TAG_GET_BOARD_REV         0x10002
#define MBOX_TAG_GET_MAC_ADDR          0x10003
#define MBOX_TAG_GET_BOARD_SERIAL      0x10004
#define MBOX_TAG_GET_ARM_MEMORY        0x10005
#define MBOX_TAG_GET_VC_MEMORY         0x10006


#define MBOX_TAG_SET_POWER             0x28001
#define MBOX_TAG_SET_CLOCK_RATE        0x38002
#define MBOX_TAG_GET_PHYS_WIDTH_HEIGHT 0x40003
#define MBOX_TAG_SET_PHYS_WIDTH_HEIGHT 0x48003
#define MBOX_TAG_GET_VIRT_WIDTH_HEIGHT 0x40004
#define MBOX_TAG_SET_VIRT_WIDTH_HEIGHT 0x48004
#define MBOX_TAG_LAST      0


int mbox_call(unsigned char ch);
