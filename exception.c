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
  int el_num;
  int print_level;

  el_num = e->esr & 0x3;
  dfsc = (int)e->esr & 0x0000003f;
  print_level = 0;

  switch(dfsc & 0x3c) {
      case 0b000000: puts("Address size fault", 0, 1); print_level = 1; break;
      case 0b000100: puts("Translation fault", 0, 1);  print_level = 1; break;
      case 0b001000: puts("Access flag fault", 0, 1);  print_level = 1; break;
      case 0b001100: puts("Permission fault", 0, 1);   print_level = 1; break;
      case 0b010000: puts("Synchronous External abort, not table walk", 0, 1); break;
      case 0b010001: puts("Synchronous Tag Check", 0, 1); break;
      case 0b010100: puts("Synchronous External abort, on table walk", 0, 1); 
        print_level = 1; 
        break;
      case 0b011000: puts("Synchronous parity or ECC error on memory access, not on table walk", 0, 1);
        break;
      case 0b011100: puts("Synchronous parity or ECC error on memory access, on table walk", 0, 1);
        print_level = 1;
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
//    if (print_level) {
//      switch(el_num) {
//        case 0: puts(" at level 0 "); break;
//        case 1: puts(" at level 1 "); break;
//        case 2: puts(" at level 2 "); break;
//        case 3: puts(" at level 3 "); break;
//      }
//    }
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

static void __handle_interrupt_synchronous(exception_info_t *e)
{
  /* exception class */
  uint8_t ec;
  char buf[256];

  ec = (e->esr >> 26) & 0xff; 

  sprintf(buf, "class: %s, esr: %08x, elr: %08x, sp now at: %08x", 
      get_exception_class_string(ec), 
      (int)e->esr, 
      (int)e->elr, 
      &buf[0]);
  puts(buf, 0, 1);
  dump_stack(e->stack, 40, 0, 2);
  wait_msec(5000);
  dump_exception_ctx(e, 60, 3);
  while(1);


  switch (ec) {
    case EXC_CLASS_DATA_ABRT_LO_EL:
    case EXC_CLASS_DATA_ABRT_EQ_EL:
      __handle_data_abort(e);
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

void __handle_interrupt(exception_info_t *e)
{
  char buf[256];

  p_cpu_ctx = e->cpu_ctx;

  sprintf(buf, "interrupt: t: %s", get_interrupt_type_string(e->type));
  puts(buf, 0, 0);
  dump_exception_ctx(e, 60, 3);

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
}
