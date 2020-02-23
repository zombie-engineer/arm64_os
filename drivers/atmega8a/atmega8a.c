#include <drivers/atmega8a.h>
#include <gpio.h>
#include <gpio_set.h>
#include <spi.h>
#include <error.h>
#include <common.h>
#include <delays.h>

typedef struct atmega8a_dev {
  spi_dev_t *spi;
  int gpio_pin_reset;
  gpio_set_handle_t gpio_set_handle;
} atmega8a_dev_t;

static atmega8a_dev_t atmega8a_dev = {
  .spi = 0,
  .gpio_pin_reset = -1,
  .gpio_set_handle = GPIO_SET_INVALID_HANDLE
};

DECL_GPIO_SET_KEY(atmega8a_reset_key, "ATMEGA8_GPIO_RS");

#define CMD_PROGRAMMIG_ENABLE    0x000053ac

#define EXTRACT_BYTE(res, x) ((char)((res >> (x * 8)) & 0xff))

static inline uint32_t get_read_signature_bytes_cmd(int byte_idx)
{
  return 0x00000030 | (byte_idx << 16);
}

static inline uint32_t get_read_program_memory_cmd(uint32_t addr)
{
  return 0x00000020 | (addr & 0x0f00) | ((addr & 0xff) << 16);
}

int atmega8a_cmd(spi_dev_t *spidev, uint32_t cmd, uint32_t *res)
{
  int ret;
  ret = spidev->xmit((const char *)&cmd, (char *)res, 4);
  if (ret != ERR_OK) {
    printf("spidev->xmit failed: %d\n", ret);
    return ret;
  }

  printf("cmd:%02x:%02x:%02x:%02x\n",
      EXTRACT_BYTE(*res, 0),
      EXTRACT_BYTE(*res, 1),
      EXTRACT_BYTE(*res, 2),
      EXTRACT_BYTE(*res, 3)
  );
  return ERR_OK;
}

static inline int atmega8a_read_signature(spi_dev_t *spidev, uint32_t *signature)
{
  int i;
  uint32_t res;
  uint32_t tmp;
  uint32_t cmd;
  tmp = 0;
  for (i = 0; i < 3; ++i) {
    cmd = get_read_signature_bytes_cmd(i);
    if (atmega8a_cmd(spidev, cmd, &res) != ERR_OK)
      return ERR_FATAL;
    tmp = (tmp << 8) | EXTRACT_BYTE(res, 3);
  }
  *signature = tmp;
  return ERR_OK;
}

int atmega8a_init(spi_dev_t *spidev, int gpio_pin_reset)
{
  int ret;
  uint32_t res;
  gpio_set_handle_t gpio_set_handle;

  uint32_t signature;

  gpio_set_handle = gpio_set_request_1_pins(gpio_pin_reset, atmega8a_reset_key);
  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE)
    return ERR_BUSY;

  gpio_set_function(gpio_pin_reset, GPIO_FUNC_OUT);
  gpio_set_off(gpio_pin_reset);

  /* Programming enable */
  gpio_set_on(gpio_pin_reset);
  wait_msec(10);
  gpio_set_off(gpio_pin_reset);
  wait_msec(10);
  gpio_set_on(gpio_pin_reset);
  wait_msec(10);
  gpio_set_off(gpio_pin_reset);
  wait_msec(10);

  if ((ret = atmega8a_cmd(spidev, CMD_PROGRAMMIG_ENABLE, &res)) != ERR_OK)
    return ERR_FATAL;

  if (EXTRACT_BYTE(res, 2) != 0x53) {
    printf("PROGRAMMING_ENABLE command failed: 0x53 not in echo. Should write retry code. Exiting..\n");
    return ERR_FATAL;
  }

  if (atmega8a_read_signature(spidev, &signature) != ERR_OK)
      return ERR_FATAL;

  printf("Signature bytes: %08x\n", signature);
  atmega8a_dev.spi = spidev;
  atmega8a_dev.gpio_pin_reset = gpio_pin_reset;
  atmega8a_dev.gpio_set_handle = gpio_set_handle;
  return ERR_OK;
}

int atmega8a_read_program_memory(void *buf, uint32_t addr, size_t sz)
{
  uint32_t res, cmd;
  char *ptr = (char *)buf; 
  size_t i;
  for (i = 0; i < sz; ++i) {
    cmd = get_read_program_memory_cmd(addr + i);
    if (atmega8a_cmd(atmega8a_dev.spi, cmd, &res) != ERR_OK)
      return ERR_FATAL;
    *ptr++ = EXTRACT_BYTE(res, 3);
  }
  return ERR_OK;
}
