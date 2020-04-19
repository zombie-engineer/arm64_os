#pragma once
#include <drivers/servo/servo.h>
#include <pwm.h>

struct servo *servo_sg90_create(struct pwm *pwm, const char *id);
int servo_sg90_init();
