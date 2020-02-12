#pragma once

typedef struct console_dev {
  int(*puts)(const char*);
  int(*putc)(char);
} console_dev_t;

void console_init();
int console_add_device(console_dev_t*, const char* devname);
void console_rm_device(const char* devname);
void console_enable_device(const char* devname);
void console_disable_device(const char* devname);

int console_putc(char);
int console_puts(const char*);

void init_consoles();

