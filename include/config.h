#pragma once

#define BCM2835_SYSTEM_CLOCK 250000000
#define NUM_CORES 4

//#define CONFIG_SYSTEM_TIMER_BCM2835_SYSTEM_TIMER
#define CONFIG_SYSTEM_TIMER_BCM2835_ARM_TIMER

// #define CONFIG_UART_MINI
#define CONFIG_UART_PL011

// #define CONFIG_EMMC_DEBUG2

#define CONFIG_DEBUG_LIST
// #define CONFIG_DEBUG_MBOX

#ifdef QEMU
#define CONFIG_SCHED_INTERVAL_US 1000000
#else
#define CONFIG_SCHED_INTERVAL_US 10000
#endif /* QEMU */

// #define CONFIG_DEBUG_LED_MBOX

#define CONFIG_DEBUG_LED_1_GPIO_NUM 29
#define CONFIG_DEBUG_LED_2_GPIO_NUM 21
#define CONFIG_DEBUG_LED_3_GPIO_NUM 26

#define CONFIG_DISPLAY_WIDTH 1824
#define CONFIG_DISPLAY_HEIGHT 984

/* EL0 stack size is 1Mb */
#define STACK_SIZE_EL0_LOG 20

/* EL1 stack size is 1Mb */
#define STACK_SIZE_EL1_LOG 20

#define DMA_AREA_SIZE 65536

/* Compile with enabled image download via UART feature */
#define ENABLE_UART_DOWNLOAD

/* Compile with debug via JTAG feature */
#define ENABLE_JTAG
/* Compile with enabled image download via JTAG feature */
#define ENABLE_JTAG_DOWNLOAD

#if defined(ENABLE_JTAG_DOWNLOAD) || defined(ENABLE_UART_DOWNLOAD)
/*
 * Compile with special section to provide enough scratchspace to
 * download images if one of ENABLE_*_DOWNLOAD features is enabled
 */
#define MAX_DOWNLOAD_IMAGE_SIZE 0x200000
#endif
