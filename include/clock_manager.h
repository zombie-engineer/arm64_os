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


#define CM_SETCLK_SRC_GND     0
#define CM_SETCLK_SRC_OSC     1
#define CM_SETCLK_SRC_TSTDBG0 2
#define CM_SETCLK_SRC_TSTDBG1 3
#define CM_SETCLK_SRC_PLLA    4
#define CM_SETCLK_SRC_PLLC    5
#define CM_SETCLK_SRC_PLLD    6
#define CM_SETCLK_SRC_HDMI    7


#define CM_SETCLK_MASH_OFF    0
#define CM_SETCLK_MASH_1STAGE 1
#define CM_SETCLK_MASH_2STAGE 2
#define CM_SETCLK_MASH_3STAGE 3


#define CM_SETCLK_ERR_OK      0
#define CM_SETCLK_ERR_INV    -1
#define CM_SETCLK_ERR_BUSY   -2


int cm_set_clock(int clk_id, uint32_t clock_src, uint32_t mash, uint32_t divi, uint32_t divf);
