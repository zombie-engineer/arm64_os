#pragma once
#include <spi.h>
#include <font.h>

#define NOKIA5110_PIXEL_SIZE_X    84 
#define NOKIA5110_PIXEL_SIZE_Y    48

#define NOKIA5110_ROW_MAX (NOKIA5110_PIXEL_SIZE_Y / 8 - 1)
#define NOKIA5110_COLUMN_MAX (NOKIA5110_PIXEL_SIZE_X - 1)

#define NOKIA5110_BIAS_MAX 7

#define NOKIA5110_DISPLAY_MODE_BLANK      0
#define NOKIA5110_DISPLAY_MODE_NORMAL     1
#define NOKIA5110_DISPLAY_MODE_ALL_SEG_ON 2
#define NOKIA5110_DISPLAY_MODE_INVERSE    3

// Extended instruction set
#define NOKIA5110_FUNCF_EXT_ISET    1

// Vertical addressing mode. Otherwise it's horizontal addressing mode
#define NOKIA5110_FUNCF_VADDRESSING 2

// Power down mode
#define NOKIA5110_FUNCF_PD_MODE     4

// initialize nokia5110 display
// spidev         - SPI device object that will provide data transfer via SPI protocol MOSI, SCLK and CS0 pins 
// rst_pin        - GPIO pin number for RST role
// dc_pin         - GPIO pin number for D/C role
// function_flags - NOKIA5110_FUNCF_* flags that describe mode of operation
// display_mode   - one of NOKIA5110_DSIPLAY_MODE_* values that will be set during initialization
// return value   - 0 - success, non-zero - some error
int nokia5110_init(spi_dev_t *spidev, uint32_t rst_pin, uint32_t dc_pin, int function_flags, int display_mode);

int nokia5110_set_function(int function_flags);

int nokia5110_set_display_mode(int display_mode);


#define NOKIA5110_TEMP_COEFF_0 0
#define NOKIA5110_TEMP_COEFF_1 1
#define NOKIA5110_TEMP_COEFF_2 2
#define NOKIA5110_TEMP_COEFF_3 3
#define NOKIA5110_TEMP_COEFF_MAX NOKIA5110_TEMP_COEFF_3
// Set's the temperature coefficient for a display, resulting in a lower/higher contrast level
int nokia5110_set_temperature_coeff(int coeff);

// Set LCD bias, valid range for bias value is [0-7]
int nokia5110_set_bias(int bias);

// Pulse RST pin to reset the device
int nokia5110_reset(void);

// Start drawing from this address in LCD RAM
int nokia5110_set_cursor(int x, int y);

// Draw a dot
int nokia5110_draw_dot(int x, int y);

// Draw a line 
int nokia5110_draw_line(int x0, int y0, int x1, int y1);

// Draw rectangle 
int nokia5110_draw_rect(int x, int y, int sx, int sy);

// Set current font
void nokia5110_set_font(const font_desc_t *f);

// Draw text using currently set font starting from 
// coordinates x,y
int nokia5110_draw_text(const char *text, int x, int y);

// Prints debug information about the display
void nokia5110_print_info();

int nokia5110_run_test_loop_1(int interations, int wait_interval);

int nokia5110_run_test_loop_2(int interations, int wait_interval);

int nokia5110_run_test_loop_3(int interations, int wait_interval);

