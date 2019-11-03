#include <uart/uart.h>
#include <stringlib.h>
#include <vcanvas.h>
#include <delays.h>


#define puts(txt) \
  uart_puts(txt); vcanvas_puts(&x, &y, txt);

void exc_handler(
  unsigned long type, 
  unsigned long esr, 
  unsigned long elr, 
  unsigned long spsr, 
  unsigned long far)
{
  char buf[64];
  int x = 40, y = 0; 
  // print out interruption type
  switch(type) {
    case 0: puts("Synchronous"); break;
    case 1: puts("IRQ"); break;
    case 2: puts("FIQ"); break;
    case 3: puts("SError"); break;
  }
  puts(": ");
  // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
  switch(esr >> 26) {
    case 0b000000: puts("Unknown"); break;
    case 0b000001: puts("Trapped WFI/WFE"); break;
    case 0b001110: puts("Illegal execution"); break;
    case 0b010101: puts("System call"); break;
    case 0b100000: puts("Instruction abort, lower EL"); break;
    case 0b100001: puts("Instruction abort, same EL"); break;
    case 0b100010: puts("Instruction alignment fault"); break;
    case 0b100100: puts("Data abort, lower EL"); break;
    case 0b100101: puts("Data abort, same EL"); break;
    case 0b100110: puts("Stack alignment fault"); break;
    case 0b101100: puts("Floating point"); break;
    default: puts("Unknown"); break;
  }
  // decode data abort cause
  if (esr >> 26 == 0b100100 || esr >> 26 == 0b100101) {
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
      switch(esr & 0x3) {
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

  // dump registers
  sprintf(buf, "esr: %08x", (int)esr);
  x = 40, y = 1;
  vcanvas_puts(&x, &y, buf);
  sprintf(buf, "elr: %08x", (int)elr);
  x = 40, y = 2;
  vcanvas_puts(&x, &y, buf);
  sprintf(buf, "spsr: %08x", (int)spsr);
  x = 40, y = 3;
  vcanvas_puts(&x, &y, buf);
  sprintf(buf, "far: %08x", (int)far);
  x = 40, y = 4;
  vcanvas_puts(&x, &y, buf);
  return;

  uart_hex(esr>>32);
  uart_hex(esr);
  puts(" ELR_EL1 ");
  uart_hex(elr>>32);
  uart_hex(elr);
  puts("\n SPSR_EL1 ");
  uart_hex(spsr>>32);
  uart_hex(spsr);
  puts(" FAR_EL1 ");
  uart_hex(far>>32);
  uart_hex(far);
  puts("\n");
  // no return from exception for now
  while(1);
}
