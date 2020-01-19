#include <cmdrunner.h>
#include "cmdrunner_internal.h"
#include <types.h>
#include <spinlock.h>
#include <ringbuf.h>


//typedef struct uart_worker {
//  int initialized;
//} uart_worker_t;
//
//static uart_worker_t uart_worker = {
//  .initialized = 0,
//};

extern ringbuf_t uart_pipe;
extern uint64_t uart_pipe_lock;

//static char cmdrunner_getch()
//{
//  int n;
//  char c;
//
//  while(1) {
//    spinlock_lock(&uart_pipe_lock);
//    n = ringbuf_read(&uart_pipe, &c, 1);
//    spinlock_unlock(&uart_pipe_lock);
//    if (n)
//      break;
//    asm volatile ("wfi"); 
//  }
//  return c;
//}

int cmdrunner_process(int argc, char *argv[])
{
  char c;
  cmdrunner_state_t s;
  cmdrunner_state_init(&s);

  while(1) {
    asm volatile ("wfe");

//    c = cmdrunner_getch();
//    pl011_uart_send('&');
//    cmdrunner_handle_char(&s, c);
  }

  return 0;
}
