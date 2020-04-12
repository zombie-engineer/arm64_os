//#include "atmega8.h"
#include <atmcmd.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>

// #define F_CPU 8000000
// #include <util/delay.h>

#define DEBUG_PIN_SETUP() DDRD |= 1<<PIND7

#define DEBUG_PIN_ON() PORTD |= (1<<PIND7)

#define DEBUG_PIN_OFF() PORTD &= ~(1<<PIND7)

#define I2CSLAVE_ADDR 0x4e
#define PORT_DDR 0xb0
#define PORT_IN  0xb1
#define PORT_OUT 0xb2

//typedef unsigned short uint16_t;
//typedef short int16_t;
//typedef unsigned int uint32_t;
//typedef int int32_t;
//
//extern void delay_cycles_16(uint16_t num_cycles);
extern void countdown_32(uint16_t count_hi, uint16_t count_low);
#define LOW16(v) (v & 0xffff)
#define HIGH16(v) ((v >> 16) & 0xffff)
#define ARG_SPLIT_32(v) HIGH16(v), LOW16(v)

#define wait_usec(v) countdown_32()

#define PIN_MODE_OUT(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(DDR ## port), "i"(pin))
//
#define PIN_MODE_IN(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(DDR ## port), "i"(pin))
//
#define PIN_ON(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(PORT ## port), "i"(pin))
//
#define PIN_OFF(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(PORT ## port), "i"(pin))
//
//#define PIN_PULL_UP(port, pin) \
//  asm volatile("sbi %0, %1\n"::"i"(PORT ## port), "i"(pin))
//
//#define PIN_PULL_OFF(port, pin) \
//  asm volatile("cbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

//#define PIN_ON(port, pin) PORT ## port |= _BV(pin)
//#define PIN_OFF(port, pin) PORT ## port &= ~_BV(pin)
//#define PIN_MODE_OUT(port, pin) DDR ## port |= _BV(pin)

//void __attribute__((optimize("O2"))) blink() 
//{
//  char *blinker = (char *)0x100;
//  if (*blinker == 1) {
//    *blinker = 0;
//    PIN_ON(B, 0);
//  } else {
//    *blinker = 1;
//    PIN_OFF(B, 0);
//  }
//}

#define MSEC_PER_SEC 1000
// #define FCPU 1000000
#define FCPU 8000000
#define COUNT_PER_SEC (FCPU / 5)
#define COUNT_PER_MSEC (COUNT_PER_SEC / MSEC_PER_SEC)

// 1 second  = 200 000
// 1 milli   = 200 000 / 1000 = 200
// 100 milli = (200 000 / 1000) * 100 = 200 * 100 = 20000
#define MSEC_TO_COUNT(v) (v * COUNT_PER_MSEC)
#define SEC_TO_COUNT(v) (v * COUNT_PER_SEC)

//void wait_1_sec()
//{
//  countdown_32(ARG_SPLIT_32(SEC_TO_COUNT(1)));
//}
//
void wait_3_sec()
{
  countdown_32(ARG_SPLIT_32(SEC_TO_COUNT(3)));
}
//
//void wait_100_msec()
//{
//  countdown_32(ARG_SPLIT_32(MSEC_TO_COUNT(100)));
//}
//
//void wait_1_msec()
//{
//  countdown_32(ARG_SPLIT_32(MSEC_TO_COUNT(1)));
//}
//
//int __attribute__((noinline)) get_int(int x)
//{
//  return x+2;
//}

extern int twi_get_status(void);
extern void twi_master_init(void);
extern void twi_master_deinit(void);
extern void twi_master_start(void);

unsigned char regaddr;
unsigned char regdata;

void i2c_slave_action(unsigned char rw_status)
{
  switch (regaddr) {
    case PORT_DDR:
      if (rw_status == 0)
        // read
        regdata = 0x66;// DDRB;
      else
        // write
        ;//DDRB = regdata;
      break;
    case PORT_IN:
      if (rw_status == 0)
        // read
        regdata = 0x77;
      break;
    case PORT_OUT:
      if (rw_status == 1)
        PORTB |= 1;
      break;
  }
}

static int atmcmd_status;

static inline __attribute__((optimize("O2"))) void atmcmd_cmd(char cmd)
{
  switch (cmd) {
    case ATMCMD_CMD_RESET:
      DEBUG_PIN_OFF();
      atmcmd_status = ATMCMD_STATUS_IDLE;
      break;
    case ATMCMD_CMD_READ_SIGN:
      atmcmd_status = ATMCMD_STATUS_SEND_SIGN1;
      DEBUG_PIN_OFF();
      while(1);
      break;
    case ATMCMD_CMD_ADC_START:
      atmcmd_status = ATMCMD_STATUS_SEND_ADC;
      break;
    case ATMCMD_CMD_GET_RESP:
      atmcmd_status = ATMCMD_STATUS_IDLE;
      break;
    default:
      DEBUG_PIN_OFF();
      atmcmd_status = ATMCMD_STATUS_IDLE;
      SPDR = 0x7f;
      return;
  }
  SPDR = 0x80 | cmd;
}

