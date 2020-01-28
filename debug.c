#include <delays.h>
#include <gpio.h>

void blink_led(int count, unsigned int period_msec)
{
  int i;
  uint32_t half_period = period_msec >> 1;
  for (i = 0; i < count; ++i) {
    gpio_set_on(29);
    wait_msec(half_period);
    gpio_set_off(29);
    wait_msec(half_period);
  }
}

void debug_event_1()
{
  gpio_set_on(21);
  wait_msec(40);
  gpio_set_off(21);
}

void debug_init()
{
  gpio_set_function(21, GPIO_FUNC_OUT);
}
