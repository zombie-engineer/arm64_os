#pragma once

#include <shiftreg.h>
#include <types.h>

int f5161ah_init(shiftreg_t *sr);

#define CHAR_NULL 0xff

int f5161ah_display_char(uint8_t ch, uint8_t dot);
