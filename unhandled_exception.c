#include <unhandled_exception.h>
#include <stringlib.h>
#include <cpu.h>
#include <uart/uart.h>
#include <vcanvas.h>
#include <drivers/display/nokia5110.h>
#include "binblock.h"
#include <checksum.h>
#include <common.h>

#define FATAL_EXCEPTION_REPORTER_UART      "uart"
#define FATAL_EXCEPTION_REPORTER_VCANVAS   "vcanvas"
#define FATAL_EXCEPTION_REPORTER_NOKIA5110 "nokia5110"

/* Exception level set by interrupt_handler */
extern int elevel;

typedef int (*print_stack_cb)(uint64_t addr, uint64_t value, void *cb_priv);

static void print_stack_generic(uint64_t stack_top, int depth, print_stack_cb cb, void *cb_priv)
{
  uint64_t *ptr = (uint64_t *)stack_top;
  uint64_t value;
  int i;
  for (i = 0; i < depth; ++i) {
    value = *ptr;
    if (cb((uint64_t)ptr, value, cb_priv))
      break;
    ptr++;
  }
}

typedef struct print_stack_ctx {
  int nr;
} print_stack_ctx_t;

int print_stack_uart_cb(uint64_t addr, uint64_t value, void *cb_priv)
{
  char buf[64];
  print_stack_ctx_t *arg = (print_stack_ctx_t *)cb_priv;
  snprintf(buf, sizeof(buf), "0x%016llx: 0x%016llx\n", addr, value);
  uart_puts(buf);
  arg->nr++;
  return 0;
}

static void print_stack_uart(uint64_t stack_top, int depth)
{
  print_stack_ctx_t arg;
  memset(&arg, 0, sizeof(arg));
  print_stack_generic(stack_top, depth, print_stack_uart_cb, &arg); 
}

void unhandled_exception_print_summary_uart(exception_info_t *e)
{
  char buf[256];
  int n;
  n = gen_exception_string_generic(e, buf, sizeof(buf));
  uart_send_buf(buf, n);
  uart_send_buf("\n\r", 2);
  n = gen_exception_string_specific(e, buf, sizeof(buf));
  uart_send_buf(buf, n);
  uart_send_buf("\n\r", 2);
}

void unhandled_exception_print_summary_vcanvas(exception_info_t *e)
{
  char buf[64];
  int x, y;
  x = 0;
  y = 0;
  snprintf(buf, sizeof(buf), "%s", get_exception_type_string(e->type));

  vcanvas_puts(&x, &y, buf);
}

void unhandled_exception_print_summary_nokia5110(exception_info_t *e)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "E: %s", get_exception_type_string(e->type));
  nokia5110_draw_text(buf, 0, 0);
}


typedef struct print_cpuctx_ctx {
  int nr;
} print_cpuctx_ctx_t;


static int print_reg_cb(const char *reg_str, size_t reg_str_sz, void *cb_priv)
{
  print_cpuctx_ctx_t *print_ctx = (print_cpuctx_ctx_t *)cb_priv;
  if (print_ctx->nr) {
    if (print_ctx->nr % 4 == 0) {
      uart_putc('\n');
      uart_putc('\r');
    } else {
      uart_putc(',');
      uart_putc(' ');
    }
  }
  uart_puts(reg_str);
  print_ctx->nr++;
  return 0;
}

static void unhandled_exception_print_cpu_ctx_uart(exception_info_t *e)
{
  print_cpuctx_ctx_t print_ctx = { 0 };
  if (cpuctx_print_regs(e->cpu_ctx, print_reg_cb, &print_ctx))
  /* Nothing we can do but ignore error */;
  uart_putc('\n');
  uart_putc('\n');
  uart_putc('\r');
  print_stack_uart((uint64_t)(e->stack), 8);
}

void unhandled_exception_dump_cpu_ctx_uart(exception_info_t *e)
{
  int n;
  char binblock[2048];
  uint64_t sp_value;
  uart_puts("*EXCEPTION_CONTEXT_DUMP***********************");
  n = binblock_fill_exception(binblock, sizeof(binblock), e);
  if (n < 0) {
    uart_puts("Failed to generate exception binblock.\n");
    return;
  }

  if (binblock_send(binblock, n, BINBLOCK_ID_EXCEPTION, uart_send_buf) < 0) {
    uart_puts("Failed to send exception binblock.\n");
    return;
  }

  sp_value = cpuctx_get_sp(e->cpu_ctx);
  if (binblock_send((const void *)sp_value, 0x100, BINBLOCK_ID_STACK, uart_send_buf) < 0) {
    uart_puts("Failed to dump stack.");
  }

  uart_puts("*EXCEPTION_CONTEXT_DUMP_END*******************");
}

void unhandled_exception_print_cpu_ctx_vcanvas(exception_info_t *e)
{
}

void unhandled_exception_print_cpu_ctx_nokia5110(exception_info_t *e)
{
}

static unhandled_exception_reporter_t unhandled_exception_reporters[] = {
  {
    .name = FATAL_EXCEPTION_REPORTER_UART,
    .reporter_id = REPORTER_ID_UART_PL011,
    .enabled = 0,
    .print_summary = unhandled_exception_print_summary_uart,
    .print_cpu_ctx = unhandled_exception_print_cpu_ctx_uart,
    .dump_cpu_ctx = unhandled_exception_dump_cpu_ctx_uart,
  },
  {
    .name = FATAL_EXCEPTION_REPORTER_VCANVAS,
    .reporter_id = REPORTER_ID_VCANVAS,
    .enabled = 0,
    .print_summary = unhandled_exception_print_summary_vcanvas,
    .print_cpu_ctx = unhandled_exception_print_cpu_ctx_vcanvas,
    .dump_cpu_ctx = 0
  },
  {
    .name = FATAL_EXCEPTION_REPORTER_NOKIA5110,
    .reporter_id = REPORTER_ID_NOKIA5110,
    .enabled = 0,
    .print_summary = unhandled_exception_print_summary_nokia5110,
    .print_cpu_ctx = unhandled_exception_print_cpu_ctx_nokia5110,
    .dump_cpu_ctx = 0
  }
};

void init_unhandled_exception_reporters()
{
  int i;
  unhandled_exception_reporter_t *r;
  for (i = 0; i < ARRAY_SIZE(unhandled_exception_reporters); ++i) {
    r = &unhandled_exception_reporters[i];
    r->enabled = 1;
  }
}

void report_unhandled_exception(exception_info_t *e)
{
  int i;
  unhandled_exception_reporter_t *r;
  for (i = 0; i < ARRAY_SIZE(unhandled_exception_reporters); ++i) {
    r = &unhandled_exception_reporters[i];
    if (r->enabled) {
      if (r->print_summary)
        r->print_summary(e);
      if (r->print_cpu_ctx)
        r->print_cpu_ctx(e);
      if (r->dump_cpu_ctx)
        r->dump_cpu_ctx(e);
    }
  }
}

int enable_unhandled_exception_reporter(int reporter_id)
{
  int i;
  unhandled_exception_reporter_t *r;
  for (i = 0; i < ARRAY_SIZE(unhandled_exception_reporters); ++i) {
    r = &unhandled_exception_reporters[i];
    if (r->reporter_id == reporter_id) {
      r->enabled = 1;
      return ERR_OK;
    }
  }
  return ERR_NOT_FOUND;
}
