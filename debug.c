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
