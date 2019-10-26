#include <shiftreg.h>
#include <gpio.h>
#include <delays.h>
#include <common.h>


#define SRCLK_PULSE(sr)    \
  gpio_set_on(sr->srclk);  \
  wait_msec(sr->delay_ms); \
  gpio_set_off(sr->srclk); \
  wait_msec(sr->delay_ms)


#define RCLK_PULSE(sr)      \
    gpio_set_off(sr->rclk); \
    wait_msec(sr->delay_ms);\
    gpio_set_on(sr->rclk);  \


#define SRCLR_PULSE(sr)      \
    gpio_set_off(sr->srclr); \
    wait_msec(sr->delay_ms); \
    gpio_set_on(sr->srclr);
 

#define PUSH_BIT(sr, b) {    \
    if (b)                   \
      gpio_set_on(sr->ser);  \
    else                     \
      gpio_set_off(sr->ser); \
    wait_msec(sr->delay_ms); \
    SRCLK_PULSE(sr);         \
  }


#define CHECK_INITIALIZED(sr) if (!shiftreg_initialized || !sr) return -1;

static shiftreg_t shiftreg;
static int shiftreg_initialized = 0;

#define CHECKED_CALL(fn, ...) if (fn(__VA_ARGS__)) return -1;

shiftreg_t *shiftreg_init(int32_t ser, int32_t srclk, int32_t rclk, int32_t srclr, int32_t ce)
{
  if (ser == SHIFTREG_INIT_PIN_DISABLED || srclk == SHIFTREG_INIT_PIN_DISABLED || rclk == SHIFTREG_INIT_PIN_DISABLED) {
    puts("shiftreg_init: not possible to init shiftreg without ser, srclk and rclk pins set.\n");
    return 0;
  }

  shiftreg.ser   = ser;
  shiftreg.srclk = srclk;
  shiftreg.rclk  = rclk;
  shiftreg.srclr = srclr;
  shiftreg.ce    = ce;

  // srclk = 21;
  // rclk  = 20;
  // ser   = 16;
  // srclr = 12;

  gpio_set_function(ser  , GPIO_FUNC_OUT); 
  gpio_set_function(rclk , GPIO_FUNC_OUT); 
  gpio_set_function(srclk, GPIO_FUNC_OUT); 
  // gpio_set_function(srclr, GPIO_FUNC_OUT); 

  gpio_set_off (ser); 
  gpio_set_off (rclk); 
  gpio_set_off (srclk); 
  // gpio_set_off (srclr); 

  // gpio_set_pullupdown (ser  , GPIO_PULLUPDOWN_EN_PULLDOWN); 
  // gpio_set_pullupdown (rclk , GPIO_PULLUPDOWN_EN_PULLDOWN); 
  // gpio_set_pullupdown (srclk, GPIO_PULLUPDOWN_EN_PULLDOWN); 
  // gpio_set_pullupdown (srclr, GPIO_PULLUPDOWN_EN_PULLDOWN); 

  shiftreg_initialized = 1;
  return &shiftreg;
}

int shiftreg_push_bit(shiftreg_t *sr, char bit)
{
  CHECK_INITIALIZED(sr);
  PUSH_BIT(sr, bit);
  return 0;
}

int shiftreg_push_byte(shiftreg_t *sr, char byte)
{
  CHECK_INITIALIZED(sr);
  int i;
  char b;
  for (i = 0; i < 8; ++i) {
    b = (byte >> (7 - i)) & 1;
    PUSH_BIT(sr, b);
  }
  return 0;
}

int shiftreg_pulse_rclk(shiftreg_t *sr)
{
  CHECK_INITIALIZED(sr);
  RCLK_PULSE(sr);
  return 0;
}
