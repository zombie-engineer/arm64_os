#pragma once

/* pwm_enable  - enables pwm 
 * channel_id  - enable one of two channels 0 or 1
 */
int pwm_enable(int channel, int ms_mode);

int pwm_set(int channel, int range, int data);
