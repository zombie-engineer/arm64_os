#include <error.h>
#include <common.h>
#include <gpio.h>
#include "board_map.h"
#include <delays.h>
#include <bits_api.h>

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

#define BSC_MASTER_S_BIT_TA   0
#define BSC_MASTER_S_BIT_DONE 1
#define BSC_MASTER_S_BIT_TXW  2
#define BSC_MASTER_S_BIT_RXR  3
#define BSC_MASTER_S_BIT_TXD  4
#define BSC_MASTER_S_BIT_RXD  5
#define BSC_MASTER_S_BIT_TXE  6
#define BSC_MASTER_S_BIT_RXF  7
#define BSC_MASTER_S_BIT_ERR  8
#define BSC_MASTER_S_BIT_CLKT 9

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

static reg32_t i2c_c    = (reg32_t)0x3f804000;
static reg32_t i2c_s    = (reg32_t)0x3f804004;
static reg32_t i2c_dlen = (reg32_t)0x3f804008;
static reg32_t i2c_a    = (reg32_t)0x3f80400c;
// static reg32_t i2c_fifo = (reg32_t)0x3f804010;
static reg32_t i2c_div  = (reg32_t)0x3f804014;
// static reg32_t i2c_del  = (reg32_t)0x3f804018;
// static reg32_t i2c_clkt = (reg32_t)0x3f80401c;

static inline void i2c_debug(const char *tag)
{
    printf("i2c_status %s: %08x, addr: %08x, cdiv:%08x, dlen:%d" __endline,
      tag,
      read_reg(i2c_s),
      read_reg(i2c_a),
      read_reg(i2c_div),
      read_reg(i2c_dlen));
}

int i2c_init()
{
  printf("i2c_init\n");
  gpio_set_function(PIN_SDA, GPIO_FUNC_ALT_0);
  gpio_set_function(PIN_SCL, GPIO_FUNC_ALT_0);

  /* pullup is not needed because it's done in in alt0 automatically */

  /* Clear status register */
//  int s_reg;
//  s_reg = read_reg(BSC_MASTER_REG_S(1));
//  s_reg |= BSC_MASTER_S_DONE;
//  s_reg |= BSC_MASTER_S_CLKT;
//  s_reg |= BSC_MASTER_S_ERR;
//  write_reg(BSC_MASTER_REG_S(1), s_reg);

  /* Clear FIFO */
  i2c_debug("before on");
  write_reg(i2c_c, BSC_MASTER_C_I2CEN | BSC_MASTER_C_CLEAR);
  i2c_debug("after on");
  // write_reg(BSC_MASTER_REG_C(1), BSC_MASTER_C_I2CEN | BSC_MASTER_C_CLEAR);
  // while(!(read_reg(BSC_MASTER_REG_S(1)) & BSC_MASTER_S_TXD));
  // printf("clear FIFO completed.\n");

  return ERR_OK;
}

#define BSC_ERR_OK 0
#define BSC_ERR_TIMEOUT -1
#define BSC_ERR_ACK -2

static inline int i2c_wait_transfer_ready(int num_retries)
{
  int i = 0;
  uint32_t s;
  for (i = 0; i < num_retries; ++i) {
    s = read_reg(BSC_MASTER_REG_S(1));
    printf("%d: %08x"__endline, i, s);
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_ERR))
      return BSC_ERR_ACK;
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_TA))
      return BSC_ERR_OK;
  }

  return BSC_ERR_TIMEOUT;
}

static inline int i2c_wait_tx_ready(int num_retries)
{
  int i = 0;
  uint32_t s;
  for (i = 0; i < num_retries; ++i) {
    s = read_reg(BSC_MASTER_REG_S(1));
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_ERR))
      return BSC_ERR_ACK;
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_TXD))
      return BSC_ERR_OK;
  }

  return BSC_ERR_TIMEOUT;
}

static inline int i2c_wait_rx_has_data(int num_retries)
{
  int i = 0;
  uint32_t s;
  for (i = 0; i < num_retries; ++i) {
    s = read_reg(BSC_MASTER_REG_S(1));
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_ERR))
      return BSC_ERR_ACK;
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_RXD))
      return BSC_ERR_OK;
  }

  return BSC_ERR_TIMEOUT;
}

