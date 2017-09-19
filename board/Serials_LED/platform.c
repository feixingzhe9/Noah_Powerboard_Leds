/**
******************************************************************************
* @file    platform.c 
* @author  Adam Huang
* @version V1.0.0
* @date    25-May-2017
* @brief   This file provides all MICO Peripherals mapping table and platform
*          specific functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MROBOT Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 

#include "stdio.h"
#include "string.h"

#include "platform.h"
#include "platform_config.h"
#include "platform_peripheral.h"
#include "platform_internal.h"
#include "platform_logging.h"
#include "mico_platform.h"
#include "fifo.h"

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/
extern WEAK void PlatformEasyLinkButtonClickedCallback(void);
extern WEAK void PlatformStandbyButtonClickedCallback(void);
extern WEAK void PlatformEasyLinkButtonLongPressedCallback(void);
extern WEAK void bootloader_start(void);

/******************************************************
*               Variables Definitions
******************************************************/

const platform_gpio_t platform_gpio_pins[] =
{
  [MICO_GPIO_SYS_LED]           = { GPIOB,  5 },
  
  [MICO_GPIO_LEDS_PWM]          = { GPIOA,  6 },
  
  [MICO_GPIO_UART2_TX]          = { GPIOA,  2 },
  [MICO_GPIO_UART2_RX]          = { GPIOA,  3 },
  
  [MICO_GPIO_UART1_TX]          = { GPIOA,  9 },
  [MICO_GPIO_UART1_RX]          = { GPIOA, 10 },
   
  [MICO_GPIO_FRONT_LEFT_LED]      = { GPIOB, 0},
    [MICO_GPIO_FRONT_RIGHT_LED]     = { GPIOB, 1 },
  
    [MICO_GPIO_BACK_RIGHT_LED]      = { GPIOA, 6},
    [MICO_GPIO_BACK_LEFT_LED]       = { GPIOA, 7 },
  
   
    [MICO_GPIO_EYES_LED]       = { GPIOB,  14 },
};

const platform_uart_t platform_uart_peripherals[] =
{
  [MICO_UART_1] =
  {
    .port                         = USART1,
    .pin_tx                       = &platform_gpio_pins[MICO_GPIO_UART1_TX],
    .pin_rx                       = &platform_gpio_pins[MICO_GPIO_UART1_RX],
    .pin_cts                      = NULL,
    .pin_rts                      = NULL,
    .tx_dma_config =
    {
      .controller                 = DMA1,
      .channel                    = DMA1_Channel4,
      .irq_vector                 = DMA1_Channel4_IRQn,
      .complete_flags             = DMA_ISR_TCIF4,
      .error_flags                = DMA_ISR_TEIF4,
    },
    .rx_dma_config =
    {
      .controller                 = DMA1,
      .channel                    = DMA1_Channel5,
      .irq_vector                 = DMA1_Channel5_IRQn,
      .complete_flags             = DMA_ISR_TCIF5,
      .error_flags                = DMA_ISR_TEIF5,
    },
  },
  [MICO_UART_2] =
  {
    .port                         = USART2,
    .pin_tx                       = &platform_gpio_pins[MICO_GPIO_UART2_TX],
    .pin_rx                       = &platform_gpio_pins[MICO_GPIO_UART2_RX],
    .pin_cts                      = NULL,
    .pin_rts                      = NULL,
    .tx_dma_config =
    {
      .controller                 = DMA1,
      .channel                    = DMA1_Channel7,
      .irq_vector                 = DMA1_Channel7_IRQn,
      .complete_flags             = DMA_ISR_TCIF7,
      .error_flags                = DMA_ISR_TEIF7,
    },
    .rx_dma_config =
    {
      .controller                 = DMA1,
      .channel                    = DMA1_Channel6,
      .irq_vector                 = DMA1_Channel6_IRQn,
      .complete_flags             = DMA_ISR_TCIF6,
      .error_flags                = DMA_ISR_TEIF6,
    },
  },
};
static UART_HandleTypeDef UartHandle[2];
static DMA_HandleTypeDef  uart_tx_dmaHandle[2];
static DMA_HandleTypeDef  uart_rx_dmaHandle[2];
//platform_uart_driver_t platform_uart_drivers[MICO_UART_MAX];
platform_uart_driver_t platform_uart_drivers[] = 
{
  [MICO_UART_1] = 
  {
    .uart_handle = &UartHandle[0],
    .rx_dma_handle = &uart_rx_dmaHandle[0],
    .tx_dma_handle = &uart_tx_dmaHandle[0],
  },
  [MICO_UART_2] = 
  {
    .uart_handle = &UartHandle[1],
    .rx_dma_handle = &uart_rx_dmaHandle[1],
    .tx_dma_handle = &uart_tx_dmaHandle[1],
  },
};

/* Flash memory devices */
const platform_flash_t platform_flash_peripherals[] =
{
  [MICO_FLASH_EMBEDDED] =
  {
    .flash_type                   = FLASH_TYPE_EMBEDDED,
    .flash_start_addr             = 0x08000000,
    .flash_length                 = 0x80000,
  },
};

platform_flash_driver_t platform_flash_drivers[MICO_FLASH_MAX];

