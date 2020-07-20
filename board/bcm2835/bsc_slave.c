#include <error.h>
#include <common.h>
#include <gpio.h>
#include <gpio_set.h>
#include <board/bcm2835/bcm2835.h>
#include "board_map.h"

#define BSC_SLAVE_DR      0x00
#define BSC_SLAVE_RSR     0x04
#define BSC_SLAVE_SLV     0x08
#define BSC_SLAVE_CR      0x0c
#define BSC_SLAVE_FR      0x10
#define BSC_SLAVE_IFLS    0x14
#define BSC_SLAVE_IMSC    0x18
#define BSC_SLAVE_RIS     0x1c
#define BSC_SLAVE_MIS     0x20
#define BSC_SLAVE_ICR     0x24
#define BSC_SLAVE_DMACR   0x28
#define BSC_SLAVE_TDR     0x2c
#define BSC_SLAVE_GPUSTAT 0x30
#define BSC_SLAVE_HCTLR   0x34
#define BSC_SLAVE_DEBUG1  0x38
#define BSC_SLAVE_DEBUG2  0x3c

#define BSC_SLAVE_SDA_PIN 18
#define BSC_SLAVE_SCL_PIN 19

#define BSC_SLAVE_CR_EN          0
#define BSC_SLAVE_CR_SPI         1
#define BSC_SLAVE_CR_I2C         2
#define BSC_SLAVE_CR_CPHA        3
#define BSC_SLAVE_CR_CPOL        4
#define BSC_SLAVE_CR_ENSTAT      5
#define BSC_SLAVE_CR_ENCTRL      6
#define BSC_SLAVE_CR_BRK         7
#define BSC_SLAVE_CR_TXE         8
#define BSC_SLAVE_CR_RXE         9
#define BSC_SLAVE_CR_INV_RXF    10
#define BSC_SLAVE_CR_TESTFIFO   11
#define BSC_SLAVE_CR_HOSTCTRLEN 12
#define BSC_SLAVE_CR_INV_TXF    13

DECL_GPIO_SET_KEY(gpio_set_key_bsc_i2c_slave, "BSC_SLAVE_I2C_K");

static char bsc_slave_addr = 0xff;
static int bsc_slave_mode = 0;

static int bsc_slave_init_as_spi(char addr)
{
  return ERR_NOT_IMPLEMENTED;
}

static int bsc_slave_init_as_i2c(char addr)
{
  int pins[2] = { BSC_SLAVE_SDA_PIN, BSC_SLAVE_SCL_PIN };

  gpio_set_handle_t gpio_set_handle;

  gpio_set_handle = gpio_set_request_n_pins(pins, ARRAY_SIZE(pins),
      gpio_set_key_bsc_i2c_slave);

  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE)
    return ERR_BUSY;

  gpio_set_function(BSC_SLAVE_SDA_PIN, GPIO_FUNC_ALT_3);
  gpio_set_function(BSC_SLAVE_SCL_PIN, GPIO_FUNC_ALT_3);

  write_reg(BSC_SLAVE_SLV, addr);
  write_reg(BSC_SLAVE_CR, (1<<BSC_SLAVE_CR_EN) |
      (1<<BSC_SLAVE_CR_I2C) |
      (1<<BSC_SLAVE_CR_TXE) |
      (1<<BSC_SLAVE_CR_RXE));

  return ERR_OK;
}

int bsc_slave_init(int mode, char addr)
{
  int err = ERR_OK;

  bsc_slave_addr = addr;
  bsc_slave_mode = mode;

  if (mode == BSC_SLAVE_MODE_I2C)
    err = bsc_slave_init_as_i2c(addr);
  else if (mode == BSC_SLAVE_MODE_SPI)
    err = bsc_slave_init_as_spi(addr);
  else
    err = ERR_INVAL_ARG;

  return err;
}

int bsc_slave_debug()
{
  int debug_i2c, ris, icr, fr, dr, rsr;
  while(1) {
    dr  = read_reg(BSC_SLAVE_DR);
    rsr = read_reg(BSC_SLAVE_RSR);
    fr  = read_reg(BSC_SLAVE_FR);
    ris = read_reg(BSC_SLAVE_RIS);
    icr = read_reg(BSC_SLAVE_ICR);
    debug_i2c = read_reg(BSC_SLAVE_DEBUG1);
    printf("dr:%08x,rsr:%08x,fr:%08x,", dr, rsr, fr);
    printf("ris:%08x,icr:%08x,debug:%08x\r\n", ris, icr, debug_i2c);
    if ((dr & 0xff) == 0x66)
      break;
  }
  return ERR_OK;
}

