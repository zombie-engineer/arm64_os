#include <asm_macros.h>
#include <memory.h>

.equ MBOX_BASE, (PERIPHERAL_ADDR_RANGE_START + 0xb880)
.equ MBOX_RD,       (MBOX_BASE + 0x00)
.equ MBOX_RD_PEEK,  (MBOX_RD + 0x10)
.equ MBOX_RD_SENDER,(MBOX_RD + 0x14)
.equ MBOX_RD_STATUS,(MBOX_RD + 0x18)
.equ MBOX_RD_CONFIG,(MBOX_RD + 0x1c)
.equ MBOX_WR,       (MBOX_BASE + 0x20)
.equ MBOX_WR_PEEK,  (MBOX_WR + 0x10)
.equ MBOX_WR_SENDER,(MBOX_WR + 0x14)
.equ MBOX_WR_STATUS,(MBOX_WR + 0x18)
.equ MBOX_WR_CONFIG,(MBOX_WR + 0x18)

.equ MBOX_STATUS_FULL, 0x80000000
.equ MBOX_STATUS_EMPTY, 0x40000000
.equ MBOX_RESPONSE, 0x80000000
.equ BUS_ADDR_OFFSET, RAM_BASE_BUS_CACHED

.data
/* Buffer is used to pass data from VC to ARM and back
 * it should be aligned a little bit, so that it's 4 lsb bits were 
 * free
 */
.align 4
GLOBAL_VAR(mbox_buffer):
.rept 36 * 4
.byte 0
.endr

.text

/* Refer to this document to understand the logic of this function 
 * https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
 */

FUNC(mbox_call_blocking):
  stp   x29, x30, [sp, #-0x10]!
  stp   x1, x2, [sp, #-0x10]!
  /* Combine buffer address with channel number
   * mbox_buffer addr is aligned, so it can be combined with
   * a 4 bits property channel number
   */
  ldr   x1, =mbox_buffer  
  and   w0, w0, #0xf
  orr   w1, w1, w0

1: /* wait until NOT FULL loop */
  /* Write to mailbox, when write status flag not FULL */
  reg_set_addr32 x0, MBOX_WR_STATUS
  ldr   w0, [x0]
  ands  w0, w0, MBOX_STATUS_FULL
  bne   1b

  reg_set_addr32 x0, MBOX_WR
  str   w1, [x0]

1: /* wait until NOT EMPTY loop */
  reg_set_addr32 x0, MBOX_RD_STATUS
  ldr   w0, [x0]
  ands  w0, w0, MBOX_STATUS_EMPTY
  bne   1b

  /* Check that this response is ours, 
   * if not report error
   */
  reg_set_addr32 x0, MBOX_RD
  ldr   w0, [x0]
  cmp   w0, w1
  mov   w0, #0
  beq   1f
  mov  w0, #1
1:
  ldp   x1, x2, [sp], #0x10
  ldp   x29, x30, [sp], #0x10
  ret
