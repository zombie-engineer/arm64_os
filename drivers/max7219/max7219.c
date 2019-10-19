#include <max7219.h>
#include <gpio.h>
#include <common.h>


#define SHIFT_REG_CLOCK_PIN 17
#define SHIFT_REG_DATA_PIN 2
#define SHIFT_REG_LATCH_PIN 4


static spi_dev_t *spidev = 0;

static int max7219_verbose_output = 0;

#define DECL_CHECKED_FUNC(fn, ...) \
int fn(__VA_ARGS__) \
{ \
  if (!spidev) { \
    puts(#fn " error: max7219 driver not initialized\n"); \
    return -1; \
  } 

#define MAX7219_ADDR_NOOP         0x0000
#define MAX7219_ADDR_DIGIT_0      0x0100
#define MAX7219_ADDR_DIGIT_1      0x0200
#define MAX7219_ADDR_DIGIT_2      0x0300
#define MAX7219_ADDR_DIGIT_3      0x0400
#define MAX7219_ADDR_DIGIT_4      0x0500
#define MAX7219_ADDR_DIGIT_5      0x0600
#define MAX7219_ADDR_DIGIT_6      0x0700
#define MAX7219_ADDR_DIGIT_7      0x0800
#define MAX7219_ADDR_DECODE_MODE  0x0900
#define MAX7219_ADDR_INTENSITY    0x0a00
#define MAX7219_ADDR_SCAN_LIMIT   0x0b00
#define MAX7219_ADDR_SHUTDOWN     0x0c00
#define MAX7219_ADDR_DISPLAY_TEST 0x0f00

#define MAX7219_DATA_INTENSITY_MASK 0x000f

#define MAX7219_DATA_SHUTDOWN_ON  0
#define MAX7219_DATA_SHUTDOWN_OFF 1

#define MAX7219_DATA_DISPLAY_TEST_ON  1
#define MAX7219_DATA_DISPLAY_TEST_OFF 0


static int max7219_store(uint16_t value)
{
  int st;
  if (max7219_verbose_output)
    printf("shiftpush 16bits: %04x\n", value);
  int i;
  for (i = 15; i >= 0; --i) {
    st = spidev->push_bit((value >> i) & 1);
    if (st) {
      printf("spidev->push_bit error: %d\n", st);
      return st;
    }
  }

  if (max7219_verbose_output)
    puts("\n");

  st = spidev->ce0_set();
  if (st) {
    printf("spidev->ce0_set error: %d\n", st);
    return st;
  }

  st = spidev->ce0_clear();
  if (st) {
    printf("spidev->ce0_clear error: %d\n", st);
    return st;
  }
  return 0;
}


int max7219_set_spi_dev(spi_dev_t *spi_dev)
{
  if (!spi_dev) {
    puts("max7219_set_spi_dev error: spi_dev is NULL\n");
    return -1;
  }

  spidev = spi_dev;
  return 0;
}


DECL_CHECKED_FUNC(max7219_set_shutdown_mode_on)
  return max7219_store(MAX7219_ADDR_SHUTDOWN | MAX7219_DATA_SHUTDOWN_ON);
}


DECL_CHECKED_FUNC(max7219_set_shutdown_mode_off)
  return max7219_store(MAX7219_ADDR_SHUTDOWN | MAX7219_DATA_SHUTDOWN_OFF);
}


DECL_CHECKED_FUNC(max7219_set_test_mode_on)
  return max7219_store(MAX7219_ADDR_DISPLAY_TEST | MAX7219_DATA_DISPLAY_TEST_ON);
}


DECL_CHECKED_FUNC(max7219_set_test_mode_off)
  return max7219_store(MAX7219_ADDR_DISPLAY_TEST | MAX7219_DATA_DISPLAY_TEST_OFF);
}


DECL_CHECKED_FUNC(max7219_set_intensity, uint8_t value)
  if (value > 0xf)
    return -1;
  return max7219_store(MAX7219_ADDR_INTENSITY | (value & MAX7219_DATA_INTENSITY_MASK));
}


DECL_CHECKED_FUNC(max7219_set_decode_mode, uint8_t value)
  if  (value != MAX7219_DATA_DECODE_MODE_NO 
    && value != MAX7219_DATA_DECODE_MODE_B0
    && value != MAX7219_DATA_DECODE_MODE_B30 
    && value != MAX7219_DATA_DECODE_MODE_B70)
    return -1;

  return max7219_store(MAX7219_ADDR_DECODE_MODE | value);
}


DECL_CHECKED_FUNC(max7219_set_scan_limit, uint8_t value)
  if (value > 7)
    return -1;

  return max7219_store(MAX7219_ADDR_SCAN_LIMIT | value);
}


DECL_CHECKED_FUNC(max7219_set_digit, int digit_idx, uint8_t value)
  if (digit_idx > 7)
    return -1;

  return max7219_store((digit_idx + 1) << 8 | value);
}


DECL_CHECKED_FUNC(max7219_set_raw, uint16_t value)
  max7219_store(value);
  return 0;
}
