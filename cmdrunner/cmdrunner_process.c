#include <cmdrunner.h>
#include "cmdrunner_internal.h"
#include <types.h>
#include <spinlock.h>


//typedef struct uart_worker {
//  int initialized;
//} uart_worker_t;
//
//static uart_worker_t uart_worker = {
//  .initialized = 0,
//};

extern uint64_t pipe_lock;

static char cmdrunner_getch()
{
  while(1) {
    spinlock_lock(&pipe_lock);
    asm volatile ("wfi"); 
  }
  return '0';
}

int cmdrunner_process(int argc, char *argv[])
{
  char c;
  cmdrunner_state_t s;
  cmdrunner_state_init(&s);

  while(1) {
    c = cmdrunner_getch();
    cmdrunner_handle_char(&s, c);
  }

  return 0;
}
