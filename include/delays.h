#pragma once
#include <types.h>
#include <time.h>

void wait_cycles(uint32_t n);

/* wait n microseconds */
void wait_usec(uint32_t n);

/* wait n milliseconds */
void wait_msec(uint32_t n);

uint64_t get_system_timer();
