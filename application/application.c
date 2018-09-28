
#include "main.h"
#include "stm32f1xx_hal.h"

#include "mico.h"
#include "platform.h"
#include "platform_internal.h"
#include "platform_config.h"
#include "serial_leds.h"
#include "uart_protocol.h"
#include "fifo.h"

#define os_PowerBoard_log(format, ...)  custom_log("PowerBoard", format, ##__VA_ARGS__)

extern void Main_Menu(void);
#define Application_REVISION "v2.1"

#ifdef SIZE_OPTIMIZE
const t char menu[] =
"\r\n"
"PowerBoard application for %s, %s, HARDWARE_REVISION: %s\r\n"
"0:BOOTUPDATE,"
"1:FWUPDATE,"
"2:DRIVERUPDAT,"
"3:PARAUPDATE,"
"4:FLASHUPDATE,"
"5:MEMORYMAP,"
"6:BOOT,"
"7:REBOOT";
#else
const char menu[] =
"\r\n"
"PowerBoard application for %s, %s, HARDWARE_REVISION: %s\r\n"
"+ command -------------------------+ function ------------+\r\n"
"| 0:BOOTUPDATE    <-r>             | Update bootloader    |\r\n"
"| 1:FWUPDATE      <-r>             | Update application   |\r\n"
"| 2:PARUPDATE     <-id n><-r><-e>  | Update MICO partition|\r\n"
"| 3:FLASHUPDATE   <-dev device>    |                      |\r\n"
"|  <-e><-r><-start addr><-end addr>| Update flash content |\r\n"
"| 4:MEMORYMAP                      | List flash memory map|\r\n"
"| C:COMLOGON                       | Print com log        |\r\n"
"| D:COMLOGOFF                      | No print com log     |\r\n"
"| M:MODE                           | Set leds mode        |\r\n"
"| E:EFFECT                         | Set leds effect      |\r\n"
"| G:WatchDog  R:ResetMcu    ?:help                        |\r\n"
"+----------------------------------+----------------------+\r\n"
"|    (C) COPYRIGHT 2016 MUYE Corporation  By Driver Group |\r\n"
" Notes:\r\n"
" -e Erase only  -r Read from flash -dev flash device number\r\n"
"  -start flash start address -end flash start address\r\n"
" Example: Input \"4 -dev 0 -start 0x400 -end 0x800\": Update \r\n"
"          flash device 0 from 0x400 to 0x800\r\n";
#endif


int main(void)
{
    init_clocks();
    init_architecture();
    init_platform();
    printf ( menu, MODEL, SW_VERSION, HW_VERSION );
    os_PowerBoard_log( "System clock = %d Hz",HAL_RCC_GetHCLKFreq() );

    HAL_Init();

    init_serial_leds();

    init_fifo(fifo, fifo_data_in_ram, RCV_DATA_LEN_MAX);
    while (1)
    {
        //Main_Menu();
        //UartSendTest();
        leds_protocol_period();
    }

}


#if 0
uint32_t uart_send_test_start_time = 0;
#define UART_SEND_TEST_PERIOD       500/SYSTICK_PERIOD
static void UartSendTest(void)
{
    if(os_get_time() - uart_send_test_start_time >= UART_SEND_TEST_PERIOD)
    {
        uart_send_test_start_time = os_get_time();
        HAL_UART_Transmit(&huart2, test_data, sizeof(test_data), 10);
    }
}
#endif


#define SYS_LED_PERIOD      500/SYSTICK_PERIOD
static uint32_t sys_led_start_time = 0;
void SysLed(void)
{
    if( os_get_time() - sys_led_start_time > SYS_LED_PERIOD )
    {
        sys_led_start_time = os_get_time();
        MicoGpioOutputTrigger( MICO_GPIO_SYS_LED );
    }
}
