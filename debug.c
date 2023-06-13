#include <delays.h>
#include <gpio.h>
#include <config.h>

static inline void do_blink_led(int count, unsigned int period_msec, int gpio)
{
  int i;
  uint32_t half_period = period_msec >> 1;
  for (i = 0; i < count; ++i) {
    gpio_set_on(gpio);
    wait_msec(half_period);
    gpio_set_off(gpio);
    wait_msec(half_period);
  }
}

void blink_led(int count, unsigned int period_msec)
{
  do_blink_led(count, period_msec, CONFIG_DEBUG_LED_1_GPIO_NUM);
}

void blink_led_2(int count, unsigned int period_msec)
{
  do_blink_led(count, period_msec, CONFIG_DEBUG_LED_2_GPIO_NUM);
}

void blink_led_3(int count, unsigned int period_msec)
{
  // do_blink_led(count, period_msec, CONFIG_DEBUG_LED_3_GPIO_NUM);
}

void debug_event_1()
{
  gpio_set_on(CONFIG_DEBUG_LED_2_GPIO_NUM);
  wait_msec(40);
  gpio_set_off(CONFIG_DEBUG_LED_2_GPIO_NUM);
  wait_msec(40);
}

void debug_init()
{
  gpio_set_function(CONFIG_DEBUG_LED_2_GPIO_NUM, GPIO_FUNC_OUT);
  // gpio_set_function(CONFIG_DEBUG_LED_3_GPIO_NUM, GPIO_FUNC_OUT);
}
