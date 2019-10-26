#include <shiftreg.h>
#include <gpio.h>
#include <delays.h>


#define PULSE(time) \
  gpio_set_on(srclk_pin);    \
  wait_msec(time);       \
  gpio_set_off(srclk_pin);   \
  wait_msec(time)

#define PULSE_RCLK(time) \
    gpio_set_off(rclk_pin);  \
    wait_msec(time);     \
    gpio_set_on(rclk_pin);   \

#define PULSE_SRCLR()     \
    gpio_set_off(srclr);  \
    wait_msec(80000);     \
    gpio_set_on(srclr);   \
    wait_msec(80000);     \
    gpio_set_off(srclr);  \
    wait_msec(80000);     \
    gpio_set_on(srclr);   \
 
#define PUSH_BIT(b, time) {\
    if (b) gpio_set_on(ser_pin); else gpio_set_off(ser_pin); \
    wait_msec(time);    \
    PULSE(time); \
  }

#define CHECK_INITIALIZED() if (!shiftreg_initialized) return -1;

static uint32_t ser_pin;
static uint32_t srclk_pin;
static uint32_t rclk_pin;

static int shiftreg_initialized = 0;

#define CHECKED_CALL(fn, ...) if (fn(__VA_ARGS__)) return -1;

int shiftreg_init(uint32_t ser, uint32_t srclk, uint32_t rclk)
{
  CHECKED_CALL(gpio_set_function, ser  , GPIO_FUNC_OUT);
  CHECKED_CALL(gpio_set_function, srclk, GPIO_FUNC_OUT);
  CHECKED_CALL(gpio_set_function, rclk , GPIO_FUNC_OUT);

  gpio_set_off (ser); 
  gpio_set_off (rclk); 
  gpio_set_off (srclk); 

  ser_pin = ser;
  srclk_pin = srclk;
  rclk_pin = rclk;

  shiftreg_initialized = 1;

 // gpio_set_pullupdown (ser  , GPIO_PULLUPDOWN_EN_PULLDOWN); 
 // gpio_set_pullupdown (rclk , GPIO_PULLUPDOWN_EN_PULLDOWN); 
 // gpio_set_pullupdown (srclk, GPIO_PULLUPDOWN_EN_PULLDOWN); 
 return 0;
}

int shiftreg_push_bit(char bit)
{
  CHECK_INITIALIZED();
  PUSH_BIT(bit, 30000);
  return 0;
}

int shiftreg_push_byte(char byte)
{
  CHECK_INITIALIZED();
  int i;
  char b;
  for (i = 0; i < 8; ++i) {
    b = (byte >> (7 - i)) & 1;
    PUSH_BIT(b, 30000);
  }
  return 0;
}

int shiftreg_pulse_rclk()
{
  CHECK_INITIALIZED();
  PULSE_RCLK(30000);
  return 0;
}
