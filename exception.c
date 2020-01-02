#include <exception.h>
#include <uart/uart.h>
#include <stringlib.h>
#include <vcanvas.h>
#include <delays.h>
#include <cpu.h>

#define INTERRUPT_TYPE_SYNCHRONOUS 0
#define INTERRUPT_TYPE_IRQ         1
#define INTERRUPT_TYPE_FIQ         2
#define INTERRUPT_TYPE_SERROR      3


static void *p_cpu_ctx;
static char cpu_ctx_buf[2048];

/* exception level to control nested entry to exception code */
static int elevel = 0;

typedef struct printer {
  int x;
  int y;
} printer_t;

static printer_t printer;

void printer_puts(printer_t *p, const char *str, int x, int y)
{
  uart_puts(str);
  vcanvas_puts(&x, &y, str);
}

void printer_putc(printer_t *p, char c, int x, int y)
{
  uart_putc(c);
  vcanvas_putc(&x, &y, c);
}

#define puts(str, x, y) printer_puts(&printer, str, x, y)
#define putc(c, x, y) printer_putc(&printer, c, x, y)

void generate_exception()
{
  *(volatile long int*)(0xffffffffffffffff) = 1;
}

void kernel_panic(const char *msg)
{
  generate_exception();
}

irq_cb_t irq_cb = 0;
irq_cb_t fiq_cb = 0;

void set_irq_cb(irq_cb_t cb)
{
  irq_cb = cb;
}

void set_fiq_cb(irq_cb_t cb)
{
  fiq_cb = cb;
}

static inline void exception_panic(const char *msg)
{
  puts("exception_panic: ", 0, 0);
  puts(msg, 0, 1);
  while(1);
}

static void dump_stack(uint64_t *stack, int depth, int x, int y)
{
  char buf[64];
  int i;

  sprintf(buf, "stack at: %016x", stack);
  puts(buf, x, y++);

  for (i = 0; i < depth; ++i) {
     sprintf(buf, "%08x: %016x", stack + i, *(stack + i));
     puts(buf, x, y++);
  }
}

typedef struct bin_data_header {
  char magic[8];
  uint32_t crc;
  uint32_t len;
} packed bin_data_header_t;

typedef struct bin_stack_header {
  char magic[8];
  uint32_t crc;
  uint32_t len;
  uint64_t stack_addr;
} packed bin_stack_header_t;

uint32_t count_crc(void *src, int size, uint32_t init_crc)
{
  int i;
  uint32_t result;
  result = init_crc;
  for (i = 0; i < size; ++i)
    result += ((const char *)src)[i];
  return result;
}

#define MAGIC_STACK "STACK"
#define MAGIC_BINBLOCK "BINBLOCK"

static void dump_stack_to_uart(void *stack, int num_recs)
{
  bin_stack_header_t sh = { 0 };
  bin_data_header_t h = { 0 };

  memcpy(sh.magic, MAGIC_STACK, sizeof(MAGIC_STACK));
  sh.len = sizeof(uint64_t) * num_recs;
  sh.stack_addr = (uint64_t)stack;
  sh.crc = count_crc(stack, sh.len, 0);

  memcpy(h.magic, MAGIC_BINBLOCK, sizeof(MAGIC_BINBLOCK));
  h.len = sizeof(sh) + sh.len;
  h.crc = count_crc(&sh, sizeof(sh.crc), sh.crc);

  uart_send_buf(&h, sizeof(h));
  uart_send_buf(&sh, sizeof(sh));
  uart_send_buf(stack, sizeof(uint64_t) * num_recs);
}

static void dump_exception_ctx(exception_info_t *e, int x, int y)
{
  int stop;
  char *p1, *p2;
  int n;
  p1 = cpu_ctx_buf;
  p2 = p1;
  stop = 0;

  n = cpu_dump_ctx(e->cpu_ctx, cpu_ctx_buf, sizeof(cpu_ctx_buf));

  if (n >= sizeof(cpu_ctx_buf)) {
    puts("Failed to dump cpu context.\n", 0, 0);
    while(1);
  }

  while(!stop) {
    while(*p2 && *p2 != '\n') p2++;
    if (*p2 == 0)
      stop = 1;

    *(p2++) = 0;
    puts(p1, x, y++);
    p1 = p2;
  }
}


const char *get_interrupt_type_string(int interrupt_type) {
  switch(interrupt_type) {
    case INTERRUPT_TYPE_SYNCHRONOUS: return "Synchronous"; 
    case INTERRUPT_TYPE_IRQ        : return "IRQ";         
    case INTERRUPT_TYPE_FIQ        : return "FIQ";         
    case INTERRUPT_TYPE_SERROR     : return "SError";      
    default                        : return "Undefined";    
  }
}

// decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
#define EXC_CLASS_UNKNOWN           0b000000
#define EXC_CLASS_TRAPPED_WFI_WFE   0b000001
#define EXC_CLASS_ILLEGAL_EXECUTION 0b001110
#define EXC_CLASS_SYSTEM_CALL       0b010101
#define EXC_CLASS_INST_ABRT_LO_EL   0b100000
#define EXC_CLASS_INST_ABRT_EQ_EL   0b100001
#define EXC_CLASS_INST_ALIGNMENT    0b100010
#define EXC_CLASS_DATA_ABRT_LO_EL   0b100100
#define EXC_CLASS_DATA_ABRT_EQ_EL   0b100101
#define EXC_CLASS_STACK_ALIGN_FLT   0b100110
#define EXC_CLASS_FLOATING_POINT    0b101100

