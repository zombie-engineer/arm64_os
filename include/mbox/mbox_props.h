#pragma once
#include <types.h>

int mbox_get_firmware_rev();

int mbox_get_board_model();

int mbox_get_board_rev();

unsigned long mbox_get_board_serial();

int mbox_get_arm_memory(int *base_addr, int *byte_size);

int mbox_get_vc_memory(int *base_addr, int *byte_size);

int mbox_get_mac_addr(char *mac_start, char *mac_end);


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

int mbox_get_clock_state(uint32_t clock_id, uint32_t* enabled, uint32_t *exists);

int mbox_set_clock_state(uint32_t clock_id, uint32_t* enabled, uint32_t *exists);

int mbox_get_clock_rate(uint32_t clock_id, uint32_t *clock_rate);

int mbox_get_min_clock_rate(uint32_t clock_id, uint32_t *clock_rate);

int mbox_get_max_clock_rate(uint32_t clock_id, uint32_t *clock_rate);

int mbox_set_clock_rate(uint32_t clock_id, uint32_t *clock_rate, uint32_t skip_turbo);