static inline int i2c_wait_transfer_done(int num_retries)
{
  int i = 0;
  int s;
  for (i = 0; i < num_retries; ++i) {
    s = read_reg(BSC_MASTER_REG_S(1));
    printf("%d: %08x"__endline, i, s);
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_ERR))
      return BSC_ERR_ACK;
    if (BIT_IS_SET(s, BSC_MASTER_S_BIT_DONE))
      return BSC_ERR_OK;
  }

  return BSC_ERR_TIMEOUT;
}

#define WAIT_TRANSFER_CHECK() \
  do {\
    if (err != BSC_ERR_OK) {\
      if (err == BSC_ERR_TIMEOUT) {\
        puts("BSC_ERR_TIMEOUT"__endline);\
        return ERR_GENERIC;\
      }\
      if (err == BSC_ERR_ACK) {\
        puts("BSC_ERR_ACK"__endline);\
        return ERR_GENERIC;\
      }\
    }\
  } while(0)

#define WAIT_TRANSFER_READY() \
  do {\
    err = i2c_wait_transfer_ready(300);\
    WAIT_TRANSFER_CHECK();\
  } while(0)

#define WAIT_TRANSFER_DONE() \
  do {\
    err = i2c_wait_transfer_done(300);\
    WAIT_TRANSFER_CHECK();\
  } while(0)

#define WAIT_TX_READY() \
  do {\
    err = i2c_wait_tx_ready(300);\
    WAIT_TRANSFER_CHECK();\
  } while(0)

#define WAIT_RX_HAS_DATA() \
  do {\
    err = i2c_wait_rx_has_data(300);\
    WAIT_TRANSFER_CHECK();\
  } while(0)

int i2c_write(uint8_t i2c_addr, const char *buf, int bufsz)
{
  int i;
  int s;
  int err;

  write_reg(BSC_MASTER_REG_A(1), i2c_addr);
  write_reg(BSC_MASTER_REG_DLEN(1), bufsz);
  write_reg(BSC_MASTER_REG_C(1), BSC_MASTER_C_I2CEN | BSC_MASTER_C_ST);
  i2c_debug("i2c_write_start");

  for (i = 0; i < bufsz; ++i) {
    WAIT_TX_READY();
    write_reg(BSC_MASTER_REG_FIFO(1), buf[i]);
  }

  WAIT_TRANSFER_DONE();

  i2c_debug("after write complete");
  return bufsz;


  BUG(BIT_IS_SET(s, BSC_MASTER_S_BIT_TA),
    "I2C error: transfer is still active");

  BUG(BIT_IS_CLEAR(s, BSC_MASTER_S_BIT_DONE),
    "I2C error: transfer not done");

  BUG(BIT_IS_SET(s, BSC_MASTER_S_BIT_ERR),
    "I2C error: transfer not done");

  BUG(read_reg(i2c_dlen),
    "I2C error: DLEN should be zero");

  return bufsz;
}

int i2c_read(uint8_t i2c_addr, char *buf, int bufsz)
{
  int i;
  int s;
  int err;

  write_reg(BSC_MASTER_REG_A(1), i2c_addr);
  write_reg(BSC_MASTER_REG_DLEN(1), bufsz);
  write_reg(BSC_MASTER_REG_C(1), BSC_MASTER_C_I2CEN | BSC_MASTER_C_ST | BSC_MASTER_C_READ);
  i2c_debug("i2c_read_start");

  for (i = 0; i < bufsz; ++i) {
    WAIT_RX_HAS_DATA();
    buf[i] = read_reg(BSC_MASTER_REG_FIFO(1)) & 0xff;
  }

  WAIT_TRANSFER_DONE();
  i2c_debug("i2c_read_end");
  return bufsz;


  s = read_reg(BSC_MASTER_REG_S(1));

  BUG(BIT_IS_SET(s, BSC_MASTER_S_BIT_TA),
    "I2C error: transfer is still active");

  BUG(BIT_IS_CLEAR(s, BSC_MASTER_S_BIT_DONE),
    "I2C error: transfer not done");

  BUG(BIT_IS_SET(s, BSC_MASTER_S_BIT_ERR),
    "I2C error: transfer not done");

  BUG(read_reg(i2c_dlen),
    "I2C error: DLEN should be zero");

  return bufsz;
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
