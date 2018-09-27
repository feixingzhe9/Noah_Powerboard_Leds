#pragma once

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/


#define SYSTICK_PERIOD  100
#define os_get_time                   HAL_GetTick
#define DEFAULT_NAME        "POWERBOARD"
#define MODEL               "PB_NOAH_LEDS"

#define HW_VERSION                      "00"
#define SW_VERSION                      "NOAHC001M10A004"
#define PROTOCOL_VERSION                "20170607P0001"

/* MICO RTOS tick rate in Hz */
#define NBOS_DEFAULT_TICK_RATE_HZ                   (1000)

//#define SIZE_OPTIMIZE
#define NO_MICO_RTOS
#define NO_BLOCK_MENU
#define DEBUG               1
/************************************************************************
 * Uncomment to disable watchdog. For debugging only */
//#define MICO_DISABLE_WATCHDOG

/************************************************************************
 * Uncomment to disable standard IO, i.e. printf(), etc. */
//#define MICO_DISABLE_STDIO

/************************************************************************
 * Uncomment to disable MCU powersave API functions */
#define MICO_DISABLE_MCU_POWERSAVE

/************************************************************************
 * Uncomment to enable MCU real time clock */
#define MICO_ENABLE_MCU_RTC

#ifdef BOOTLOADER
#define STDIO_UART          MICO_UART_1
#define STDIO_UART_BAUDRATE (115200)
#else
#define STDIO_UART          MICO_UART_1
#define STDIO_UART_BAUDRATE (115200)
#endif

#define COMM_UART        MICO_UART_1
#define COMM_UART_BAUDRATE (115200) 
#define UART_FOR_APP     MICO_UART_1
#define CLI_UART         MICO_UART_1
