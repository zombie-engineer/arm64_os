#include <jtag.h>
#include <config.h>
#include <gpio_set.h>
#include <error.h>
#include <common.h>

#ifdef ENABLE_JTAG
DECL_GPIO_SET_KEY(jtag_gpio_set_key, "JTAG_ALT4_GPOIO");
gpio_set_handle_t jtag_gpio_set_handle;
#endif

#define JTAG_PIN_TDI   22
#define JTAG_PIN_TDO   23
#define JTAG_PIN_TMS   24
#define JTAG_PIN_TCK   25
#define JTAG_PIN_TRST  26
#define JTAG_PIN_TSCK  27

void jtag_init(void)
{
#ifdef ENABLE_JTAG
  int pins[6] = {
    JTAG_PIN_TDI,
    JTAG_PIN_TDO,
    JTAG_PIN_TMS,
    JTAG_PIN_TCK,
    JTAG_PIN_TRST,
    JTAG_PIN_TSCK
  };

  jtag_gpio_set_handle = gpio_set_request_n_pins(pins, ARRAY_SIZE(pins), jtag_gpio_set_key);
  if (jtag_gpio_set_handle == GPIO_SET_INVALID_HANDLE) {
    puts("Failed to request gpio pins for JTAG ATL4 function.\n");
    while(1);
  }
#endif
}
