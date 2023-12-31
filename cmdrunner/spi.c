#include <cmdrunner.h>
#include "argument.h"
#include <stringlib.h>
#include <spi.h>
#include <common.h>

static spi_dev_t *spi_emulated_dev = NULL;

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
  puts("spi dmasend SPI_TYPE BYTE1 [BYTE2] - send 1 or 2 bytes over selected spi interface\n");
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
  st = spi0_init();
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
  int sclk, mosi, miso, ce0, ce1;
  char *endptr;
  ASSERT_NUMARGS_EQ(5);
  GET_NUMERIC_PARAM(sclk, int, 0, "sclk");
  GET_NUMERIC_PARAM(mosi, int, 1, "mosi");
  GET_NUMERIC_PARAM(miso, int, 2, "miso");
  GET_NUMERIC_PARAM(ce0 , int, 3, "ce0");
  GET_NUMERIC_PARAM(ce1 , int, 4, "ce1");
  spi_emulated_dev = spi_allocate_emulated("spi_cmdrunner", sclk, mosi, miso, ce0, ce1,
    SPI_EMU_MODE_MASTER);
  if (IS_ERR(spi_emulated_dev)) {
    printf("spi_emulated_init failed with error %d\n", (int)PTR_ERR(spi_emulated_dev));
    spi_emulated_dev = NULL;
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

  spidev = spi_get_dev(spi_type_from_string(subcmd_token->s, subcmd_token->len));
  if (!spidev) {
    printf("not a valid spi device %s\n", subcmd_token->s);
    return CMD_ERR_INVALID_ARGS;
  }

  numbytes = 1;
  GET_NUMERIC_PARAM(bytes[0], char, 1, "byte1");
  if (args->len > 2) {
    GET_NUMERIC_PARAM(bytes[1], char, 2, "byte2");
    numbytes++;
  }

  printf("spi0->xmit %d bytes %02x %02x.\n", numbytes, bytes[0], bytes[1]);

  st = spidev->xmit(spidev, bytes, 0, numbytes);
  if (st) {
    printf("command_pwd_enable error: spidev xmit completed with error %d\n");
    return CMD_ERR_EXECUTION_ERR;
  }

  return CMD_ERR_NO_ERROR;
}


static int command_spi_dmasend(const string_tokens_t *args)
{
  int st;
  spi_dev_t *spidev;
  char *endptr;
  void *src;
  int srclen;

  DECL_ARGS_CTX();
  ASSERT_NUMARGS_EQ(3);

  spidev = spi_get_dev(spi_type_from_string(subcmd_token->s, subcmd_token->len));
  if (!spidev) {
    printf("not a valid spi device %s\n", subcmd_token->s);
    return CMD_ERR_INVALID_ARGS;
  }

  GET_NUMERIC_PARAM(src,   void *, 1, "source addr");
  GET_NUMERIC_PARAM(srclen,   int, 2, "source len");

  printf("spi0->xmit_dma %d bytes from %p.\n", srclen, src);
  st = spidev->xmit_dma(spidev, src, 0, srclen);
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
  if (string_token_eq(subcmd_token, "dmasend"))
    return command_spi_dmasend(&subargs);
  if (string_token_eq(subcmd_token, "info"))
    return command_spi_info(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(spi, "access to SPI controller");
