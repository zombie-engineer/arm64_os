#include <drivers/atmega8a.h>
#include <gpio.h>
#include <spi.h>
#include <error.h>
#include <common.h>
#include <delays.h>

#define CMD_PROGRAMMIG_ENABLE    0x000053ac
#define CMD_READ_SIGNATURE_BYTES(x) (0x00000030 | (x) << (2 * 8))

#define CMD_RES_BYTE(res, x) (char)((res >> (x * 8)) & 0xff)

int atmega8a_set_cmd(spi_dev_t *spidev, uint32_t cmd, uint32_t *res)
{
  int ret;
  ret = spidev->xmit((const char *)&cmd, (char *)res, 4);
  if (ret != ERR_OK) {
    printf("spidev->xmit failed: %d\n", ret);
    return ret;
  }

  printf("cmd_programming_enable:%02x:%02x:%02x:%02x\n", 
      CMD_RES_BYTE(*res, 0), 
      CMD_RES_BYTE(*res, 1), 
      CMD_RES_BYTE(*res, 2), 
      CMD_RES_BYTE(*res, 3)
  );
  return ERR_OK;
}

int atmega8a_init(int gpio_pin_miso, int gpio_pin_mosi, int gpio_pin_sclk, int gpio_pin_reset)
{
  int ret;
  uint32_t res;

  char signature[3];

  spi_dev_t *spidev;
  spi_emulated_init(gpio_pin_sclk, gpio_pin_mosi, gpio_pin_miso, -1, -1);
  spidev = spi_get_dev(SPI_TYPE_EMULATED);
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

  if ((ret = atmega8a_set_cmd(spidev, CMD_PROGRAMMIG_ENABLE, &res)) != ERR_OK)
    return ERR_FATAL;

  if (CMD_RES_BYTE(res, 2) != 0x53) {
    printf("PROGRAMMING_ENABLE command failed: 0x53 not in echo. Should write retry code. Exiting..\n");
    return ERR_FATAL;
  }

  if ((ret = atmega8a_set_cmd(spidev, CMD_READ_SIGNATURE_BYTES(0), &res)) != ERR_OK)
    return ERR_FATAL;
  signature[0] = (res >> (3 * 8)) & 0xff;
  if ((ret = atmega8a_set_cmd(spidev, CMD_READ_SIGNATURE_BYTES(1), &res)) != ERR_OK)
    return ERR_FATAL;
  signature[1] = (res >> (3 * 8)) & 0xff;
  if ((ret = atmega8a_set_cmd(spidev, CMD_READ_SIGNATURE_BYTES(2), &res)) != ERR_OK)
    return ERR_FATAL;
  signature[2] = (res >> (3 * 8)) & 0xff;
  printf("Signature bytes: %02x:%02x:%02x\n", signature[0], signature[1], signature[2]);
  return ERR_OK;
}
