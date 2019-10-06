#include <cmdrunner.h>
#include <stdlib.h>
#include <gpio.h>
#include <common.h>

static int command_gpio_print_help()
{
  puts("gpio enable GPIO\n");
  puts("\tEnables gpio pin with index GPIO\n");
  puts("gpio set GPIO VALUE\n");
  puts("\tSets gpio pin with index GPIO to a value of VALUE\n");
  puts("gpio set-pullupdown GPIO VALUE\n");
  puts("\tSets pullup/down mode for gpio pin with index GPIO to a value of VALUE\n");
  return CMD_ERR_NO_ERROR;
}

static int command_gpio_set(const string_token_t *args, int numargs)
{
  int pin_idx, value;
  char *endptr;

  puts("action - gpio set\n");
  if (numargs != 2) {
    puts("gpio set - invalid number of args. Expected arg1 - pin idx, arg2 - value\n");
    return CMD_ERR_INVALID_ARGS;
  }

  pin_idx = (int)strtoll(args[0].s, &endptr, 0);
  if (args[0].s == endptr) {
    puts("gpio set - invalid param for arg1 pin idx\n");
    return CMD_ERR_INVALID_ARGS;
  }

  value = (int)strtoll(args[1].s, &endptr, 0);
  if (args[0].s == endptr) {
    puts("gpio set - invalid param for arg2 value\n");
    return CMD_ERR_INVALID_ARGS;
  }

  if (value != 0 && value != 1) {
    puts("gpio set - invalid param for arg2 value. must be 0 or 1\n");
    return CMD_ERR_INVALID_ARGS;
  }

  if (value)
    gpio_set_on(pin_idx);
  else
    gpio_set_off(pin_idx);

  return CMD_ERR_NO_ERROR;
}

int command_gpio(const string_tokens_t *args)
{
  puts("gpio: \n");

  if (args->len <= 1)
    return CMD_ERR_PARSE_ARG_ERROR;

  string_token_t *subcmd_token = &args->ts[1];

  if (string_token_eq(subcmd_token, "help"))
    return command_gpio_print_help();
  if (string_token_eq(subcmd_token, "set"))
    return command_gpio_set(subcmd_token + 1, args->len - 2);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(gpio, "controls gpio");
