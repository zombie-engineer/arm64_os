#pragma once

#define BCM2835_SYSTEM_CLOCK 250000000

#define CONFIG_SYSTEM_TIMER_BCM2835_SYSTEM_TIMER
//#define CONFIG_SYSTEM_TIMER_BCM2835_ARM_TIMER

// #define CONFIG_UART_MINI
#define CONFIG_UART_PL011

#ifdef QEMU
#  define CONFIG_SCHED_INTERVAL_US 1000000
#else
#  define CONFIG_SCHED_INTERVAL_US 100
#endif /* QEMU */

#define CONFIG_DEBUG_LED_MBOX
