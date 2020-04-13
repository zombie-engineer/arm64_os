#pragma once
#include <types.h>

/* cm_set_clk - sets clock to desired clock source and divisor
 * clk_id     - clock to set, one of general purpose clocks or 
 *              pwm clock
 * clck_src   - source clock
 * mash       - noise-shaping MASH divider. Read "BCM2835 ARM Peripherals" manual.
 *            - use 0 as a value as default.
 * divi       - intergral part of the divisor
 * divf       - floating point part of the divisor
 */
#define CM_CLK_ID_GP_0        0
#define CM_CLK_ID_GP_1        1
#define CM_CLK_ID_GP_2        2
#define CM_CLK_ID_PWM         3
#define CM_CLK_ID_MAX         CM_CLK_ID_PWM


/* 0 Hz */
#define CM_SETCLK_SRC_GND     0

/* 19.2 MHz */
#define CM_SETCLK_SRC_OSC     1

/* 0 Hz */
#define CM_SETCLK_SRC_TSTDBG0 2

/* 0 Hz */
#define CM_SETCLK_SRC_TSTDBG1 3

/* 0 Hz */
#define CM_SETCLK_SRC_PLLA    4

/* 1000 MHz */
#define CM_SETCLK_SRC_PLLC    5

/* 500 MHz */
#define CM_SETCLK_SRC_PLLD    6

/* 216 MHz */
#define CM_SETCLK_SRC_HDMI    7


#define CM_SETCLK_MASH_OFF    0
#define CM_SETCLK_MASH_1STAGE 1
#define CM_SETCLK_MASH_2STAGE 2
#define CM_SETCLK_MASH_3STAGE 3


#define CM_SETCLK_ERR_OK      0
#define CM_SETCLK_ERR_INV    -1
#define CM_SETCLK_ERR_BUSY   -2

void cm_print_clock(int clock_id);
void cm_print_clocks(void);
int cm_set_clock(int clk_id, uint32_t clock_src, uint32_t mash, uint32_t divi, uint32_t divf);
int cm_get_clock(int clk_id);
const char *set_clock_err_to_str(int);
