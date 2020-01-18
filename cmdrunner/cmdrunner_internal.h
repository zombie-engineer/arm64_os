#pragma once

typedef struct cmdrunner_state {
  char escbuf[8];
  int escbuflen;
} cmdrunner_state_t;

void cmdrunner_state_init(cmdrunner_state_t *s);
int cmdrunner_handle_char(cmdrunner_state_t *s, char c);
