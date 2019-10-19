#pragma once

#define ASSERT_NUMARGS_EQ(n) \
  if (args->len != n) { \
    printf("invalid number of args: expected %d, have: %d\n", n, args->len); \
    return CMD_ERR_INVALID_ARGS; \
  }

#define ASSERT_NUMARGS_GE(n) \
  if (args->len < n) { \
    printf("invalid number of args: expected not less than %d, have: %d\n", n, args->len); \
    return CMD_ERR_INVALID_ARGS; \
  }

#define GET_NUMERIC_PARAM(dst, typ, argnum, desc) \
  dst = (typ)strtoll(args->ts[argnum].s, &endptr, 0); \
  if (args->ts[argnum].s == endptr) { \
    printf("failed to get numeric param '%s' from argument number %d\n", desc, argnum); \
    return CMD_ERR_INVALID_ARGS; \
  }

#define DECL_ARGS_CTX()           \
  string_tokens_t subargs;        \
  string_token_t *subcmd_token;   \
  subcmd_token = &args->ts[0];    \
  subargs.ts  = subcmd_token + 1; \
  subargs.len = args->len - 1     \

