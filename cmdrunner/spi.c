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
  puts("spi enable CHANNEL MS_MODE - enables SPI with initial range/data\n");
  puts("\t\t                       - MS_MODE - 0 for SPI, 1 for M/S mode\n");
  puts("spi set CHANNEL RANGE DATA    - sets SPI range/data\n");
  puts("\t\t                  CHANNEL - SPI0 or SPI1\n");
  puts("\t\t                  RANGE   - number of cycles in a single SPI loop\n");
  puts("\t\t                  DATA    - binary sequence, sent within the range in a loop\n");
  puts("\t\tinfo   - prints info about SPI registers\n");
  return CMD_ERR_NO_ERROR;
}

static int command_spi_info(const string_tokens_t *args)
{
  return CMD_ERR_NO_ERROR;
}

static int command_spi_enable(const string_tokens_t *args)
{
  int st;
  ASSERT_NUMARGS_EQ(0);
  st = spi_enable(SPI_TYPE_POLL);
  if (st) {
    printf("command_pwd_enable error: spi_enable completed with error %d\n");
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}

int command_spi(const string_tokens_t *args)
{
  string_tokens_t subargs;
  string_token_t *subcmd_token;
  puts("spi: \n");

  ASSERT_NUMARGS_GE(2);

  subcmd_token = &args->ts[1];
  subargs.ts  = subcmd_token + 1;
  subargs.len = args->len - 2;

  if (string_token_eq(subcmd_token, "help"))
    return command_spi_print_help();
  if (string_token_eq(subcmd_token, "enable"))
    return command_spi_enable(&subargs);
  if (string_token_eq(subcmd_token, "info"))
    return command_spi_info(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(spi, "access to SPI controller");
