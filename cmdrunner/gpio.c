#include <cmdrunner.h>
#include "argument.h"
#include <stdlib.h>
#include <gpio.h>
#include <common.h>
#include <string.h>


#define GET_PIN_IDX() GET_NUMERIC_PARAM(pin_idx, int, 0, "pin_idx")


typedef struct gpio_function {
  char name[32];
  int len;
} gpio_function_t;

typedef struct gpio_functions {
  gpio_function_t f[6];
} gpio_functions_t; 

static int gpio_functions_set = 0;

static gpio_functions_t gpio_funcs[54];

typedef struct gpio_fn_alias {
  char alias[8];
  int value;
} gpio_fn_alias_t;

typedef struct gpio_fn_aliases {
  gpio_fn_alias_t a[32];
  int len;
} gpio_fn_aliases_t;

static gpio_fn_aliases_t gpio_aliases;
#define GET_GPIOFN(pin_idx, func_idx) (&gpio_funcs[pin_idx].f[func_idx])

#define SET_GPIOFNNAME(pin_idx, func_idx, n) \
  strncpy(GET_GPIOFN(pin_idx, func_idx)->name, n, sizeof(n)); \
  GET_GPIOFN(pin_idx, func_idx)->len = sizeof(n);

#define SET_GPIOFNS(i, alt0, alt1, alt2, alt3, alt4, alt5) \
  SET_GPIOFNNAME(i, 0, alt0); \
  SET_GPIOFNNAME(i, 1, alt1); \
  SET_GPIOFNNAME(i, 2, alt2); \
  SET_GPIOFNNAME(i, 3, alt3); \
  SET_GPIOFNNAME(i, 4, alt4); \
  SET_GPIOFNNAME(i, 5, alt5);

