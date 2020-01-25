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
.align 5
GLOBAL_VAR(mbox_buffer):
.rept 36 * 4
.byte 0
.endr

.text
FUNC(_mbox_call):
  # x0 - mbox channel
  # First exhaust all remaining reads
  # in mailbox read port
  mov w2, MBOX_STATUS_EMPTY
1: 
  ldr x1, =MBOX_RD_STATUS
  ldr w1, [x1]
  cmp w1, w2
  beq 1f
  ldr x1, =MBOX_RD
  ldr x1, [x1]
  b 1b
  
1:
  # Now let's wait until mailbox write port
  # is not full
#  ldr x1, =MBOX_WR_STATUS
#  mov w2, MBOX_STATUS_FULL
1:
#  ldr w3, [x1]
#  cmp w2, w3 
#  beq 1b
  
  # We are ready to write our buffer address to
  # property channel now
  
  # Add property from x0 to buffer address#
  ldr x1, =mbox_buffer
  orr x0, x1, x0
  # Modify address to point to bus address, 
  # Should be done as explained in 
  # https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
 # mov w1, BUS_ADDR_OFFSET
 # orr w0, w1, w0
  # Do write
  ldr x1, =MBOX_WR
  str w0, [x1]
  # Leave w0 as it should match the one
  # we recieve from read  
1:
  # Wait until response is ready
  ldr x1, =MBOX_RD_STATUS
  mov w2, MBOX_STATUS_EMPTY
2:
  ldr w3, [x1]
  cmp w3, w2
  beq 2b
  
  # If read is ready for us, then read port should
  # hold same address | channel value that we've put
  # in write port
  ldr x1, =MBOX_RD
  ldr w1, [x1]
  cmp w1, w0
  bne 1b
   
  bic x0, x0, 0xf
  mov w1, BUS_ADDR_OFFSET
  bic w0, w0, w1
  ldr w0, [x0, #4]
  mov w1, MBOX_RESPONSE
  cmp w1, w0 
  cset w0, ne
  ret
