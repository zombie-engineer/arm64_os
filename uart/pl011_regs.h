#pragma once
#include <board_map.h>

#define PL011_UART_BASE  (PERIPHERAL_BASE_PHY + 0x00201000)

#define PL011_UARTDR    ((reg32_t)(PL011_UART_BASE + 0x00))
#define PL011_UARTFR    ((reg32_t)(PL011_UART_BASE + 0x18))
#define PL011_UARTIBRD  ((reg32_t)(PL011_UART_BASE + 0x24))
#define PL011_UARTFBRD  ((reg32_t)(PL011_UART_BASE + 0x28))
#define PL011_UARTLCR_H ((reg32_t)(PL011_UART_BASE + 0x2c))
#define PL011_UARTCR    ((reg32_t)(PL011_UART_BASE + 0x30))
/* Interrupt mask set/clear */
#define PL011_UARTIMSC  ((reg32_t)(PL011_UART_BASE + 0x38))
/* Raw interrupt status */
#define PL011_UARTRIS   ((reg32_t)(PL011_UART_BASE + 0x3c))
/* Masked interrupt status */
#define PL011_UARTMIS   ((reg32_t)(PL011_UART_BASE + 0x40))
#define PL011_UARTICR   ((reg32_t)(PL011_UART_BASE + 0x44))

/* UART0 enable */
#define PL011_UARTCR_UARTEN (1)

/* UART0 Transmit enable */
#define PL011_UARTCR_TXE (1 << 8)

/* UART0 Recieve enable */
#define PL011_UARTCR_RXE (1 << 9)

/* UART0 Work length */
#define PL011_UARTLCR_H_WLEN_5BITS (0b00 << 5)
#define PL011_UARTLCR_H_WLEN_6BITS (0b01 << 5)
#define PL011_UARTLCR_H_WLEN_7BITS (0b10 << 5)
#define PL011_UARTLCR_H_WLEN_8BITS (0b11 << 5)

/* UART is transmitting */
#define PL011_UARTFR_BUSY (1<<3)

/* Recieve FIFO empty */
#define PL011_UARTFR_RXFE (1<<4)

/* Transmit FIFO full*/
#define PL011_UARTFR_TXFF (1<<5)

/*
 * Interrupts according to
 * PrimeCell UART (PL011) Technical Reference Manual
 */

/* change in nUARTCTS modem status line */
#define PL011_UARTCTSINTR (1<<1)

/* receive interrupt - recieve buffer is not empty */
#define PL011_UARTRXINTR  (1<<4)

/* transmit interrupt - transfer buffer is empty */
#define PL011_UARTTXINTR  (1<<5)

/* recieve timeout interrupt */
#define PL011_UARTRTINTR  (1<<6)

/* framing error in a received character interrupt */
#define PL011_UARTFEINTR  (1<<7)

/* parity error in a received character interrupt */
#define PL011_UARTPEINTR  (1<<8)

/* break error interrupt - break in reception */
#define PL011_UARTBEINTR  (1<<9)

/* overrun error interrupt */
#define PL011_UARTOEINTR  (1<<10)

