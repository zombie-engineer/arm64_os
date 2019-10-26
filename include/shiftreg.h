#pragma once
#include <types.h>

#define SHIFTREG_INIT_PIN_DISABLED -1

typedef struct shiftreg {
  int32_t ser;
  int32_t srclk;
  int32_t rclk;
  int32_t srclr;
  int32_t ce; 
  int delay_ms;
} shiftreg_t;

shiftreg_t *shiftreg_init(int clk_wait, int32_t ser, int32_t srclk, int32_t rclk, int32_t srclr, int32_t ce);

int shiftreg_push_bit(shiftreg_t *sr, char bit);

int shiftreg_push_byte(shiftreg_t *sr, char byte);

int shiftreg_pulse_rclk(shiftreg_t *sr);
