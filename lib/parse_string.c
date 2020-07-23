#include <parse_string.h>
#include <stringlib.h>

int tokenize_string(const char *start, const char *end, int max_tokens, string_tokens_t *out)
{
  const char *ptr;
  struct string_token *t = out->ts;
  out->len = 0;

  ptr = start;

  while(out->len < max_tokens) {
    /* skip until first word */
    while(1) {
      if (ptr >= end || *ptr == 0)
        return 0;
      if (!isspace(*ptr))
        break;
      ptr++;
    }

    out->len++;
    t->s = ptr;
    while(1) {
      if (ptr >= end || *ptr == 0) {
        t->len = ptr - t->s;
        return 0;
      }
      if (isspace(*ptr)) {
        t->len = ptr - t->s;
        break;
      }
      ptr++;
    }
    t++;
  }

  return 0;
}
