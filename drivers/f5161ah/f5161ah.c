#include <led_display/f5161ah.h>


void shiftreg_work()
{
  while(1) {
    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PULSE_RCLK(10000);

    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PULSE_RCLK(10000);

    PUSH_BIT(0, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PULSE_RCLK(10000);
    // PULSE_SRCLR();
  }

}

typedef struct f5161ah {
  shiftreg_t *sr;
} f5161ah_t;

static f5161ah_t dev_f5161ah;
static int dev_f5161ah_initialized = 0;


/* 7 - Segments pinout 
 * 
 *   G    F    GND     A    B
 *   9    8     7      6    5
 *   |    |     |      |    |
 *   |    |     |      |    |
 *    AAAAAAAAAAAAAAAAAAAAAA
 *   F                      B
 *   F                      B
 *   F                      B
 *   F                      B
 *   F                      B
 *    GGGGGGGGGGGGGGGGGGGGGG
 *   E                      C
 *   E                      C
 *   E                      C
 *   E                      C
 *   E                      C
 *    DDDDDDDDDDDDDDDDDDDDDD  Dp
 *   |    |     |      |    |
 *   |    |     |      |    |
 *   0    1     2      3    4
 *   E    D    GND     C    Dp
*/

#define F5161AH_SEG_PIN_A    (1 << 0)
#define F5161AH_SEG_PIN_B    (1 << 1)
#define F5161AH_SEG_PIN_C    (1 << 2)
#define F5161AH_SEG_PIN_D    (1 << 3)
#define F5161AH_SEG_PIN_E    (1 << 4)
#define F5161AH_SEG_PIN_F    (1 << 5)
#define F5161AH_SEG_PIN_DP   (1 << 6)

static uint8_t f5161ah_encode_char(uint8_t ch)
{
  switch(ch) {
    case 0:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_C | F5161AH_SEG_PIN_D | F5161AH_SEG_PIN_E | F5161AH_SEG_PIN_F | F5161AH_SEG_PIN_G;
    case 1:
      return F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_C;
    case 2:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_G | F5161AH_SEG_PIN_E | F5161AH_SEG_PIN_D;
    case 3:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_G | F5161AH_SEG_PIN_C | F5161AH_SEG_PIN_D;
    case 4:
      return F5161AH_SEG_PIN_F | F5161AH_SEG_PIN_G | F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_C;
    case 5:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_F | F5161AH_SEG_PIN_G | F5161AH_SEG_PIN_C | F5161AH_SEG_PIN_D;
    case 6:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_F | F5161AH_SEG_PIN_G | F5161AH_SEG_PIN_C | F5161AH_SEG_PIN_E | F5161AH_SEG_PIN_D;
    case 7:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_C;
    case 8:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_C | F5161AH_SEG_PIN_D | F5161AH_SEG_PIN_E | F5161AH_SEG_PIN_F | F5161AH_SEG_PIN_G;
    case 9:
      return F5161AH_SEG_PIN_A | F5161AH_SEG_PIN_F | F5161AH_SEG_PIN_B | F5161AH_SEG_PIN_G | F5161AH_SEG_PIN_C | F5161AH_SEG_PIN_D;
  }
  
}

int f5161ah_init(shiftreg_t *sr)
{
  if (sr == NULL) {
    puts("f5161ah_init: can not init without shift register.\n");
    return -1;
  }

  dev_f5161ah.sr = sr;
  dev_f5161ah_initialized = 1;
  return 0;
}


int f5161ah_display_char(uint8_t ch, uint8_t dot)
{
  int st;
  uint8_t encoded_char;
  if (!dev_f5161ah_initialized) {
    puts("f5161ah_display_char: not initialized.\n");
    return -1;
  }

  encoded_char = f5161ah_encode_char(ch) | (dot & F5161AH_SEG_PIN_DP);

  if (st = shiftreg_push_byte((char)encoded_char)) {
    puts("f5161ah_display_char: shiftreg_push_byte failed with error %d\n", st);
    return -1;
  }

  if (st = shiftreg_pulse_rclk()) {
    puts("f5161ah_display_char: shiftreg_pulse_rclk failed with error %d\n", st);
    return -1;
  }
  
  return 0;
}
