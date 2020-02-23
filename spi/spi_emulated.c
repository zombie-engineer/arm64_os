#include <spi.h>
#include <gpio.h>
#include <gpio_set.h>
#include <types.h>
#include <common.h>
#include <delays.h>
#include <error.h>

DECL_GPIO_SET_KEY(spie_gpio_set_key, "SPI_EMU_GPIO_KE");

typedef struct spi_emulated_dev {
  spi_dev_t spidev;
  int sclk_gpio_pin;
  int mosi_gpio_pin;
  int miso_gpio_pin;
  int ce0_gpio_pin;
  int ce1_gpio_pin;
  int period;
  gpio_set_handle_t gpio_set_handle;
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
  if (spi_emulated.ce0_gpio_pin != -1) {
    gpio_set_on(spi_emulated.ce0_gpio_pin);
    wait_msec(100);
  }
  return ERR_OK;
}

DECL_ASSERTED_FN(spi_emulated_ce0_clear)
  if (spi_emulated.ce0_gpio_pin != -1) {
    gpio_set_off(spi_emulated.ce0_gpio_pin);
    wait_msec(100);
  }
  return ERR_OK;
}

DECL_ASSERTED_FN(spi_emulated_push_bit, uint8_t bit_in, uint8_t *bit_out)
  if (spi_emulated_verbose_output)
    printf("%d-", bit_in);

  if (bit_in)
    gpio_set_on(spi_emulated.mosi_gpio_pin);
  else
    gpio_set_off(spi_emulated.mosi_gpio_pin);

  gpio_set_on(spi_emulated.sclk_gpio_pin);
  wait_msec(10);
  *bit_out = gpio_is_set(spi_emulated.miso_gpio_pin);
  gpio_set_off(spi_emulated.sclk_gpio_pin);
  wait_msec(10);

  return ERR_OK;
}


DECL_ASSERTED_FN(spi_emulated_xmit_dma, const void *data_out, void *data_in, uint32_t len)
  return ERR_OK;
}


DECL_ASSERTED_FN(spi_emulated_xmit_byte, char byte_in, char *byte_out)
  int i, err;
  char out;

  if (spi_emulated_verbose_output)
    printf("spi_emulated_xmit_byte: 0x%02x\n", byte_in);

  out = 0;
  for (i = 0; i < 8; ++i) {
    uint8_t c;
    RET_IF_ERR(spi_emulated_push_bit, (byte_in >> (7 - i)) & 1, &c);
    out = (out << 1) | c;
  }

  spi_emulated_ce0_set();
  wait_msec(10);
  spi_emulated_ce0_clear();
  if (byte_out)
    *byte_out = out;

  return ERR_OK;
}

DECL_ASSERTED_FN(spi_emulated_xmit, const char* bytes_in, char *bytes_out, uint32_t len)
  int i, j, st;
  char out;

  if (spi_emulated_verbose_output)
    printf("spi_emulated_xmit: %02x, %02x\n", bytes_in[0], bytes_in[1]);

  for (j = 0; j < len; ++j) {
    for (i = 0; i < 8; ++i) {
      uint8_t c;
      st = spi_emulated_push_bit((bytes_in[j] >> (7 - i)) & 1, &c);
      if (st) {
        printf("spidev->push_bit error: %d\n", st);
        return st;
      }
      out = (out << 1) | c;
    }

    if (bytes_out)
      *bytes_out++ = out;
  }
  spi_emulated_ce0_set();
  wait_msec(10);
  spi_emulated_ce0_clear();
  return ERR_OK;
}


int spi_emulated_init(
  int sclk_pin, 
  int mosi_pin, 
  int miso_pin, 
  int ce0_pin,
  int ce1_pin)
{
  int pins[5];
  int numpins = 0;
  gpio_set_handle_t gpio_set_handle;

  if (spi_emulated_verbose_output) {
    printf("spi_emulated_init: sclk: %d, mosi: %d, miso: %d, ce0: %d, ce1: %d\n",
      sclk_pin, mosi_pin, miso_pin, ce0_pin, ce1_pin);
  }

  pins[numpins++] = sclk_pin;
  pins[numpins++] = mosi_pin;
  pins[numpins++] = miso_pin;
  if (ce0_pin != -1)
    pins[numpins++] = ce0_pin;
  if (ce1_pin != -1)
    pins[numpins++] = ce1_pin;

  gpio_set_handle = gpio_set_request_n_pins(pins, numpins, spie_gpio_set_key);
  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE)
    return ERR_BUSY;
  
  gpio_set_function(sclk_pin, GPIO_FUNC_OUT);
  gpio_set_function(mosi_pin, GPIO_FUNC_OUT);
  gpio_set_function(miso_pin, GPIO_FUNC_IN);

  if (ce0_pin != -1) {
    gpio_set_function(ce0_pin, GPIO_FUNC_OUT);
    gpio_set_off(ce0_pin);
  }
  if (ce1_pin != -1) {
    gpio_set_function(ce1_pin, GPIO_FUNC_OUT);
    gpio_set_off(ce1_pin);
  }

  gpio_set_off(sclk_pin);
  gpio_set_off(mosi_pin);

  spi_emulated.spidev.xmit      = spi_emulated_xmit;
  spi_emulated.spidev.xmit_byte = spi_emulated_xmit_byte;
  spi_emulated.spidev.xmit_dma  = spi_emulated_xmit_dma;

  spi_emulated.sclk_gpio_pin = sclk_pin;
  spi_emulated.mosi_gpio_pin = mosi_pin;
  spi_emulated.miso_gpio_pin = miso_pin;
  spi_emulated.ce0_gpio_pin = ce0_pin;
  spi_emulated.ce1_gpio_pin = ce1_pin;
  spi_emulated.gpio_set_handle = gpio_set_handle;

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
