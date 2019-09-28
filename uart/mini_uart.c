#include <uart/mini_uart.h>
#include <gpio.h>


#define AUX_BASE        (MMIO_BASE + 0x00215000)


/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(AUX_BASE + 0x04))
#define AUX_MU_IO       ((volatile unsigned int*)(AUX_BASE + 0x40))
#define AUX_MU_IER      ((volatile unsigned int*)(AUX_BASE + 0x44))
#define AUX_MU_IIR      ((volatile unsigned int*)(AUX_BASE + 0x48))
#define AUX_MU_LCR      ((volatile unsigned int*)(AUX_BASE + 0x4C))
#define AUX_MU_MCR      ((volatile unsigned int*)(AUX_BASE + 0x50))
#define AUX_MU_LSR      ((volatile unsigned int*)(AUX_BASE + 0x54))
#define AUX_MU_MSR      ((volatile unsigned int*)(AUX_BASE + 0x58))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(AUX_BASE + 0x5C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(AUX_BASE + 0x60))
#define AUX_MU_STAT     ((volatile unsigned int*)(AUX_BASE + 0x64))
#define AUX_MU_BAUD     ((volatile unsigned int*)(AUX_BASE + 0x68))

#define AUX_MU_IIR_NO_INTERRUPTS (0b00 << 1)
#define AUX_MU_IIR_TX_EMPTY      (0b01 << 1)
#define AUX_MU_IIR_RX_NONEMPTY   (0b10 << 1)
#define AUX_MU_IIR_FIFO_ENABLED  (0b11 << 6)

#define AUX_MU_LCR_DATASIZE_7BIT (0b00 << 0)
#define AUX_MU_LCR_DATASIZE_8BIT (0b11 << 0)

#define AUX_MU_LSR_TRANSMITTER_EMPTY (1 << 5)
#define AUX_MU_LSR_DATA_READY        (1 << 0)

#define AUX_MU_CNTL_RX_ENABLE (1 << 0)
#define AUX_MU_CNTL_TX_ENABLE (1 << 1)

static int get_baud_reg_value(int target_baudrate, int system_clock)
{
  // from BCM2835 ARM Peripherals.pdf 2.2.1 Mini UART Implementation details.
  return system_clock / target_baudrate / 8  - 1;
}

void mini_uart_init(int baudrate, int system_clock)
{
  *AUX_ENABLE |= 1;
  *AUX_MU_CNTL = 0;
  *AUX_MU_LCR  = AUX_MU_LCR_DATASIZE_8BIT;
  *AUX_MU_MCR  = 0;
  *AUX_MU_IER  = 0;
  *AUX_MU_IIR  = AUX_MU_IIR_NO_INTERRUPTS | AUX_MU_IIR_FIFO_ENABLED;
  *AUX_MU_BAUD = get_baud_reg_value(baudrate, system_clock);

  // pins gpio14, gpio15
  gpio_set_function(14, GPIO_FUNC_ALT_5);
  gpio_set_function(15, GPIO_FUNC_ALT_5);
  gpio_set_pullupdown(14, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  gpio_set_pullupdown(15, GPIO_PULLUPDOWN_NO_PULLUPDOWN);

  *AUX_MU_CNTL = AUX_MU_CNTL_RX_ENABLE | AUX_MU_CNTL_TX_ENABLE;
}

void mini_uart_send(unsigned int c)
{
  do { asm volatile("nop"); } while(!(*AUX_MU_LSR & AUX_MU_LSR_TRANSMITTER_EMPTY)); 
  *AUX_MU_IO = c;
}

char mini_uart_getc()
{
  do { asm volatile("nop"); } while(!(*AUX_MU_LSR & AUX_MU_LSR_DATA_READY)); 
  return (char)*AUX_MU_IO;
}

