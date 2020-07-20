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

#define MAX7219_ADDR_NOOP         0x00
#define MAX7219_ADDR_DIGIT_0      0x01
#define MAX7219_ADDR_DIGIT_1      0x02
#define MAX7219_ADDR_DIGIT_2      0x03
#define MAX7219_ADDR_DIGIT_3      0x04
#define MAX7219_ADDR_DIGIT_4      0x05
#define MAX7219_ADDR_DIGIT_5      0x06
#define MAX7219_ADDR_DIGIT_6      0x07
#define MAX7219_ADDR_DIGIT_7      0x08
#define MAX7219_ADDR_DECODE_MODE  0x09
#define MAX7219_ADDR_INTENSITY    0x0a
#define MAX7219_ADDR_SCAN_LIMIT   0x0b
#define MAX7219_ADDR_SHUTDOWN     0x0c
#define MAX7219_ADDR_DISPLAY_TEST 0x0f

#define MAX7219_DATA_INTENSITY_MASK 0x0f

#define MAX7219_DATA_SHUTDOWN_ON  0
#define MAX7219_DATA_SHUTDOWN_OFF 1

#define MAX7219_DATA_DISPLAY_TEST_ON  1
#define MAX7219_DATA_DISPLAY_TEST_OFF 0


static int max7219_store(uint8_t addr, uint8_t data)
{
  char bytes[2];
  if (max7219_verbose_output)
    printf("max7219_store : %02x %02x\n", addr, data);

  bytes[0] = addr;
  bytes[1] = data;
  return spidev->xmit(spidev, bytes, 0, 2);
}

int max7219_print_info()
{
  puts(  "max7219 debug info: \n");
  printf(" verbose_output: %d\n", max7219_verbose_output);
  printf(" spidev:         %p\n", spidev);
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
  return max7219_store(MAX7219_ADDR_SHUTDOWN, MAX7219_DATA_SHUTDOWN_ON);
}


DECL_CHECKED_FUNC(max7219_set_shutdown_mode_off)
  return max7219_store(MAX7219_ADDR_SHUTDOWN, MAX7219_DATA_SHUTDOWN_OFF);
}


DECL_CHECKED_FUNC(max7219_set_test_mode_on)
  return max7219_store(MAX7219_ADDR_DISPLAY_TEST, MAX7219_DATA_DISPLAY_TEST_ON);
}


DECL_CHECKED_FUNC(max7219_set_test_mode_off)
  return max7219_store(MAX7219_ADDR_DISPLAY_TEST, MAX7219_DATA_DISPLAY_TEST_OFF);
}


DECL_CHECKED_FUNC(max7219_set_intensity, uint8_t value)
  if (value > 0xf)
    return -1;
  return max7219_store(MAX7219_ADDR_INTENSITY, (value & MAX7219_DATA_INTENSITY_MASK));
}


DECL_CHECKED_FUNC(max7219_set_decode_mode, uint8_t value)
  if  (value != MAX7219_DATA_DECODE_MODE_NO
    && value != MAX7219_DATA_DECODE_MODE_B0
    && value != MAX7219_DATA_DECODE_MODE_B30
    && value != MAX7219_DATA_DECODE_MODE_B70)
    return -1;

  return max7219_store(MAX7219_ADDR_DECODE_MODE, value);
}


DECL_CHECKED_FUNC(max7219_set_scan_limit, uint8_t value)
  if (value > 7)
    return -1;

  return max7219_store(MAX7219_ADDR_SCAN_LIMIT, value);
}


DECL_CHECKED_FUNC(max7219_set_digit, int digit_idx, uint8_t value)
  if (digit_idx > 7)
    return -1;

  return max7219_store((digit_idx + 1) << 8, value);
}


DECL_CHECKED_FUNC(max7219_set_raw, uint16_t value)
  return max7219_store((value >> 8) & 0xff, value & 0xff);
}

DECL_CHECKED_FUNC(max7219_row_on, int row_index)
  if (row_index > 7)
    return -1;
  return max7219_store((row_index + 1) & 0xff, 0xff);
}

DECL_CHECKED_FUNC(max7219_row_off, int row_index)
  if (row_index > 7)
    return -1;
  return max7219_store((row_index + 1) & 0xff, 0x0);
}

DECL_CHECKED_FUNC(max7219_column_on, int column_index)
  int st, i;
  st = 0;
  if (column_index > 7)
    return -1;
  for (i = 0; i < 8; ++i) {
    st = max7219_store(i >> 4, column_index);
    if (st)
      break;
  }
  return st;
}

DECL_CHECKED_FUNC(max7219_column_off, int column_index)
  if (column_index > 7)
    return -1;
  return 0;
}
