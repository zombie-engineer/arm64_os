#include <cmdrunner.h>
#include "argument.h"
#include <stdlib.h>
#include <spi.h>
#include <common.h>
#include <string.h>

static int command_spi_print_help()
{
  puts("spi COMMAND VALUES\n");
  puts("\tCOMMANDS:\n");
  puts("spi init SPI_TYPE [OPTIONS] - initialize SPI bus of a specified type\n");
  puts("                            - SPI_TYPE: spi0, spi1, emulated\n");
  puts("                   OPTIONS  - for spi0 no options\n");
  puts("                   OPTIONS  - for spi-emulated OPTIONS are in format 'SCLK_PIN MOSI_PIN MISO_PIN CE0_PIN CE1_PIN'\n");
  puts("                            - where each *PIN is a index of a gpio pin that would take on the role of the\n");
  puts("                            - correspongin SPI function.\n");
  puts("                            - ex: spi init emulated 3 4 10 15\n");
  puts("                            -     init spi emulated on gpio pins 3(SCLK),4(MOSI),10(MISO) 15(CE0) and 16(CE1)\n");
  puts("                            - ex: spi init spi0\n");
  puts("spi set CHANNEL RANGE DATA    - sets SPI range/data\n");
  puts("\t\t                  CHANNEL - SPI0 or SPI1\n");
  puts("\t\t                  RANGE   - number of cycles in a single SPI loop\n");
  puts("\t\t                  DATA    - binary sequence, sent within the range in a loop\n");
  puts("spi send SPI_TYPE BYTE1 [BYTE2] - send 1 or 2 bytes over selected spi interface\n");
  puts("                      SPI_TYPE  - spi0, spi1, spi2, emulated\n");
  puts("                      BYTEx     - byte values BYTE1 should be, BYTE2 optional\n");
  puts("\t\tinfo   - prints info about SPI registers\n");
  return CMD_ERR_NO_ERROR;
}

static int command_spi_info(const string_tokens_t *args)
{
  spi_emulated_print_info();
  return CMD_ERR_NO_ERROR;
}


static int command_spi_init_spi0(const string_tokens_t *args)
{
  int st;
  st = spi0_init(SPI_TYPE_POLL);
  if (st) {
    printf("command_pwd_enable error: spi0_init completed with error %d\n");
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}


static int command_spi_init_spi1(const string_tokens_t *args)
{
  return CMD_ERR_NOT_IMPLEMENTED;
}


static int command_spi_init_emulated(const string_tokens_t *args)
{
  int st, sclk, mosi, miso, ce0, ce1;
  char *endptr;
  ASSERT_NUMARGS_EQ(5);
  GET_NUMERIC_PARAM(sclk, int, 0, "sclk");
  GET_NUMERIC_PARAM(mosi, int, 1, "mosi");
  GET_NUMERIC_PARAM(miso, int, 2, "miso");
  GET_NUMERIC_PARAM(ce0 , int, 3, "ce0");
  GET_NUMERIC_PARAM(ce1 , int, 4, "ce1");
  st = spi_emulated_init(sclk, mosi, miso, ce0, ce1);
  if (st) {
    printf("spi_emulated_init failed with error %d\n", st);
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}


static int command_spi_send(const string_tokens_t *args)
{
  int st, numbytes;
  char bytes[8];
  spi_dev_t *spidev;
  char *endptr;

  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(2);

  if (string_token_eq(subcmd_token, "spi0"))
     spidev = spi0_get_dev();
  else if (string_token_eq(subcmd_token, "spi1"))
     spidev = spi1_get_dev();
  else if (string_token_eq(subcmd_token, "spi2"))
     spidev = spi2_get_dev();
  else if (string_token_eq(subcmd_token, "emulated"))
     spidev = spi_emulated_get_dev();
  else {
    printf("not a valid spi device %s\n", subcmd_token->s);
    return CMD_ERR_INVALID_ARGS;
  }

  if (!spidev) {
    puts("spi device not initialized\n");
    return CMD_ERR_INVALID_ARGS;
  }

  numbytes = 1;
  GET_NUMERIC_PARAM(bytes[0], char, 1, "byte1");
  if (args->len > 2) {
    GET_NUMERIC_PARAM(bytes[1], char, 2, "byte2");
    numbytes++;
  }

  printf("spi0->xmit %d bytes %02x %02x.\n", numbytes, bytes[0], bytes[1]);

  st = spidev->xmit(bytes, numbytes);
  if (st) {
    printf("command_pwd_enable error: spidev xmit completed with error %d\n");
    return CMD_ERR_EXECUTION_ERR;
  }

  return CMD_ERR_NO_ERROR;
}


static int command_spi_init(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "spi0"))
    return command_spi_init_spi0(&subargs);
  if (string_token_eq(subcmd_token, "spi1"))
    return command_spi_init_spi1(&subargs);
  if (string_token_eq(subcmd_token, "emulated"))
    return command_spi_init_emulated(&subargs);

  puts("command_spi_init error: Unknown subcommand\n");
  return CMD_ERR_UNKNOWN_SUBCMD;
}


int command_spi(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_spi_print_help();
  if (string_token_eq(subcmd_token, "init"))
    return command_spi_init(&subargs);
  if (string_token_eq(subcmd_token, "send"))
    return command_spi_send(&subargs);
  if (string_token_eq(subcmd_token, "info"))
    return command_spi_info(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(spi, "access to SPI controller");
