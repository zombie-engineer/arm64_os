
/*
 * _read_and_set - first read the whole register then OR (*reg |= 1<<pin).
 *                 For registers that will disable pins where bits are clear.
 * _set_no_read  - just write the pin, don't read first.
 *                 For registers that will only apply to pins, which are set
 *                 in the value.
 * _64           - register address provided is the first register in the pair
 *                 of 32bit registers. pin number value can be 0-64, so for
 *                 pin numbers over 31 next register will be used.
 *                  (*(reg+4) = pin_nr - 32)
 * _3            - register address provided is the first register in series
 *                 of GPIO registers, with 3 bits per pin.
 *
 */

/* 
 * gpio_read_and_set_64(uin32_t *base_reg_addr, uint32_t pin_nr)
 * x0 - base_reg_addr
 * w1 - pin_nr
 */
.globl gpio_read_and_set_64
gpio_read_and_set_64:
  /*
   * addr += 4 * ((pin_nr / 32) & 1)
   */
  mov w2, w1, lsr 5
  and w2, w2, #1
  add x0, x0, x2, lsl 2

  /*
   *  OR_value = 1 << (pin_nr % 32)
   */
  mov w2, #1
  and w1, w1, #0x1f
  lsl w1, w2, w1

  /*
   * *addr |= OR_value 
   */
  ldr w3, [x0]
  orr w3, w3, w1
  str w3, [x0]
ret

.macro div_by_10, regid, gpion, wtmp
  mov   \wtmp, #0xcccd
  movk  \wtmp, #0xcccc, lsl #16
  umull \regid, \gpion, \wtmp
  lsr   \regid, \regid, #35
.endm

/* 
 * gpio_read_and_set_3(uin32_t *base_reg_addr, uint32_t pin_nr, uint32_t mode)
 * x0 - base_reg_addr
 * w1 - pin_nr
 * w2 - mode
 */
.globl gpio_read_and_set_3
gpio_read_and_set_3:
regaddr .req x0
gpionr  .req w1
mode    .req w2
wtmp    .req w3
  /* Select the right register */
  /* reg offset = gpio_num / 10 */
  div_by_10 x4, gpionr, wtmp
.unreq wtmp

regidx .req x4
  add   regaddr, regaddr, regidx, lsl #2
.unreq regidx
regidx .req w4
  /* gpionr = gpionr % 10 = gpionr - (gpionr * 10) */
  add   regidx, regidx, regidx, lsl #2
  lsl   regidx, regidx, #1
.unreq gpionr
bitpos .req w1  
  sub   bitpos, bitpos, regidx
.unreq regidx

  /*
   * bit position = gpio_num * 3
   */
  add   bitpos, bitpos, bitpos, lsl #1
  lsl   mode, mode, bitpos

  /*
   * bit mask = is argument to bic away old value, 
   * ex: 0b00001110
   */
bitmask .req w3
  mov   bitmask, #7
  lsl   bitmask, bitmask, bitpos

regval .req w4
  ldr   regval, [regaddr]
  bic   regval, regval, bitmask
  orr   regval, regval, mode
  str   regval, [regaddr]

.unreq bitmask
.unreq regaddr
.unreq regval
.unreq mode
  ret
