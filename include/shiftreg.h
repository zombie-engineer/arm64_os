#pragma once
#include <types.h>

int shiftreg_init(uint32_t ser, uint32_t srclk, uint32_t rclk);

int shiftreg_push_bit(char bit);

int shiftreg_push_byte(char byte);

int shiftreg_pulse_rclk();
