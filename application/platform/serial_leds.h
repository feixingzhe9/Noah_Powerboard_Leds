/* 
*  Author: Adam Huang
*  Date:2016/6/15
*/
#ifndef __SERIAL_LED_H__
#define __SERIAL_LED_H__
#include <stdint.h>
#include "Common.h"

//#include "platform.h"
#include "mico_platform.h"

#define FRAME_TYPE_LEDS_CONTROL         0x01
#define FRAME_TYPE_VERSION              0x02

#define LED_FRAME_LEN                   10

#define BITBAND(addr, bitnum) 		((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)  					*((__IO uint32_t*)(addr))

#define GPIOout(GPIOx,bit)				MEM_ADDR(BITBAND((uint32_t)(&GPIOx->ODR), bit))
#define GPIOin(GPIOx,bit)					MEM_ADDR(BITBAND((uint32_t)(&GPIOx->IDR), bit))

#define LED		GPIOout(GPIOC, 6)//GPIOout(GPIOB, 12)

#define FRAME_HEADER                    0x5A
#define FRAME_FOOTER                    0xA5

#define SERIALS_LEDS_UART_RCV_LEN       255

typedef enum {
  LIGHTS_MODE_DEFAULT                   = 0x00,
  LIGHTS_MODE_WELCOME                   = 0x01,
  LIGHTS_MODE_GOODEBYE                  = 0x02,
  LIGHTS_MODE_IDLE                      = 0x03,
  LIGHTS_MODE_DANCE                     = 0x04,
  LIGHTS_MODE_WARMING                   = 0x05,
  LIGHTS_MODE_COM_FAULT                 = 0x06,
  LIGHTS_MODE_BARRIER                   = 0x07,
  LIGHTS_MODE_SOLICIT                   = 0x08,
  LIGHTS_MODE_FREEDOM                   = 0x09,
  LIGHTS_MODE_IS_CHARGING               = 0x0A,
  LIGHTS_MODE_CHARGE_TO_ON              = 0x0B,
  LIGHTS_MODE_CHARGE_FINISH             = 0x0C,
  LIGHTS_MODE_MAX
} lightsMode_t;

typedef enum 
{
    LIGHTS_MODE_NONE                    = 0,
    LIGHTS_MODE_NORMAL                   = 1,
    LIGHTS_MODE_ERROR                   = 2,
    LIGHTS_MODE_LOW_POWER,
    LIGHTS_MODE_CHARGING_POWER_MEDIUM,
    LIGHTS_MODE_CHARGING_POWER_LOW,
    LIGHTS_MODE_CHARGING_FULL,
    LIGHTS_MODE_TURN_LEFT,
    LIGHTS_MODE_TURN_RIGHT,
    LIGHTS_MODE_COM_ERROR,
    LIGHTS_MODE_EMERGENCY_STOP,
      
    
    LIGHTS_MODE_SETTING                 = 0xff,
}light_mode_t;



/*
#define LIGHTS_MODE_DEFAULT             0x00
#define LIGHTS_MODE_WELCOME             0x01
#define LIGHTS_MODE_GOODEBYE            0x02
#define LIGHTS_MODE_IDLE                0x03
#define LIGHTS_MODE_DANCE               0x04
#define LIGHTS_MODE_WARMING             0x05
#define LIGHTS_MODE_COM_FAULT           0x06
#define LIGHTS_MODE_BARRIER             0x07
#define LIGHTS_MODE_SOLICIT             0x08
#define LIGHTS_MODE_FREEDOM             0x09
*/
#define TOTAL                   148
#define LEVEL                   20

#define SERIAL_LEDS_PERIOD      10 

#if 1
#define asm            __asm
#define delay_300ns()     do {asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");} while(1==0)
                                
#define delay_600ns()     do { asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");} while(1==0)
                                
#define delay_us(n)       do { for(uint32_t i=0;i<n;i++){delay_300ns();delay_600ns();}\
                                } while(0==1)
                                
                                
#define delay_200ns()     do {asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");asm("nop");asm("nop");asm("nop");\
                              asm("nop");\
                              } while(1==0)
                                
