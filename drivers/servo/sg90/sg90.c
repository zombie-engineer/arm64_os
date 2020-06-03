#include <drivers/servo/sg90.h>
#include <memory/static_slot.h>
#include <common.h>
#include <stringlib.h>

struct servo_sg90 {
  struct servo servo;
  struct pwm *pwm;
  char id[8];
  struct list_head STATIC_SLOT_OBJ_FIELD(servo_sg90);
};

DECL_STATIC_SLOT(struct servo_sg90, servo_sg90, 4);
static int servo_sg90_initialized = 0;

/*
 * SG90 Servo specs:
 * Wires: Orange: PWM, Red: +5v, Brown: GND
 * Voltage: 4.8V (~5V)
 * PWM: 50Hz
 * Motion: 0-180 degrees, Speed: 0.1 sec, Torgue kg/cm: 2.5
 * Position:
 *         "-90": ~1.ms pulse is all the way to the left
 *           "0" : 1.5ms pulse is middle
 *          "90": ~2.ms pulse is all the way to the right
 *
 *
 *         1-2ms duty cycle
 *     >----------< 
 * 4.8v ___________ 
 *     |          |
 *     |          | 
 *     |          |
 * 0v__|          |_____________________________
 *     ^                                    ^
 *     .                                    .
 *     .                                    .
 *     .<---------------------------------->.
 *            20ms 50hz pwm period
 */

/*
 * Implementation
 * 50Hz, 20ms period with 1-2ms dury cycle is implemented as follows
 * Raspberry PWM module sources from clock CM_CLK_ID_PWM which runs at 19.2MHz
 * The clock integral divisor is set to 192 resulting in 
 * 19 200 000 / 192 = 100 000 Hz.
 * The PWM is set to M/S mode. MSENn = 1
 *
 * M/S mode:
 *          M
 *     <---------->
 *     ___________
 *     |          |
 *     |          | 
 *     |          |
 *   __|          |_____________________________
 *     ^                                    ^
 *     .                                    .
 *     .                                    .
 *     .<---------------------------------->.
 *                     S
 *
 * In M/S mode at 100 000 Hz. The full period is set by S(PWM range register) 
 * and duty cycle is set by M(PWM data register) 
 *
 * The math for M/S values is as follows:
 * at clock set 100 000 Hz: 1  sec = 100 000 ticks.
 *                        : 1   ms = 100 000/1000    = 100 ticks.
 *              period(M) : 20  ms = 100 ticks * 20  = 2000 ticks
 *             -90 deg(S) : 1   ms = 100 ticks * 1   = 100  ticks
 *               0 deg(S) : 1.5 ms = 100 ticks * 1.5 = 150  ticks
 *             +90 deg(S) : 2   ms = 100 ticks * 2   = 200  ticks
 *
 * It is established that the actual value range for the motor
 * are closer to 60-to-250 than 100 to 200, so we use it.
 *
 */
#define SG90_RANGE_START 60
#define SG90_RANGE_END  250
#define SG90_RANGE (SG90_RANGE_END - SG90_RANGE_START)
#define SG90_RANGE_FULL 2000
#define SG90_MIN_DEGREE -90
#define SG90_MAX_DEGREE  90
#define SG90_DEGREE_RANGE (SG90_MAX_DEGREE - SG90_MIN_DEGREE)

static int servo_sg90_set_angle(struct servo *servo, int angle)
{
  uint32_t data;
  float norm;
  int normal_angle;
    
  struct servo_sg90 *s =
    container_of(servo, struct servo_sg90, servo);

  normal_angle = max(angle, SG90_MIN_DEGREE);
  normal_angle = min(normal_angle, SG90_MAX_DEGREE);
  normal_angle -= SG90_MIN_DEGREE;
  norm = ((float)(normal_angle)) / SG90_DEGREE_RANGE;
  //printf("servo_sg90_set_angle:%p,%d,%d,%d\r\n", 
  // s, 
  // angle, normal_angle, (int)(SG90_RANGE * norm));
  data = SG90_RANGE_START + SG90_RANGE * norm;
  return s->pwm->set_data(s->pwm, data);
}

static int servo_sg90_destroy(struct servo *servo)
{
  int err;
  struct servo_sg90 *s =
    container_of(servo, struct servo_sg90, servo);
  err = s->pwm->release(s->pwm);
  if (err) {
    printf("servo_sg90_destroy: failed to release pwm\r\n");
    kernel_panic("Failed to release pwm\r\n");
  }

  servo_sg90_release(s);
  return ERR_OK;
}

struct servo *servo_sg90_create(struct pwm *pwm, const char *id)
{
  struct servo_sg90 *s;
  if (!servo_sg90_initialized) {
    printf("servo_sg90 not initialized\r\n");
    return ERR_PTR(ERR_NOT_INIT);
  }

  s = servo_sg90_alloc();
  if (IS_ERR(s))
    return (struct servo *)s;

  s->servo.set_angle = servo_sg90_set_angle;
  s->servo.destroy = servo_sg90_destroy;
  s->pwm = pwm;
  s->pwm->set_clk_freq(s->pwm, 100000 /* 100 000 Hz */);
  s->pwm->set_range(s->pwm, SG90_RANGE_FULL);
  return &s->servo;
}

int servo_sg90_init()
{
  STATIC_SLOT_INIT_FREE(servo_sg90);
  servo_sg90_initialized = 1;
  return ERR_OK;
}
