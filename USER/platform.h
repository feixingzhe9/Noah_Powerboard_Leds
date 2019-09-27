#ifndef __USER_PLATFORM_H__
#define __USER_PLATFORM_H__
#include "stm32f10x.h"
#include "ucos_ii.h"


typedef enum
{
    PLATFORM_GPIO_SYS_LED,

    PLATFORM_GPIO_SERIAL_LED_FRONT_RIGHT,
    PLATFORM_GPIO_SERIAL_LED_BACK_LEFT,
    PLATFORM_GPIO_SERIAL_LED_BACK_MID,
    PLATFORM_GPIO_SERIAL_LED_BACK_RIGHT,

    PLATFORM_GPIO_MAX, /* Denotes the total number of GPIO port aliases. Not a valid GPIO alias */
    PLATFORM_GPIO_NONE,
} platform_gpio_e;


typedef struct
{
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;

}platform_gpio_t;

extern const platform_gpio_t platform_gpio_pins[];

uint32_t get_tick(void);
void mcu_restart(void);

void hardware_init(void);
void user_param_init(void);

#endif