void command_gpio_init_functions()
{
  memset(&gpio_funcs, 0, sizeof(gpio_funcs));

  SET_GPIOFNS(0 , "SDA0"      , "SA5"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(1 , "SCL0"      , "SA4"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(2 , "SDA1"      , "SA3"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(3 , "SCL1"      , "SA2"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(4 , "GPCLK0"    , "SA1"        , "<reserved>", ""              , ""          , "ARM_TDI" );
  SET_GPIOFNS(5 , "GPCLK1"    , "SA0"        , "<reserved>", ""              , ""          , "ARM_TDO" );
  SET_GPIOFNS(6 , "GPCLK2"    , "SOE_N/SE"   , "<reserved>", ""              , ""          , "ARM_RTCK");
  SET_GPIOFNS(7 , "SPI0_CE1_N", "SWE_N/SRW_N", "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(8 , "SPI0_CE0_N", "SD0"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(9 , "SPI0_MISO" , "SD1"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(10, "SPI0_MOSI" , "SD2"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(11, "SPI0_SCLK" , "SD3"        , "<reserved>", ""              , ""          , ""        );
  SET_GPIOFNS(12, "PWM0"      , "SD4"        , "<reserved>", ""              , ""          , "ARM_TMS" );
  SET_GPIOFNS(13, "PWM1"      , "SD5"        , "<reserved>", ""              , ""          , "ARM_TCK" );
  SET_GPIOFNS(14, "TXD0"      , "SD6"        , "<reserved>", ""              , ""          , "TXD1"    );
  SET_GPIOFNS(15, "RXD0"      , "SD7"        , "<reserved>", ""              , ""          , "RXD1"    );
  SET_GPIOFNS(16, "<reserved>", "SD8"        , "<reserved>", "CTS0"          , "SPI1_CE2_N", "CTS1"    );
  SET_GPIOFNS(17, "<reserved>", "SD9"        , "<reserved>", "RTS0"          , "SPI1_CE1_N", "RTS1"    );
  SET_GPIOFNS(18, "PCM_CLK"   , "SD10"       , "<reserved>", "BSCSL_SDA/MOSI", "SPI1_CE0_N", "PWM0"    );
  SET_GPIOFNS(19, "PCM_FS"    , "SD11"       , "<reserved>", "BSCSL_SCL/SCLK", "SPI1_MISO" , "PWM1"    );
  SET_GPIOFNS(20, "PCM_DIN"   , "SD12"       , "<reserved>", "BSCSL/MISO"    , "SPI1_MOSI" , "GPCLK0"  );
  SET_GPIOFNS(21, "PCM_DOUT"  , "SD13"       , "<reserved>", "BSCSL/CE_N"    , "SPI1_SCLK" , "GPCLK1"  );
  SET_GPIOFNS(22, "<reserved>", "SD14"       , "<reserved>", "SD1_CLK"       , "ARM_TRST"  , ""        );
  SET_GPIOFNS(23, "<reserved>", "SD15"       , "<reserved>", "SD1_CMD"       , "ARM_RTCK"  , ""        );
  SET_GPIOFNS(24, "<reserved>", "SD16"       , "<reserved>", "SD1_DAT0"      , "ARM_TDO"   , ""        );
  SET_GPIOFNS(25, "<reserved>", "SD17"       , "<reserved>", "SD1_DAT1"      , "ARM_TCK"   , ""        );
  SET_GPIOFNS(26, "<reserved>", "<reserved>" , "<reserved>", "SD1_DAT2"      , "ARM_TDI"   , ""        );
  SET_GPIOFNS(27, "<reserved>", "<reserved>" , "<reserved>", "SD1_DAT3"      , "ARM_TMS"   , ""        );
  SET_GPIOFNS(28, "SDA0"      , "SA5"        , "PCM_CLK"   , "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(29, "SCL0"      , "SA4"        , "PCM_FS"    , "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(30, "<reserved>", "SA3"        , "PCM_DIN"   , "CTS0"          , ""          , "CTS1"    );
  SET_GPIOFNS(31, "<reserved>", "SA2"        , "PCM_DOUT"  , "RTS0"          , ""          , "RTS1"    );
  SET_GPIOFNS(32, "GPCLK0"    , "SA1"        , "<reserved>", "TXD0"          , ""          , "TXD1"    );
  SET_GPIOFNS(33, "<reserved>", "SA0"        , "<reserved>", "RXD0"          , ""          , "RXD1"    );
  SET_GPIOFNS(34, "GPCLK0"    , "SOE_N/SE"   , "<reserved>", "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(35, "SPI0_CE1_N", "SWE_N/SRW_N", ""          , "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(36, "SPI0_CE0_N", "SD0"        , "TXD0"      , "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(37, "SPI0_MISO" , "SD1"        , "RXD0"      , "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(38, "SPI0_MOSI" , "SD2"        , "RTS0"      , "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(39, "SPI0_SCLK" , "SD3"        , "CTS0"      , "<reserved>"    , ""          , ""        );
  SET_GPIOFNS(40, "PWM0"      , "SD4"        , ""          , "<reserved>"    , "SPI2_MISO" , "TXD1"    );
  SET_GPIOFNS(41, "PWM1"      , "SD5"        , "<reserved>", "<reserved>"    , "SPI2_MOSI" , "RXD1"    );
  SET_GPIOFNS(42, "GPCLK1"    , "SD6"        , "<reserved>", "<reserved>"    , "SPI2_SCLK" , "RTS1"    );
  SET_GPIOFNS(43, "GPCLK2"    , "SD7"        , "<reserved>", "<reserved>"    , "SPI2_CE0_N", "CTS1"    );
  SET_GPIOFNS(44, "GPCLK1"    , "SDA0"       , "SDA1"      , "<reserved>"    , "SPI2_CE1_N", ""        );
  SET_GPIOFNS(45, "PWM1"      , "SCL0"       , "SCL1"      , "<reserved>"    , "SPI2_CE2_N", ""        );
  SET_GPIOFNS(46, "<internal>", ""           , ""          , ""              , ""          , ""        );
  SET_GPIOFNS(47, "<internal>", ""           , ""          , ""              , ""          , ""        );
  SET_GPIOFNS(48, "<internal>", ""           , ""          , ""              , ""          , ""        );
  SET_GPIOFNS(49, "<internal>", ""           , ""          , ""              , ""          , ""        );
  SET_GPIOFNS(50, "<internal>", ""           , ""          , ""              , ""          , ""        );
  SET_GPIOFNS(51, "<internal>", ""           , ""          , ""              , ""          , ""        );
  SET_GPIOFNS(52, "<internal>", ""           , ""          , ""              , ""          , ""        );
  SET_GPIOFNS(53, "<internal>", ""           , ""          , ""              , ""          , ""        );

  memset(&gpio_aliases, 0, sizeof(gpio_aliases));

#define ADD_ALIAS(v, n) \
  strcpy(gpio_aliases.a[gpio_aliases.len].alias, n); \
  gpio_aliases.a[gpio_aliases.len].value = v; \
  gpio_aliases.len++

  ADD_ALIAS(GPIO_FUNC_IN  , "input");
  ADD_ALIAS(GPIO_FUNC_IN  , "in");
  ADD_ALIAS(GPIO_FUNC_OUT , "output");
  ADD_ALIAS(GPIO_FUNC_OUT , "out");
  ADD_ALIAS(GPIO_FUNC_ALT_0, "alt0");
  ADD_ALIAS(GPIO_FUNC_ALT_1, "alt1");
  ADD_ALIAS(GPIO_FUNC_ALT_2, "alt2");
  ADD_ALIAS(GPIO_FUNC_ALT_3, "alt3");
  ADD_ALIAS(GPIO_FUNC_ALT_4, "alt4");
  ADD_ALIAS(GPIO_FUNC_ALT_5, "alt5");
  ADD_ALIAS(GPIO_FUNC_ALT_0, "0");
  ADD_ALIAS(GPIO_FUNC_ALT_1, "1");
  ADD_ALIAS(GPIO_FUNC_ALT_2, "2");
  ADD_ALIAS(GPIO_FUNC_ALT_3, "3");
  ADD_ALIAS(GPIO_FUNC_ALT_4, "4");
  ADD_ALIAS(GPIO_FUNC_ALT_5, "5");
}

static int command_gpio_print_help()
{
  puts("gpio enable GPIO\n");
  puts("\tEnables gpio pin with index GPIO\n");
  puts("gpio set GPIO VALUE\n");
  puts("\tSets gpio pin with index GPIO to a value of VALUE\n");
  puts("gpio set-pullupdown GPIO VALUE - set pull-up/pull-down mode for pin.\n");
  puts("\t                       GPIO  - pin index\n");
  puts("\t                       VALUE - pullup, pulldown, no\n");
  puts("gpio get-functions GPIO - list functions for a selected gpio pin\n");
  puts("gpio set-function  GPIO FUNCTION - set function for a selected gpio pin\n");
  return CMD_ERR_NO_ERROR;
}

static int command_gpio_set_function(const string_tokens_t *args)
{
  int pin_idx, i, status;
  char *endptr;
  const string_token_t *funcarg;

  ASSERT_NUMARGS_EQ(2);
  GET_PIN_IDX();
  funcarg = &args->ts[1];

  for (i = 0; i < gpio_aliases.len; ++i) {
    if (string_token_eq(funcarg, gpio_aliases.a[i].alias)) {
      puts("gpio set-function: ");
      status = gpio_set_function(pin_idx, gpio_aliases.a[i].value);
      if (status) {
        printf("gpio_set_functions failed with error: %d\n", status);
        return CMD_ERR_EXECUTION_ERR;
      }
      return CMD_ERR_NO_ERROR;
    }
  }
  return CMD_ERR_INVALID_ARGS;
}

static int command_gpio_get_functions(const string_tokens_t *args) 
{
  int pin_idx, i;
  char *endptr;
  ASSERT_NUMARGS_EQ(1);
  GET_PIN_IDX();
  putc('\r');
  putc('\n');
  for (i = 0; i < 6; ++i)
    printf("\tgpio%d/alt%d: %s\n", pin_idx, i, GET_GPIOFN(pin_idx, i)->name);
  return CMD_ERR_NO_ERROR;
}

static int command_gpio_set(const string_tokens_t *args)
{
  int pin_idx, pin_value;
  char *endptr;

  ASSERT_NUMARGS_EQ(2);
  GET_PIN_IDX()
  GET_NUMERIC_PARAM(pin_value, int, 1, "pin_value");

  if (pin_value != 0 && pin_value != 1) {
    puts("invalid param for pin_value. must be 0 or 1\n");
    return CMD_ERR_INVALID_ARGS;
  }

  if (pin_value)
    gpio_set_on(pin_idx);
  else
    gpio_set_off(pin_idx);

  printf("success: pin%d is set to %d\n", pin_idx, pin_value);
  return CMD_ERR_NO_ERROR;
}

static int command_gpio_set_pullupdown(const string_tokens_t *args)
{
  int pin_idx, pullup_val;
  char *endptr;

  ASSERT_NUMARGS_EQ(2);
  GET_PIN_IDX();

  if      (string_token_eq(&args->ts[1], "no"))
    pullup_val = GPIO_PULLUPDOWN_NO_PULLUPDOWN;
  else if (string_token_eq(&args->ts[1], "pulldown"))
    pullup_val = GPIO_PULLUPDOWN_EN_PULLDOWN;
  else if (string_token_eq(&args->ts[1], "pullup"))
    pullup_val = GPIO_PULLUPDOWN_EN_PULLUP;
  else {
    puts("gpio set_pullupdown wrong value argument. Should be 'pullup', 'pulldown' or 'no'\n");
    return CMD_ERR_INVALID_ARGS;
  }

  if (gpio_set_pullupdown(pin_idx, pullup_val)) {
    puts("gpio_set_pullupdown failed\n");
    return CMD_ERR_EXECUTION_ERR;
  }
    

  printf("success: pin%d is set to %d\n", pin_idx, pullup_val);
  return CMD_ERR_NO_ERROR;
}

int command_gpio(const string_tokens_t *args)
{
  string_tokens_t subargs;
  string_token_t *subcmd_token;
  puts("gpio: \n");
  if (gpio_functions_set != 1) {
    command_gpio_init_functions();
    gpio_functions_set = 1;
  }

  ASSERT_NUMARGS_GE(2);

  subcmd_token = &args->ts[1];
  subargs.ts  = subcmd_token + 1;
  subargs.len = args->len - 2;

  if (string_token_eq(subcmd_token, "help"))
    return command_gpio_print_help();
  if (string_token_eq(subcmd_token, "set"))
    return command_gpio_set(&subargs);
  if (string_token_eq(subcmd_token, "get-functions"))
    return command_gpio_get_functions(&subargs);
  if (string_token_eq(subcmd_token, "set-function"))
    return command_gpio_set_function(&subargs);
  if (string_token_eq(subcmd_token, "set-pullupdown"))
    return command_gpio_set_pullupdown(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(gpio, "controls gpio");
