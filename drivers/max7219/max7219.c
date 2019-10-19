#include <max7219.h>
#include <gpio.h>

#define SHIFT_REG_CLOCK_PIN 17
#define SHIFT_REG_DATA_PIN 2
#define SHIFT_REG_LATCH_PIN 4


int shiftreg_push_bit(uint8_t b) 
{
  printf("%d-", b);
  if (b)
    gpio_set_on(SHIFT_REG_DATA_PIN);
  else
    gpio_set_off(SHIFT_REG_DATA_PIN);

  gpio_set_on(SHIFT_REG_CLOCK_PIN);
  wait_msec(2000);
  gpio_set_off(SHIFT_REG_CLOCK_PIN);
  wait_msec(1000);
  return 0;
}


int shiftreg_pulse_latch()
{
  gpio_set_on(SHIFT_REG_LATCH_PIN);
  wait_msec(100);
  gpio_set_off(SHIFT_REG_LATCH_PIN);
  wait_msec(100);
  return 0;
}


int shiftreg_setup()
{
  gpio_set_function(SHIFT_REG_CLOCK_PIN, GPIO_FUNC_OUT);
  gpio_set_function(SHIFT_REG_LATCH_PIN, GPIO_FUNC_OUT);
  gpio_set_function(SHIFT_REG_DATA_PIN, GPIO_FUNC_OUT);
  return 0;
}

void shiftreg_push_16bits(uint16_t value)
{
  printf("shiftpush 16bits: %04x\n", value);
  int i;
  for (i = 15; i >= 0; --i)
    shiftreg_push_bit((value >> i) & 1);
  puts("\n");
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

#define MAX7219_DATA_DISPLAY_TEST_ON  1
#define MAX7219_DATA_DISPLAY_TEST_OFF 0

#define MAX7219_DATA_SHUTDOWN_ON  1
#define MAX7219_DATA_SHUTDOWN_OFF 0

#define MAX7219_DATA_INTENSITY_MASK 0x000f

// No decode for digits 0-7
#define MAX7219_DATA_DECODE_MODE_NO   0x0
// Code B for digit 0, no decode for digits 1-7
#define MAX7219_DATA_DECODE_MODE_B0   0x1
// Code B for digits 0-3, no decode for digits 4-7
#define MAX7219_DATA_DECODE_MODE_B30  0xf
// Code B for digits 0-7
#define MAX7219_DATA_DECODE_MODE_B70  0xff


void max7219_set_shutdown_mode_on()
{
  shiftreg_push_16bits(MAX7219_ADDR_SHUTDOWN | MAX7219_DATA_SHUTDOWN_ON);
  shiftreg_pulse_latch();
}


void max7219_set_shutdown_mode_off()
{
  shiftreg_push_16bits(MAX7219_ADDR_SHUTDOWN | MAX7219_DATA_SHUTDOWN_OFF);
  shiftreg_pulse_latch();
}


void max7219_set_test_mode_on()
{
  shiftreg_push_16bits(MAX7219_ADDR_DISPLAY_TEST | MAX7219_DATA_DISPLAY_TEST_ON);
  shiftreg_pulse_latch();
}


void max7219_set_test_mode_off()
{
  shiftreg_push_16bits(MAX7219_ADDR_DISPLAY_TEST | MAX7219_DATA_DISPLAY_TEST_OFF);
  shiftreg_pulse_latch();
}


int max7219_set_intensity(uint8_t value)
{
  if (value > 0xf)
    return -1;
  shiftreg_push_16bits(MAX7219_ADDR_INTENSITY | (value & MAX7219_DATA_INTENSITY_MASK));
  shiftreg_pulse_latch();
  return 0;
}


int max7219_set_decode_mode(uint8_t value)
{
  if  (value != MAX7219_DATA_DECODE_MODE_NO 
    && value != MAX7219_DATA_DECODE_MODE_B0
    && value != MAX7219_DATA_DECODE_MODE_B30 
    && value != MAX7219_DATA_DECODE_MODE_B70)
    return -1;

  shiftreg_push_16bits(MAX7219_ADDR_DECODE_MODE | value);
  shiftreg_pulse_latch();
  return 0;
}


int max7219_set_scan_limit(uint8_t value)
{
  if (value > 7)
    return -1;

  shiftreg_push_16bits(MAX7219_ADDR_SCAN_LIMIT | value);
  shiftreg_pulse_latch();
  return 0;
}


int max7219_set_digit(int digit_idx, uint8_t value)
{
  if (digit_idx > 7)
    return -1;
  shiftreg_push_16bits((digit_idx + 1) << 8 | value);
  shiftreg_pulse_latch();
  return 0;
}


int max7219_init()
{
  int i;
  uint8_t digits[8] = { 0xaa, 0xf0, 0x1, 0x2, 0x3, 0x4, 0x5, 0xe };
  puts("max7219 starting init...\n");

  puts("max7219 display test...\n");
  max7219_set_test_mode_on();
  wait_msec(80000);
  puts("max7219 display test over.\n");
  max7219_set_test_mode_off();
  wait_msec(80000);

  for (i = 0; i < 8; ++i) {
    printf("setting digit %d to 0x%02x\n", i, digits[i]);
    if (max7219_set_digit(i, digits[i])) {
      printf("failed to set digit %d\n", i);
      return -1;
    }
    wait_msec(40000);
    return 0;
  }
  max7219_set_shutdown_mode_off();
  return 0; 

  wait_msec(40000);
  if (max7219_set_intensity(0xf)) {
    puts("failed to set intensity\n");
    return -1;
  }
  wait_msec(40000);

  if (max7219_set_scan_limit(7)) {
    puts("failed to set scan limit\n");
    return -1;
  }
  wait_msec(40000);

  if (max7219_set_decode_mode(MAX7219_DATA_DECODE_MODE_B70)) {
    puts("failed to set decode mode\n");
    return -1;
  }
  wait_msec(40000);

  puts("setting digits ...\n");

  return 0;
}
