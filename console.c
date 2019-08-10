#include "console.h"
#include "exception.h"
#include "string.h"
#include "uart.h"
#include "video_console.h"

#define MAX_CONSOLE_DEVICES 4
#define MAX_DEVNAME_SIZE 11

typedef struct console_dev_ent {
  console_dev_t dev;
  char dev_name[MAX_DEVNAME_SIZE];
  int enabled;
} console_dev_ent_t;

static console_dev_ent_t console_devices[MAX_CONSOLE_DEVICES];

static int num_console_devices;

void console_init()
{
  __builtin_memset(&console_devices[0], 0, sizeof(console_devices));
  num_console_devices = 0;
}

int console_add_device(console_dev_t* dev, const char* devname)
{
  console_dev_ent_t *con;
  if (num_console_devices > MAX_CONSOLE_DEVICES)
    generate_exception();

  if (num_console_devices == MAX_CONSOLE_DEVICES)
    return -1;

  con = &console_devices[num_console_devices];
  if (strnlen(devname, MAX_DEVNAME_SIZE) == MAX_DEVNAME_SIZE)
    return -1;
  strncpy(con->dev_name, devname, MAX_DEVNAME_SIZE - 1);
  con->dev_name[MAX_DEVNAME_SIZE] = 0;
  con->dev.puts = dev->puts;
  con->dev.putc = dev->putc;
  con->enabled = 1;
  num_console_devices++;
  return 0;
}

void console_rm_device(const char* devname)
{
  // TODO
}

void console_enable_device(const char* devname)
{
  // TODO
}

void console_disable_device(const char* devname)
{
  // TODO
}

void console_puts(const char* str)
{
  int i = 0;
  console_dev_ent_t *con;
  for (; i < MAX_CONSOLE_DEVICES; ++i)
  {
    con = &console_devices[i];
    if (con->enabled)
      con->dev.puts(str);
  }
}

void init_consoles()
{
  console_init();

  video_console_init();
  console_dev_t dev;
  dev.puts = video_console_puts;
  dev.putc = video_console_putc;
  if (console_add_device(&dev, VIDEO_CONSOLE_NAME))
    generate_exception();

  dev.puts = uart_puts;
  dev.putc = uart_putc;
  if (console_add_device(&dev, UART_CONSOLE_NAME))
    generate_exception();
}
