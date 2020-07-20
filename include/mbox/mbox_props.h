#pragma once
#include <types.h>

int mbox_get_firmware_rev();

int mbox_get_board_model();

int mbox_get_board_rev();

unsigned long mbox_get_board_serial();

int mbox_get_arm_memory(int *base_addr, int *byte_size);

int mbox_get_vc_memory(int *base_addr, int *byte_size);

int mbox_get_mac_addr(char *mac_start, char *mac_end);

int mbox_set_power_state(uint32_t device_id, uint32_t *powered_on, uint32_t wait, uint32_t *exists);

int mbox_get_power_state(uint32_t device_id, uint32_t *powered_on, uint32_t *exists);

int mbox_get_clock_state(uint32_t clock_id, uint32_t* enabled, uint32_t *exists);

int mbox_set_clock_state(uint32_t clock_id, uint32_t* enabled, uint32_t *exists);

int mbox_get_clock_rate(uint32_t clock_id, uint32_t *clock_rate);

int mbox_get_min_clock_rate(uint32_t clock_id, uint32_t *clock_rate);

int mbox_get_max_clock_rate(uint32_t clock_id, uint32_t *clock_rate);

int mbox_set_clock_rate(uint32_t clock_id, uint32_t *clock_rate, uint32_t skip_turbo);

int mbox_get_virt_wh(uint32_t *out_width, uint32_t *out_height);

typedef struct mbox_set_fb_args {
  uint32_t psize_x;
  uint32_t psize_y;
  uint32_t vsize_x;
  uint32_t vsize_y;
  uint32_t voffset_x;
  uint32_t voffset_y;
  uint32_t depth;
  uint32_t pixel_order;
} mbox_set_fb_args_t;

typedef struct mbox_set_fb_res {
  uint32_t fb_addr;
  uint32_t fb_width;
  uint32_t fb_height;
  uint32_t fb_pitch;
  uint32_t fb_pixel_size;
} mbox_set_fb_res_t;

int mbox_set_fb(mbox_set_fb_args_t *args, mbox_set_fb_res_t *res);
