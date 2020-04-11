#include <error.h>
#include <common.h>
#include <gpio.h>
#include "board_map.h"
#include <delays.h>

/* Control */
#define BSC_MASTER_REG_C(master_id)    BSC ## master_id ## _BASE + 0x00
/* Status */
#define BSC_MASTER_REG_S(master_id)    BSC ## master_id ## _BASE + 0x04
/* Data length */
#define BSC_MASTER_REG_DLEN(master_id) BSC ## master_id ## _BASE + 0x08
/* Slave address */
#define BSC_MASTER_REG_A(master_id)    BSC ## master_id ## _BASE + 0x0c
/* Data FIFO */
#define BSC_MASTER_REG_FIFO(master_id) BSC ## master_id ## _BASE + 0x10
/* Clock divider */
#define BSC_MASTER_REG_DIV(master_id)  BSC ## master_id ## _BASE + 0x14
/* Data delay */
#define BSC_MASTER_REG_DEL(master_id)  BSC ## master_id ## _BASE + 0x18
/* Clock timeout */
#define BSC_MASTER_REG_CLKT(master_id) BSC ## master_id ## _BASE + 0x1c

#define BSC_MASTER_C_READ  (1<<0)
#define BSC_MASTER_C_CLEAR (3<<4)
#define BSC_MASTER_C_ST    (1<<7)
#define BSC_MASTER_C_INTD  (1<<8)
#define BSC_MASTER_C_INTT  (1<<9)
#define BSC_MASTER_C_INTR  (1<<10)
#define BSC_MASTER_C_I2CEN (1<<15)

#define BSC_MASTER_S_TA    (1<<0)
#define BSC_MASTER_S_DONE  (1<<1)
#define BSC_MASTER_S_TXW   (1<<2)
#define BSC_MASTER_S_RXR   (1<<3)
#define BSC_MASTER_S_TXD   (1<<4)
#define BSC_MASTER_S_RXD   (1<<5)
#define BSC_MASTER_S_TXE   (1<<6)
#define BSC_MASTER_S_RXF   (1<<7)
#define BSC_MASTER_S_ERR   (1<<8)
#define BSC_MASTER_S_CLKT  (1<<9)

#define SLAVE_ADDR 0x1e

#define PIN_SDA 2
#define PIN_SCL 3
int i2c_init()
{
  printf("i2c_init\n");
  gpio_set_function(PIN_SDA, GPIO_FUNC_ALT_0);
  gpio_set_function(PIN_SCL, GPIO_FUNC_ALT_0);

  gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  gpio_set_pullupdown(PIN_SCL, GPIO_PULLUPDOWN_NO_PULLUPDOWN);

  // Clear status register
  int s_reg;
  s_reg = read_reg(BSC_MASTER_REG_S(1));
  s_reg |= BSC_MASTER_S_DONE;
  s_reg |= BSC_MASTER_S_CLKT;
  s_reg |= BSC_MASTER_S_ERR;
  write_reg(BSC_MASTER_REG_S(1), s_reg);


  // Clear FIFO
  write_reg(BSC_MASTER_REG_C(1), BSC_MASTER_C_CLEAR);
  while(!(read_reg(BSC_MASTER_REG_S(1)) & BSC_MASTER_S_TXD));
  printf("clear FIFO completed.\n");

  // ready the data 1 byte with value 0x66
  // will be sent to slave
  write_reg(BSC_MASTER_REG_DLEN(1), 1);
  write_reg(BSC_MASTER_REG_FIFO(1), 0x66);
  write_reg(BSC_MASTER_REG_A(1), SLAVE_ADDR);

  write_reg(BSC_MASTER_REG_C(1), 
      BSC_MASTER_C_I2CEN       | 
      BSC_MASTER_C_ST          | 
      BSC_MASTER_C_CLEAR);

  while(read_reg(BSC_MASTER_REG_S(1)) & BSC_MASTER_S_TA);
  s_reg = read_reg(BSC_MASTER_REG_S(1));
  if (s_reg & BSC_MASTER_S_DONE)
    puts("done:1\r\n");
  if (s_reg & BSC_MASTER_S_ERR)
    puts("err:1\r\n");
  if (s_reg & BSC_MASTER_S_CLKT)
    puts("CLKT:1\r\n");

  printf("Transfer completed.\n");

  while(1);
  return ERR_OK;
}

void i2c_start_transfer()
{
  uint32_t c = BSC_MASTER_C_I2CEN|BSC_MASTER_C_ST;
  write_reg(BSC_MASTER_REG_DLEN(1), 1);
  write_reg(BSC_MASTER_REG_C(1), c);
}

#define LOG_ST(msg) \
  printf("i2c:C:%08x:S:%08x-" msg "-\r\n", \
      read_reg(BSC_MASTER_REG_C(1)), \
      read_reg(BSC_MASTER_REG_S(1)));

void i2c_test()
{
  uint32_t st;
  puts("i2c_test\r\n");
  gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_EN_PULLDOWN);
  gpio_set_pullupdown(PIN_SCL, GPIO_PULLUPDOWN_EN_PULLDOWN);

  gpio_set_function(PIN_SDA, GPIO_FUNC_ALT_0);
  gpio_set_function(PIN_SCL, GPIO_FUNC_ALT_0);


  write_reg(BSC_MASTER_REG_C(1), 0);
  wait_msec(1000);
  LOG_ST("c=0");

  write_reg(BSC_MASTER_REG_C(1), BSC_MASTER_C_I2CEN|BSC_MASTER_C_CLEAR);
  LOG_ST("c=en|clear");

  // FILL FIFO
  write_reg(BSC_MASTER_REG_FIFO(1), 0xff);
  // START TRANSFER
  write_reg(BSC_MASTER_REG_A(1), 0x4e);
  write_reg(BSC_MASTER_REG_DLEN(1), 1);
  write_reg(BSC_MASTER_REG_C(1), BSC_MASTER_C_I2CEN|BSC_MASTER_C_ST);
  LOG_ST("c=en|st");
  wait_msec(1000);
  LOG_ST("c=en|st");

  while(1);

  write_reg(BSC_MASTER_REG_FIFO(1), 0x99);
  // Data length is 1 byte
  write_reg(BSC_MASTER_REG_DLEN(1), 1);
  while(1) {
    // Enable I2C Master 
    write_reg(BSC_MASTER_REG_C(1), 
        BSC_MASTER_C_I2CEN       | 
        BSC_MASTER_C_ST          | 
        BSC_MASTER_C_CLEAR);

    while(1) {
      st = read_reg(BSC_MASTER_REG_S(1));
#define CHECK_STAT(x) if (st & BSC_MASTER_S_ ## x) puts(#x"-")
      CHECK_STAT(TA);
      CHECK_STAT(DONE);
      CHECK_STAT(TXW);
      CHECK_STAT(RXR);
      CHECK_STAT(TXD);
      CHECK_STAT(RXD);
      CHECK_STAT(TXE);
      CHECK_STAT(RXF);
      CHECK_STAT(ERR);
      CHECK_STAT(CLKT);
      if (st & BSC_MASTER_S_TA)
        break;
      puts("-\r\n");
    }
    puts("--\r\n");
    // write_reg(BSC_MASTER_REG_C(0), BSC_MASTER_C_READ | BSC_MASTER_C_I2CEN);
    // c = read_reg(BSC_MASTER_REG_FIFO(0));
    // printf("read from i2c:%x\n", (int)c);
    wait_msec(2000);
  }
}
