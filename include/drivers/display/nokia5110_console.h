#pragma once
typedef struct console_dev console_dev_t;

typedef struct nokia5110_console_control {
  int initialized;
  int cursor_x;
  int cursor_y;
} nokia5110_console_control_t;

int nokia5110_console_init();

console_dev_t *nokia5110_get_console_device();
