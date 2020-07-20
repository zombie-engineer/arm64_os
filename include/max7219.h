#pragma once
#include <types.h>
#include <spi.h>


// Set spi device
int max7219_set_spi_dev(spi_dev_t *spi_dev);


// Shutdown mode.
// At shutdown mode, max7219 does not cycle through registers and
// does not output any values to pins. Disable shutdown mode
// to see output from the chip.
// at power on max7219 is set in shutdown mode.
int max7219_set_shutdown_mode_on();
int max7219_set_shutdown_mode_off();


// Display test mode.
// At display test mode the chip outputs 1 to all pins resulting
// in all LEDS light up.
int max7219_set_test_mode_on();
int max7219_set_test_mode_off();


// Intensity.
// set intensity, controlled by on-chip PWM module,
// valid values are from 0 to 0xf
int max7219_set_intensity(uint8_t value);


// Decode mode.
// Setting decode mode to non-zero value allows to
// interpret register value as BCD endoded symbol

// No decode for digits 0-7
#define MAX7219_DATA_DECODE_MODE_NO   0x0
// Code B for digit 0, no decode for digits 1-7
#define MAX7219_DATA_DECODE_MODE_B0   0x1
// Code B for digits 0-3, no decode for digits 4-7
#define MAX7219_DATA_DECODE_MODE_B30  0xf
// Code B for digits 0-7
#define MAX7219_DATA_DECODE_MODE_B70  0xff

int max7219_set_decode_mode(uint8_t value);


#define MAX7219_SCAN_LIMIT_FULL 7
// Scan limit.
// Number of digits/lines that would be output during redraw phase.
// Valid values from 0 to 7
int max7219_set_scan_limit(uint8_t value);


// Set value for a specific digit / line
// use this to fill line/digit-related register with each bit
// interpreted as raw 0/1 on/off value or full value interpreted as
// as BCD encoding.
int max7219_set_digit(int digit_idx, uint8_t value);


// Set raw value to address and data registers of the chip.
int max7219_set_raw(uint16_t value);

int max7219_row_on(int row_index);
int max7219_row_off(int row_index);

int max7219_column_on(int column_index);
int max7219_column_off(int column_index);

// Print some debug information
int max7219_print_info();
