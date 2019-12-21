#include <exception.h>
#include <uart/uart.h>
#include <stringlib.h>
#include <vcanvas.h>
#include <delays.h>

#define INTERRUPT_TYPE_SYNCHRONOUS 0
#define INTERRUPT_TYPE_IRQ         1
#define INTERRUPT_TYPE_FIQ         2
#define INTERRUPT_TYPE_SERROR      3

#define puts(txt) \
  uart_puts(txt); vcanvas_puts(x, y, txt);

#define putc(c) \
  uart_putc(c); vcanvas_putc(x, y, c);

void generate_exception()
{
  *(volatile long int*)(0xffffffffffffffff) = 1;
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

void __handle_interrupt_synchronous(unsigned long esr, int *x, int *y)
{
  uint8_t exception_class;
  int el_num;

  exception_class = (esr >> 26) & 0xff; 
  el_num = esr & 0x3;
  puts("exception class: ");
  puts(get_exception_class_string(exception_class));
  putc('\n');

  // decode data abort cause
  if (exception_class == EXC_CLASS_DATA_ABRT_LO_EL 
      || exception_class == EXC_CLASS_DATA_ABRT_EQ_EL) {
    puts(", ");
    int dfsc = (int)esr & 0x0000003f;
    int print_level = 0;
    switch(dfsc & 0x3c) {
      case 0b000000: puts("Address size fault"); print_level = 1; break;
      case 0b000100: puts("Translation fault");  print_level = 1; break;
      case 0b001000: puts("Access flag fault");  print_level = 1; break;
      case 0b001100: puts("Permission fault");   print_level = 1; break;
      case 0b010000: puts("Synchronous External abort, not table walk"); break;
      case 0b010001: puts("Synchronous Tag Check"); break;
      case 0b010100: puts("Synchronous External abort, on table walk"); 
        print_level = 1; 
        break;
      case 0b011000: puts("Synchronous parity or ECC error on memory access, not on table walk");
        break;
      case 0b011100: puts("Synchronous parity or ECC error on memory access, on table walk");
        print_level = 1;
        break;
      default:
        switch(dfsc) {
          case 0b100001: puts("Alignment fault"); break;
          case 0b110001: puts("Unsup atomic hardware update fault"); break;
          case 0b110000: puts("TLB conflict abort"); break;
          case 0b111101: puts("Section Domain Fault"); break;
          case 0b111110: puts("Page Domain Fault"); break;
          default: puts("Undefined Data abort code"); break;
        }
    }
    if (print_level) {
      switch(el_num) {
        case 0: puts(" at level 0 "); break;
        case 1: puts(" at level 1 "); break;
        case 2: puts(" at level 2 "); break;
        case 3: puts(" at level 3 "); break;
      }
    }
    if (esr & 0b1000000) {
      puts(", write op");
    }
    else {
      puts(", read op");
    }
    if (esr & (1 << 24)) {
      puts(", ISV valid");
    }
    if (esr & (1 << 15)) {
      puts(", 64 bit");
    }
    else {
      puts(", 32 bit");
    }
  }
}

void __handle_interrupt_serror()
{
}

void __handle_interrupt(
  unsigned long type, 
  unsigned long esr, 
  unsigned long elr, 
  unsigned long spsr, 
  unsigned long far)
{
  char buf[64];
  int cx = 40; 
  int cy = 0;
  int *x = &cx;
  int *y = &cy; 
  return;

  puts("interrupt: type: ");
  puts(get_interrupt_type_string(type));
  puts(": ");
  switch (type) {
    case INTERRUPT_TYPE_IRQ:
      if (irq_cb)
        irq_cb();
      break;
    case INTERRUPT_TYPE_FIQ:
      if (fiq_cb)
        fiq_cb();
      break;
    case INTERRUPT_TYPE_SYNCHRONOUS:
      __handle_interrupt_synchronous(esr, x, y);
      break;
    case INTERRUPT_TYPE_SERROR:
      __handle_interrupt_serror();
      break;
  } 

  // dump registers
  sprintf(buf, "esr: %08x", (int)esr);
  *x = 40, *y = 1;
  vcanvas_puts(x, y, buf);
  sprintf(buf, "elr: %08x", (int)elr);
  *x = 40, *y = 2;
  vcanvas_puts(x, y, buf);
  sprintf(buf, "spsr: %08x", (int)spsr);
  *x = 40, *y = 3;
  vcanvas_puts(x, y, buf);
  sprintf(buf, "far: %08x", (int)far);
  *x = 40, *y = 4;
  vcanvas_puts(x, y, buf);
}
