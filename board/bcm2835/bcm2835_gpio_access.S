#include <asm_macros.h>
#include <arch/armv8/armv8_asm_macros.h>
.equ PERIPHERAL_BASE_PHY, 0x3f000000
.equ GPIO_BASE         ,(PERIPHERAL_BASE_PHY + 0x200000)
.equ GPIO_REG_GPFSEL0  ,(GPIO_BASE + 0x00)
.equ GPIO_REG_GPFSEL1  ,(GPIO_BASE + 0x04)
.equ GPIO_REG_GPFSEL2  ,(GPIO_BASE + 0x08)
.equ GPIO_REG_GPFSEL3  ,(GPIO_BASE + 0x0C)
.equ GPIO_REG_GPFSEL4  ,(GPIO_BASE + 0x10)
.equ GPIO_REG_GPFSEL5  ,(GPIO_BASE + 0x14)
.equ GPIO_REG_GPSET0   ,(GPIO_BASE + 0x1C)
.equ GPIO_REG_GPSET1   ,(GPIO_BASE + 0x20)
.equ GPIO_REG_GPCLR0   ,(GPIO_BASE + 0x28)
.equ GPIO_REG_GPCLR1   ,(GPIO_BASE + 0x2c)
.equ GPIO_REG_GPLEV0   ,(GPIO_BASE + 0x34)
.equ GPIO_REG_GPLEV1   ,(GPIO_BASE + 0x38)
.equ GPIO_REG_GPEDS0   ,(GPIO_BASE + 0x40)
.equ GPIO_REG_GPEDS1   ,(GPIO_BASE + 0x44)
.equ GPIO_REG_GPREN0   ,(GPIO_BASE + 0x4c)
.equ GPIO_REG_GPREN1   ,(GPIO_BASE + 0x50)
.equ GPIO_REG_GPFEN0   ,(GPIO_BASE + 0x58)
.equ GPIO_REG_GPFEN1   ,(GPIO_BASE + 0x5c)
.equ GPIO_REG_GPHEN0   ,(GPIO_BASE + 0x64)
.equ GPIO_REG_GPHEN1   ,(GPIO_BASE + 0x68)
.equ GPIO_REG_GPLEN0   ,(GPIO_BASE + 0x70)
.equ GPIO_REG_GPLEN1   ,(GPIO_BASE + 0x74)
.equ GPIO_REG_GPAREN0  ,(GPIO_BASE + 0x7c)
.equ GPIO_REG_GPAREN1  ,(GPIO_BASE + 0x80)
.equ GPIO_REG_GPAFEN0  ,(GPIO_BASE + 0x88)
.equ GPIO_REG_GPAFEN1  ,(GPIO_BASE + 0x8c)
.equ GPIO_REG_GPPUD    ,(GPIO_BASE + 0x94)
.equ GPIO_REG_GPPUDCLK0,(GPIO_BASE + 0x98)
.equ GPIO_REG_GPPUDCLK1,(GPIO_BASE + 0x9C)

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
 * Read GPIO_REG_GPLEVn register and read the value
 * of corresponding bit;
 * Pseudocode:
 * reg_value = read(GPIO_REG_GPLEVn), where n = gpio number / 32 
 * return reg_value >> (gpio_number % 32) & 1
 * x0 - number of gpio pin 
 */

/*
 * Calculate register offset given the base register address
 * and gpio pin number and read it's value to dest
 * reg_addr - immediate: base register address
 * gpio_num - register : gpio pin number from 0 to 63 used for calculation
 *            of proper register address. Not corrupted
 * dest     - register : recieves the value in the register.
 * tmp      - register : temporary
 */
.macro read_gpio_reg_64 reg_addr, gpio_num, dest, regnum
  mov \regnum, \gpio_num
  lsr \regnum, \regnum, #5
  mov32 \reg_addr, \dest
  add \dest, \dest, \regnum, lsl #2
  ldr \dest, [\dest]
.endm

FUNC(gpio_read_level_reg):
  gpio_num .req x0
  tmp      .req x1
  read_gpio_reg_64 GPIO_REG_GPLEV0, gpio_num, gpio_num, tmp
  .unreq gpio_num
  .unreq tmp
  ret

FUNC(gpio_is_set):
  gpio_num .req x0
  regval   .req x1
  regnum   .req x2
  read_gpio_reg_64 GPIO_REG_GPLEV0, gpio_num, regval, regnum
  sub regnum, gpio_num, regnum, lsl #5
  .unreq regnum
  bitpos .req x2
  mov x0, #1
  lsr regval, regval, bitpos
  and x0, x0, regval
  .unreq gpio_num
  .unreq regval 
  .unreq bitpos
  ret

/* 
 * gpio_read_and_set_64(uin32_t *base_reg_addr, uint32_t pin_nr)
 * x0 - base_reg_addr
 * w1 - pin_nr
 */
FUNC(gpio_read_and_set_64):
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
FUNC(gpio_read_and_set_3):
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