#define delay_500ns()     do { asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");asm("nop");asm("nop");\
                               asm("nop");asm("nop");} while(1==0)
//#else

#define delay_ms          HAL_Delay
#endif
                               
#define FRONT_LEFT_LED_NUM          16
#define FRONT_RIGHT_LED_NUM         16
#define BACK_RIGHT_LED_NUM          5
#define BACK_LEFT_LED_NUM           5

#define EYES_LED_NUM                20

#if 1
#define  LedOutputHigh(gpio)    platform_gpio_pins[gpio].port->BSRR = (uint16_t) ( 1 << platform_gpio_pins[gpio].pin_number )
#define  LedOutputLow(gpio)     platform_gpio_pins[gpio].port->BSRR = (uint32_t) ( 1 << platform_gpio_pins[gpio].pin_number ) << 16;      
#else
#define  LedOutputHigh(gpio)    gpio.port->BSRR = (uint16_t) ( 1 << gpio.pin_number )
#define  LedOutputLow(gpio)     gpio.port->BSRR = (uint32_t) ( 1 << gpio.pin_number ) << 16; 
#endif
typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
}color_t;

typedef struct
{
    color_t     *color;
    uint8_t     period;
}light_mode_para_t;

typedef struct 
{
    uint16_t shine_period;
    uint8_t led_num;
    uint8_t on_off;
    color_t color;
}OneWireLedCtrl_t;                                 
     



//////////////////////////////////////////
/////////////////////////////////////////
typedef enum 
{
    RED_C = 0,
    GREEN_C,
    BLUE_C,
    ORANGE_C,
    WHITE_C,
    CYAN_C,
    GOLD_C,
    
    SETTING_C,
    NONE_C,
  
}led_color_t;
typedef enum
{
    FRONT_LEFT_LED = 0,
    FRONT_RIGHT_LED,
    BACK_LEFT_LED,
    BACK_RIGHT_LED,    
    EYES_LED, 
    
    LED_NONE,
    
}one_wire_led_t;

typedef struct
{
    mico_gpio_t     gpio;
    color_t         *color;
    uint8_t         color_number;
    uint16_t        period;
    __IO uint32_t   *data_buf;
    uint8_t         led_num;
    uint32_t        start_time;
    uint32_t        tick;
}one_wire_led_para_t;


/////////////////////////////////////////
/////////////////////////////////////////

typedef void (*freshSerialLedsFn_t)(void);

typedef struct _leds_effect_t {
  uint16_t               freshTime;
  freshSerialLedsFn_t    freshSerialLedshandle;
} leds_effect_t;

#pragma pack(1)
typedef struct _serial_leds_t {
  uint8_t                modeType;
  uint16_t               effectType;
  leds_effect_t          *leds_effect;
} serial_leds_t;
#pragma pack()


#pragma pack(1)
typedef struct 
{
//  uint8_t               ctype;
  uint8_t               cur_light_mode;
  color_t               color;
  uint8_t               period;
} rcv_serial_leds_frame_t;
#pragma pack()
typedef struct
{
    uint8_t start_flag;
    uint8_t end_flag;
    uint8_t data_len;
    uint8_t rcv_cnt;
    uint8_t rcv_buf[50];
}led_com_opt_t;

extern serial_leds_t *serial_leds;

void write_0(void);
void write_1(void);
void write_RESET(void);
void write_24bit(uint32_t word);
void reset_led(void);
//void delay_400ns(void);
//void delay_900ns(void);

OSStatus SerialLeds_Init( void );

uint32_t change_led(uint32_t word,uint8_t level);

void single_color_water_led(uint32_t color,uint8_t times);

void style_charge_function( uint32_t color );
//void test_nsDelay(void);

void SetSerialLedsEffect( light_mode_t lightsMode, color_t *color, uint8_t period );

void setCurLedsMode( lightsMode_t lightsMode );

void startDanceLedsMode( void );
void stopDanceLedsMode( void );
void serialLedsTick( void );
void leds_protocol_period(void);
void serials_leds_uart_dma_init(void);

extern UART_HandleTypeDef huart2;
extern uint8_t serial_leds_uart_buf[255];
#endif

