#pragma once

typedef struct console_dev {
  void(*puts)(const char*);
  void(*putc)(char);
} console_dev_t;

void console_init();
int console_add_device(console_dev_t*, const char* devname);
void console_rm_device(const char* devname);
void console_enable_device(const char* devname);
void console_disable_device(const char* devname);

void console_puts(const char*);

void init_consoles();

