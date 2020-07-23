#pragma once

typedef struct string_token {
  const char *s;
  int len;
} string_token_t;

typedef struct string_tokens {
  string_token_t *ts;
  int len;
} string_tokens_t;

int string_token_eq(const string_token_t *t, const char *str);

#define STRING_TOKENS_LOOP(t, var) \
  string_token_t *var = t->ts; \
  for(; var < &t->ts[t->len]; ++var)

int tokenize_string(const char *start, const char *end, int max_tokens, string_tokens_t *out);
