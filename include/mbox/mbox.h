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
#define MBOX_TAG_SET_POWER             0x28001
#define MBOX_TAG_GET_CLOCK_RATE        0x30002
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
#define MBOX_TAG_GET_COMMAND_LINE      0x50001


#define MBOX_TAG_LAST      0


int mbox_call(unsigned char ch);