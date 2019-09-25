/*
 *  Author: Kaka Xie
 *  brief: platform
 */

#include "platform.h"
#include "delay.h"
#include "stdio.h"
#include "led.h"
#include "can.h"
#include "timer.h"
#include "sys.h"


const platform_gpio_t platform_gpio_pins[] =
{
    [PLATFORM_GPIO_SYS_LED]                         = { GPIOB,  GPIO_Pin_3},

    [PLATFORM_GPIO_SERIAL_LED_FRONT_RIGHT]          = {GPIOA, GPIO_Pin_3},
    [PLATFORM_GPIO_SERIAL_LED_FRONT_LEFT]           = {GPIOA, GPIO_Pin_0},
    [PLATFORM_GPIO_SERIAL_LED_BACK_LEFT]            = {GPIOA, GPIO_Pin_1},
    [PLATFORM_GPIO_SERIAL_LED_BACK_RIGHT]           = {GPIOA, GPIO_Pin_2},

};


void serial_led_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);
}

uint32_t get_tick(void)
{
    return OSTimeGet();
}

void mcu_restart(void)
{
    __set_FAULTMASK(1);//close all interrupt
    NVIC_SystemReset();//reset all
}


static void platform_gpio_init(void)
{
    led_init();
    serial_led_init();
}


void hardware_init(void)
{
    platform_gpio_init();
    init_can1();
}

void user_param_init(void)
{
}

