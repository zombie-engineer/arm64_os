#pragma once

static int __i2c_init()
{
  return 0;
}

int i2c_init() __attribute__((weak, alias("__i2c_init")));
