#include <asm_macros.h>
#include <memory.h>

.equ UART0_BASE, (PERIPHERAL_ADDR_RANGE_START + 0x00201000)
.equ UART0_DR  , (UART0_BASE + 0x00)
.equ UART0_FR  , (UART0_BASE + 0x18)

  /* Transmit FIFO full*/
.equ UART0_FR_TXFF, (1<<5)

.text

.globl pl011_uart_putchar
pl011_uart_putchar:
#x0 & 0xff - char
  stp   x29, x30, [sp, #-0x10]!
1:
  reg_set_addr32 x1, UART0_FR
  ldr   x1, [x1]
  dsb   sy
  tst   x1, UART0_FR_TXFF
  bne   1b
  reg_set_addr32 x1, UART0_DR
  str   w0, [x1]
  ldp   x29, x30, [sp], #0x10
  ret
  
.globl pl011_uart_getchar
pl011_uart_getchar:
  stp   x29, x30, [sp, #-0x10]!
1:
  reg_set_addr32 x1, UART0_FR
  ldr   x1, [x1]
  dsb   sy
  tst   x1, UART0_FR_TXFF
  bne   1b
  reg_set_addr32 x1, UART0_DR
  ldr   w0, [x1]
  ldp   x29, x30, [sp], #0x10
  ret