const char *get_exception_class_string(uint8_t exception_class) 
{
  switch(exception_class) {
    case EXC_CLASS_UNKNOWN          : return "Unknown";
    case EXC_CLASS_TRAPPED_WFI_WFE  : return "Trapped WFI/WFE";
    case EXC_CLASS_ILLEGAL_EXECUTION: return "Illegal execution";
    case EXC_CLASS_SYSTEM_CALL      : return "System call";
    case EXC_CLASS_INST_ABRT_LO_EL  : return "Instruction abort, lower EL";
    case EXC_CLASS_INST_ABRT_EQ_EL  : return "Instruction abort, same EL";
    case EXC_CLASS_INST_ALIGNMENT   : return "Instruction alignment fault";
    case EXC_CLASS_DATA_ABRT_LO_EL  : return "Data abort, lower EL";
    case EXC_CLASS_DATA_ABRT_EQ_EL  : return "Data abort, same EL";
    case EXC_CLASS_STACK_ALIGN_FLT  : return "Stack alignment fault";
    case EXC_CLASS_FLOATING_POINT   : return "Floating point";
    default                         : return "Unknown2";
  }
}

static void __handle_data_abort(exception_info_t *e)
{
  int dfsc;

  // el_num = e->esr & 0x3;
  dfsc = (int)e->esr & 0x0000003f;

  switch(dfsc & 0x3c) {
      case 0b000000: puts("Address size fault", 0, 1); break;
      case 0b000100: puts("Translation fault", 0, 1);  break;
      case 0b001000: puts("Access flag fault", 0, 1);  break;
      case 0b001100: puts("Permission fault", 0, 1);   break;
      case 0b010000: puts("Synchronous External abort, not table walk", 0, 1); break;
      case 0b010001: puts("Synchronous Tag Check", 0, 1); break;
      case 0b010100: puts("Synchronous External abort, on table walk", 0, 1); 
        break;
      case 0b011000: puts("Synchronous parity or ECC error on memory access, not on table walk", 0, 1);
        break;
      case 0b011100: puts("Synchronous parity or ECC error on memory access, on table walk", 0, 1);
        break;
      default:
        switch(dfsc) {
          case 0b100001: puts("Alignment fault", 0, 1); break;
          case 0b110001: puts("Unsup atomic hardware update fault", 0, 1); break;
          case 0b110000: puts("TLB conflict abort", 0, 1); break;
          case 0b111101: puts("Section Domain Fault", 0, 1); break;
          case 0b111110: puts("Page Domain Fault", 0, 1); break;
          default: puts("Undefined Data abort code", 0, 1); break;
        }
    }
//    if (e->esr & 0b1000000) {
//      puts(", write op");
//    }
//    else {
//      puts(", read op");
//    }
//    if (e->esr & (1 << 24)) {
//      puts(", ISV valid");
//    }
//    if (e->esr & (1 << 15)) {
//      puts(", 64 bit");
//    }
//    else {
//      puts(", 32 bit");
//    }
    dump_exception_ctx(e, 0, 4);
}

static void __handle_instr_abort(exception_info_t *e, uint64_t elr)
{
  puts("instruction abort handler", 10, 9);
  dump_exception_ctx(e, 10, 10);
}

static int inline __get_synchr_exception_class(uint64_t esr)
{
  return (esr >> 26) & 0xff; 
}

static void __handle_interrupt_synchronous(exception_info_t *e)
{
  /* exception class */
  uint8_t ec;
  char buf[256];

  ec = __get_synchr_exception_class(e->esr);

  sprintf(buf, "class: %s, esr: %08x, elr: %08x, far: %08x, sp now at: %08x", 
      get_exception_class_string(ec), 
      (int)e->esr, 
      (int)e->elr, 
      (int)e->far,
      &buf[0]);
  puts(buf, 0, 1);
  dump_stack(e->stack, 30, 160, 0);
  dump_stack_to_uart(e->stack, 100);
  while(1);

  switch (ec) {
    case EXC_CLASS_DATA_ABRT_LO_EL:
    case EXC_CLASS_DATA_ABRT_EQ_EL:
      __handle_data_abort(e);
      while(1);
      break;
    case EXC_CLASS_INST_ABRT_LO_EL:
    case EXC_CLASS_INST_ABRT_EQ_EL:
      __handle_instr_abort(e, e->elr);
      break;
    case EXC_CLASS_INST_ALIGNMENT:
      puts("instruction alignment handler", 0, 0);
      break;
    default:
      break;
  }
}


static void __handle_interrupt_serror()
{
}

static void __handle_interrupt_irq()
{
  if (irq_cb)
    irq_cb();
}

static void __print_exception_info(exception_info_t *e, int elevel) {
  int x;
  int y;
  char buf[256];

  x = 100;
  y = 0;
  snprintf(buf, 256, "interrupt: l: %d, t: %s", elevel, get_interrupt_type_string(e->type));
  puts(buf, x, y + 2 * (elevel - 1));
  y += 3;
  dump_exception_ctx(e, x, y + 17 * (elevel - 1));
}

void __handle_interrupt(exception_info_t *e)
{
  elevel++;

  dump_stack_to_uart(e->stack, 100);
  p_cpu_ctx = e->cpu_ctx;
  __print_exception_info(e, elevel);

  switch (e->type) {
    case INTERRUPT_TYPE_IRQ:
      __handle_interrupt_irq();
      break;
    case INTERRUPT_TYPE_FIQ:
      if (fiq_cb)
        fiq_cb();
      break;
    case INTERRUPT_TYPE_SYNCHRONOUS:
      __handle_interrupt_synchronous(e);
      break;
    case INTERRUPT_TYPE_SERROR:
      __handle_interrupt_serror();
      break;
  } 
  elevel--;
}
