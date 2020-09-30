#pragma once
#include <spi.h>
#include <font.h>

#define TFTLCD_PIXEL_SIZE_X    320
#define TFTLCD_PIXEL_SIZE_Y    240

/*
 * tft_lcd_init - initialize tft lcd display
 * spidev         - SPI device object that will provide data transfer via SPI protocol MOSI, SCLK and CS0 pins
 * rst_pin        - GPIO pin number for RST role
 * dc_pin         - GPIO pin number for D/C role
 * function_flags - TFTLCD_FUNCF_* flags that describe mode of operation
 * display_mode   - one of TFTLCD_DSIPLAY_MODE_* values that will be set during initialization
 * return value   - 0 - success, non-zero - some error
 */
//int tft_lcd_init(spi_dev_t *spidev, uint32_t rst_pin, uint32_t dc_pin);
void tft_lcd_init(void);

/* Pulse RST pin to reset the device */
int tft_lcd_reset(void);

void tft_lcd_run(void);
