#pragma once
#include <types.h>

/* pwm_enable  - enables pwm
 * channel_id  - enable one of two channels 0 or 1
 */

struct pwm {
  int (*set_clk_freq)(struct pwm *pwm, int hz);
  int (*set)(struct pwm *pwm, uint32_t range, uint32_t data);
  int (*set_range)(struct pwm *pwm, uint32_t range);
  int (*set_data)(struct pwm *pwm, uint32_t data);
  int (*release)(struct pwm *pwm);
};

struct pwm *pwm_bcm2835_create(int gpio_pin, int ms_mode);

int bcm2835_set_pwm_clk_freq(int hz);

int pwm_enable(int channel, int ms_mode);

int pwm_set(int channel, int range, int data);

int pwm_prepare(int gpio_pwm0, int gpio_pwm1);

void pwm_bcm2835_init();