static uint16_t adc_value = 0x6666;
static char adc_value_low;
static char adc_value_high;
static char prev_cmd;

void atmcmd_adc(char cmd)
{
  if (prev_cmd == 0x22) {
    SPDR = adc_value_low;
  }
  else if (prev_cmd == 0x11) {
    SPDR = adc_value_high;
  }
  else
    SPDR = 0xff;
}

static inline void atmcmd_signature()
{
  SPDR = 0x71;
}

__attribute__((optimize("O2"))) 
ISR(SPI_STC_vect)
{
  char cmd;
  cmd = SPDR;
  switch(atmcmd_status) {
    case ATMCMD_STATUS_IDLE:
      atmcmd_cmd(cmd); 
      break;
    case ATMCMD_STATUS_SEND_SIGN1:
      atmcmd_signature();
      break;
   case ATMCMD_STATUS_SEND_ADC:
      atmcmd_adc(cmd);
      break;
  }
  prev_cmd = cmd;
}

static char chan = 0;

__attribute__((optimize("O2"))) 
ISR(ADC_vect)
{
  adc_value_low = ADCL;
  adc_value_high = ADCH & 3 | ((chan & 1) << 7);
  chan = chan ? 0 : 1;
  ADMUX = 0b01000000 | chan;
  ADCSRA = 0b11001000;
  DEBUG_PIN_ON();
}

ISR(TWI_vect)
{
  static unsigned char i2c_state;
  unsigned char twi_status;

  // Disable Global Interrupt
  cli();
  asm volatile("cbi 0x18, 0\n");
  // PIN_OFF(B, 0);
  while(1);
  PORTB &= ~(1<<PINB0);
  PORTD &= ~(1<<PIND7);


  // Get TWI Status Register, mask the prescaler bits (TWPS1, TWPS0)
  twi_status = (unsigned char)(TWSR & 0xf8);
  PORTB &= ~(1<<PINB0);
  PORTD &= ~(1<<PIND7);

  switch (twi_status) {
    case TW_SR_SLA_ACK: // 0x60: SLA+W received, ACK returned
      i2c_state = 0;    // Start I2C State for Register Address required
      break;
    case TW_SR_DATA_ACK: // 0x80: data received, ACK returned
      PORTB |= 1<<PINB0;
      PORTD |= 1<<PIND7;
      while(1);
      if (i2c_state == 0) {
        regaddr = TWDR; // Save data to the register address
        i2c_state = 1;
      } else {
        regdata = TWDR; // Save data to the register data
        i2c_state = 2;
      }
      break;
    case TW_SR_STOP:    // 0xA0: stop or repeated start condition
      PORTB |= 1<<PINB0;
      PORTD |= 1<<PIND7;
      while(1);
      if (i2c_state == 2) {
        i2c_slave_action(1);
        i2c_state = 0;
      }
      break;
    case TW_ST_SLA_ACK: // 0xA8: SLA+R recieved, ACK returned
    case TW_ST_DATA_ACK:// 0xB8: data transmitted, ACK received
      PORTB |= 1<<PINB0;
      PORTD |= 1<<PIND7;
      while(1);
      if (i2c_state == 1) {
        i2c_slave_action(0); // Call read I2C action
        TWDR = regdata;
        i2c_state = 0;
      }
      break;
    case TW_ST_DATA_NACK: // 0xC0: data transmitted, NACK recieved
    case TW_ST_LAST_DATA: // 0xC8: last data byte transmitted, ACK recieved
    case TW_BUS_ERROR: // 0x00: illegal start or stop condition
    default:
      PORTD |= 1<<PIND7;
      while(1);
      i2c_state = 0;      // Back to the Beginning state
  }
  // Clear TWINT Flag
  TWCR |= (1<<TWINT);
  
  // Enable Global Interrupt
  sei();
}

extern void adc_init_free_int(void);
extern void spi_init_slave(void);
extern void SPI_SlaveReceive(void);

static inline void atmcmd_start()
{
  spi_init_slave();
  atmcmd_status = ATMCMD_STATUS_IDLE;
  adc_value_low = 0xff;
  adc_value_high = 0xff;
  SPDR = 0x38;
}

//static inline void atmcmd_start_i2c()
//{
//  PORTC |= ((1<<PINC4) | (1<<PINC5));
//  // Initial I2C Slave
//  TWAR = I2CSLAVE_ADDR & 0xfe; // Set I2C Address, Ignore I2C General address 0x00
//  TWDR = 0x00;                 // Default value 
//  
//  // Start Slave Listening: Clear TWINT Flag, Enable ACK, Enable TWI, TWI Interrupt Enable
//  TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);
//  // Enable Global Interrupt
//  regaddr = 0;
//  regdata = 0;
//}

static inline void adc_start()
{
  adc_init_free_int();
}

void __attribute__((optimize("O2"))) main()
{
  DEBUG_PIN_SETUP();
  DEBUG_PIN_ON();
  adc_start();
  atmcmd_start();
  sei();
  while(1) asm volatile("sleep");
}



