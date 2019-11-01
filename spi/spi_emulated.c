#include <spi.h>
#include <gpio.h>
#include <types.h>
#include <common.h>
#include <delays.h>


typedef struct spi_emulated_dev {
  spi_dev_t spidev;
  int sclk_gpio_pin;
  int mosi_gpio_pin;
  int miso_gpio_pin;
  int ce0_gpio_pin;
  int ce1_gpio_pin;
} spi_emulated_dev_t;



static spi_emulated_dev_t spi_emulated;
static int spi_emulated_verbose_output = 0;
static int spi_emulated_initialized = 0;

#define ASSERT_INITIALIZED(fname) \
  if (!spi_emulated_initialized) { \
    puts(fname " error: spi not initialized\n"); \
    return SPI_ERR_NOT_INITIALIZED; \
  }


#define DECL_ASSERTED_FN(fn, ...) \
static int fn(__VA_ARGS__) \
{ \
  ASSERT_INITIALIZED(#fn);


DECL_ASSERTED_FN(spi_emulated_ce0_set)
  ASSERT_INITIALIZED();
  gpio_set_on(spi_emulated.ce0_gpio_pin);
  wait_msec(100);
  return 0;
}


DECL_ASSERTED_FN(spi_emulated_ce0_clear)
  gpio_set_off(spi_emulated.ce0_gpio_pin);
  wait_msec(100);
  return 0;
}

DECL_ASSERTED_FN(spi_emulated_push_bit, uint8_t b)
  if (spi_emulated_verbose_output)
    printf("%d-", b);

  if (b)
    gpio_set_on(spi_emulated.mosi_gpio_pin);
  else
    gpio_set_off(spi_emulated.mosi_gpio_pin);

  gpio_set_on(spi_emulated.sclk_gpio_pin);
  wait_msec(10);
  gpio_set_off(spi_emulated.sclk_gpio_pin);
  wait_msec(10);
  return 0;
}

DECL_ASSERTED_FN(spi_emulated_xmit, char* bytes, uint32_t len)
  int i, j, st;

  if (spi_emulated_verbose_output)
    printf("spi_emulated_xmit: %02x, %02x\n", bytes[0], bytes[1]);

  for (j = 0; j < len; ++j) {
    for (i = 0; i < 8; ++i) {
      st = spi_emulated_push_bit((bytes[j] >> (7 - i)) & 1);
      if (st) {
        printf("spidev->push_bit error: %d\n", st);
        return st;
      }
    }
  }
  spi_emulated_ce0_set();
  wait_msec(10);
  spi_emulated_ce0_clear();
  return 0;
}


int spi_emulated_init(
  int sclk_pin, 
  int mosi_pin, 
  int miso_pin, 
  int ce0_pin,
  int ce1_pin)
{
  if (spi_emulated_verbose_output) {
    printf("spi_emulated_init: sclk: %d, mosi: %d, miso: %d, ce0: %d, ce1: %d\n",
      sclk_pin, mosi_pin, miso_pin, ce0_pin, ce1_pin);
  }

  gpio_set_function(sclk_pin, GPIO_FUNC_OUT);
  gpio_set_function(mosi_pin, GPIO_FUNC_OUT);
  // gpio_set_function(miso_pin, GPIO_FUNC_IN);
  gpio_set_function(ce0_pin, GPIO_FUNC_OUT);
  // gpio_set_function(ce1_pin, GPIO_FUNC_OUT);

  gpio_set_off(sclk_pin);
  gpio_set_off(mosi_pin);
  // gpio_set_off(miso_pin, GPIO_FUNC_IN);
  gpio_set_off(ce0_pin);
  // gpio_set_off(ce1_pin);

  spi_emulated.spidev.xmit     = spi_emulated_xmit;

  spi_emulated.sclk_gpio_pin = sclk_pin;
  spi_emulated.mosi_gpio_pin = mosi_pin;
  spi_emulated.miso_gpio_pin = miso_pin;
  spi_emulated.ce0_gpio_pin = ce0_pin;
  spi_emulated.ce1_gpio_pin = ce1_pin;

  spi_emulated_initialized = 1;
  return SPI_ERR_OK;
}

spi_dev_t *spi_emulated_get_dev()
{
  return &spi_emulated.spidev;
}

void spi_emulated_print_info()
{
  puts("spi_emulated debug info:\n");
  printf(" verbose_output: %d\n", spi_emulated_verbose_output);
  printf(" initialized   : %d\n", spi_emulated_initialized);
}