/* Logic partition on flash devices */
const mico_logic_partition_t mico_partitions[] =
{
  [MICO_PARTITION_BOOTLOADER] =
  {
    .partition_owner           = MICO_FLASH_EMBEDDED,
    .partition_description     = "Bootloader",
    .partition_start_addr      = 0x08000000,
    .partition_length          = 0x8000,    //32k bytes
    .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
  },
  [MICO_PARTITION_APPLICATION] =
  {
    .partition_owner           = MICO_FLASH_EMBEDDED,
    .partition_description     = "Application",
    .partition_start_addr      = 0x08010000, 
    .partition_length          = 0x38000,   //224k bytes
    .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
  },
  [MICO_PARTITION_OTA_TEMP] =
  {
    .partition_owner           = MICO_FLASH_EMBEDDED,
    .partition_description     = "OTA Storage",
    .partition_start_addr      = 0x08048000,
    .partition_length          = 0x38000, //224k bytes
    .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
  },
  [MICO_PARTITION_PARAMETER_1] =
  {
    .partition_owner           = MICO_FLASH_EMBEDDED,
    .partition_description     = "PARAMETER1",
    .partition_start_addr      = 0x08008000,
    .partition_length          = 0x400,//0x4000, // 16k bytes
    .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
  },
  [MICO_PARTITION_PARAMETER_2] =
  {
    .partition_owner           = MICO_FLASH_EMBEDDED,
    .partition_description     = "PARAMETER2",
    .partition_start_addr      = 0x0800C000,
    .partition_length          = 0x4000, //16k bytes
    .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
  },
};

/******************************************************
*           Interrupt Handler Definitions
******************************************************/
MICO_RTOS_DEFINE_ISR( SysTick_Handler )
{
  sysTickHandler();
}

MICO_RTOS_DEFINE_ISR( USART1_IRQHandler )
{
  platform_uart_irq( &platform_uart_drivers[MICO_UART_1] );
}


extern UART_HandleTypeDef huart2;
//MICO_RTOS_DEFINE_ISR( USART2_IRQHandler )
//{
  //platform_uart_irq( &platform_uart_drivers[MICO_UART_2] );
  //HAL_UART_IRQHandler(&huart2);
//}

void USART2_IRQHandler(void)
{

    FifoPut(fifo, huart2.Instance->DR);

    //HAL_UART_IRQHandler(&huart2);

}

MICO_RTOS_DEFINE_ISR( DMA1_Channel4_IRQHandler )
{
  platform_uart_tx_dma_irq( &platform_uart_drivers[MICO_UART_1] );
}

MICO_RTOS_DEFINE_ISR( DMA1_Channel7_IRQHandler )
{
  platform_uart_tx_dma_irq( &platform_uart_drivers[MICO_UART_2] );
}

MICO_RTOS_DEFINE_ISR( DMA1_Channel5_IRQHandler )
{
  platform_uart_rx_dma_irq( &platform_uart_drivers[MICO_UART_1] );
}

MICO_RTOS_DEFINE_ISR( DMA1_Channel6_IRQHandler )
{
  platform_uart_rx_dma_irq( &platform_uart_drivers[MICO_UART_2] );
}





/******************************************************
*               Function Definitions
******************************************************/


bool watchdog_check_last_reset( void )
{
  if ( RCC->CSR & RCC_CSR_IWDGRSTF )
  {
    /* Clear the flag and return */
    RCC->CSR |= RCC_CSR_RMVF;
    return true;
  }
  
  return false;
}

void platform_init_peripheral_irq_priorities( void )
{
  /* Interrupt priority setup. Called by MiCO/platform/MCU/STM32F2xx/platform_init.c */
  NVIC_SetPriority( RTC_IRQn         ,  1 ); /* RTC Wake-up event   */
  
  NVIC_SetPriority( USART1_IRQn      ,  6 ); /* MICO_UART_1         */
  NVIC_SetPriority( USART2_IRQn      ,  6 ); /* MICO_UART_2         */

  NVIC_SetPriority( DMA1_Channel4_IRQn,  7 ); /* MICO_UART_1 TX DMA  */
  NVIC_SetPriority( DMA1_Channel5_IRQn,  7 ); /* MICO_UART_1 RX DMA  */
  NVIC_SetPriority( DMA1_Channel7_IRQn,  7 ); /* MICO_UART_2 TX DMA  */
  NVIC_SetPriority( DMA1_Channel6_IRQn,  7 ); /* MICO_UART_2 RX DMA  */

  NVIC_SetPriority( EXTI0_IRQn       , 14 ); /* GPIO                */
  NVIC_SetPriority( EXTI1_IRQn       , 14 ); /* GPIO                */
  NVIC_SetPriority( EXTI2_IRQn       , 14 ); /* GPIO                */
  NVIC_SetPriority( EXTI3_IRQn       , 14 ); /* GPIO                */
  NVIC_SetPriority( EXTI4_IRQn       , 14 ); /* GPIO                */
  NVIC_SetPriority( EXTI9_5_IRQn     , 14 ); /* GPIO                */
  NVIC_SetPriority( EXTI15_10_IRQn   , 14 ); /* GPIO                */
}

void init_platform( void )
{ 
  platform_pin_config_t pin_config;
  pin_config.gpio_speed = GPIO_SPEED_MEDIUM;
  pin_config.gpio_mode = GPIO_MODE_OUTPUT_PP;
  pin_config.gpio_pull = GPIO_PULLUP;
  
  //  Initialise system led
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_SYS_LED, &pin_config );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_SYS_LED ); 
}

void init_platform_bootloader( void )
{
  platform_pin_config_t pin_config;
  pin_config.gpio_speed = GPIO_SPEED_MEDIUM;
  pin_config.gpio_mode = GPIO_MODE_OUTPUT_PP;
  pin_config.gpio_pull = GPIO_PULLUP;
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_SYS_LED, &pin_config );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_SYS_LED );
}

void MicoSysLed(bool onoff)
{
  if (onoff) {
    MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_SYS_LED );
  } else {
    MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_SYS_LED );
  }
}

bool MicoShouldEnterBootloader(void)
{
  if( 1 )//MicoGpioInputGet((mico_gpio_t)BOOT_SEL)==false && MicoGpioInputGet((mico_gpio_t)MFG_SEL)==true)
    return true;
  else
    return false;
}
