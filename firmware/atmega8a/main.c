//#include "atmega8.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>

// #define F_CPU 8000000
// #include <util/delay.h>

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

//__attribute__((optimize("O2"))) 
//ISR(SPI_vect)
//{
//  asm volatile("cbi 0x18, 0\n");
//}
//
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

extern void SPI_SlaveInit(void);
extern void SPI_SlaveReceive(void);
void __attribute__((optimize("O2"))) main()
{
  DDRD |= 1<<PIND7;
  PORTD |= (1<<PIND7);
  SPI_SlaveInit();
  SPI_SlaveReceive();
  sei();
  while(1);


  PORTC |= ((1<<PINC4) | (1<<PINC5));
  // Initial I2C Slave
  TWAR = I2CSLAVE_ADDR & 0xfe; // Set I2C Address, Ignore I2C General address 0x00
  TWDR = 0x00;                 // Default value 
  
  // Start Slave Listening: Clear TWINT Flag, Enable ACK, Enable TWI, TWI Interrupt Enable
  TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);
  // Enable Global Interrupt
  regaddr = 0;
  regdata = 0;

  sei();
  while(1);

 // int i;
 // char status;
//  *(char*)0x100 = 0;
//  PIN_MODE_OUT(B, 0);
//  PIN_MODE_OUT(C, 5);
//  PIN_MODE_OUT(D, 7);
//  PIN_OFF(C, 5);
//  return;
}
//  PIN_ON(D, 7);
//  for (i = 0; i < 40; ++i) {
//    PIN_ON(B, 0);
//    wait_100_msec();
//    PIN_OFF(B, 0);
//    wait_100_msec();
//    wait_100_msec();
//  }
//  while(1) {
//    PIN_ON(B, 0);
//    twi_master_init();
//    wait_3_sec();
//    twi_master_start();
//    status = twi_get_status();
//    if (status != 8) {
//      PIN_OFF(D, 7);
//      while(1);
//    }
//    wait_3_sec();
//    PIN_OFF(B, 0);
//    twi_master_deinit();
//    wait_3_sec();
//  }
//}
//
