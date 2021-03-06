/*
 *  Author: Kaka
 *  Date:2017/09/10
 */
#include "serial_leds.h"

#include "mico.h"

#include "fifo.h"


#define serials_leds_log(M, ...) custom_log("serials_leds", M, ##__VA_ARGS__)

#define LIGHTS_DEBUG

#define LEDS_MODES_N                    LIGHTS_MODE_MAX
#define LED_EFFECT_N                    10


/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
uint8_t test_data[] = {0x5a,6,0,0,0x60,0xa5};
uint8_t rcv_buf[50];
HAL_StatusTypeDef uart_err;

#define serial_leds_log(M, ...) custom_log("SerialLeds", M, ##__VA_ARGS__)
#define serial_leds_log_trace() custom_log_trace("SerialLeds")

__IO uint32_t buff[TOTAL] = {0};
serial_leds_t *serial_leds;


__IO uint32_t front_left_buff[FRONT_LEFT_LED_NUM] = {0};
__IO uint32_t front_right_buff[FRONT_RIGHT_LED_NUM] = {0};
__IO uint32_t back_right_buff[BACK_RIGHT_LED_NUM] = {0};
__IO uint32_t back_left_buff[BACK_LEFT_LED_NUM] = {0};

__IO uint32_t eyes_buff[EYES_LED_NUM] = {0};

static void init_serial_leds_gpio(void)
{
    platform_pin_config_t pin_config;
    pin_config.gpio_speed = GPIO_SPEED_HIGH;
    pin_config.gpio_mode = GPIO_MODE_OUTPUT_PP;
    pin_config.gpio_pull = GPIO_PULLUP;


    MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_FRONT_LEFT_LED, &pin_config );
    MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_FRONT_RIGHT_LED, &pin_config );
    MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_BACK_RIGHT_LED, &pin_config );
    MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_BACK_LEFT_LED, &pin_config );
    MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_EYES_LED, &pin_config );

    MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_FRONT_LEFT_LED );
    MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_FRONT_RIGHT_LED );
    MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_BACK_RIGHT_LED );
    MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_BACK_LEFT_LED );
    MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_EYES_LED );
}



static void init_serial_leds_uart(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        //printf("HAL_UART_Init error !");
    }

}


static void init_serial_leds_uart_dma_irq(void)
{

    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

uint8_t serial_leds_uart_buf[255] = {0x00};
static void uart_dma_init(UART_HandleTypeDef* huart)
{

    GPIO_InitTypeDef GPIO_InitStruct;
    if(huart->Instance==USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART2 DMA Init */
        /* USART2_RX Init */
        hdma_usart2_rx.Instance = DMA1_Channel6;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_NORMAL;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
        {
            serials_leds_log("HAL_DMA_Init error !");
        }

        __HAL_LINKDMA(huart,hdmarx,hdma_usart2_rx);

        /* USART2_TX Init */
        hdma_usart2_tx.Instance = DMA1_Channel7;
        hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
        {
            serials_leds_log("HAL_DMA_Init error !");
        }

        __HAL_LINKDMA(huart,hdmatx,hdma_usart2_tx);

        HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);    //enable UART2 global interrupt
    }

}

static void start_dma_rcv(void)
{
    huart2.Instance->CR3 |= USART_CR3_DMAR;

//    hdma_usart2_tx.Instance->CNDTR = 5;
//    hdma_usart2_tx.Instance->CPAR = (uint32_t)&huart2.Instance->DR;
//    hdma_usart2_tx.Instance->CMAR = *(uint32_t*)test_buf;
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart2, serial_leds_uart_buf, SERIALS_LEDS_UART_RCV_LEN);

}

static void serials_leds_uart_dma_init(void)
{
    //MX_GPIO_Init();
    init_serial_leds_uart_dma_irq();
    init_serial_leds_uart();

    uart_dma_init(&huart2);
    start_dma_rcv();
}



OSStatus init_serial_leds( void )
{
    OSStatus err = kNoErr;

    serials_leds_uart_dma_init();
    init_serial_leds_gpio();

    serial_leds_log("serial leds init success!");

    set_serial_leds_effect( LIGHTS_MODE_NORMAL, NULL, 0 );

    return err;
}




extern const platform_gpio_t            platform_gpio_pins[];
static void write_0(void)
{
    LedOutputHigh((mico_gpio_t)MICO_GPIO_FRONT_LEFT_LED);
    delay_300ns();
    LedOutputLow((mico_gpio_t)MICO_GPIO_FRONT_LEFT_LED);
    delay_300ns();
    delay_600ns();
}

static void write_1(void)
{

    LedOutputHigh((mico_gpio_t)MICO_GPIO_FRONT_LEFT_LED);
    delay_600ns();

    LedOutputLow((mico_gpio_t)MICO_GPIO_FRONT_LEFT_LED);
    delay_600ns();
}

static void write_RESET(void)
{
    LedOutputLow((mico_gpio_t)MICO_GPIO_FRONT_LEFT_LED);
    delay_us(80);
}

static void write_24bit(uint32_t word)
{
    uint8_t i;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint32_t RGB;

    R = (word >> 8) & 0xff;
    G = (word >> 16) & 0xff;
    B = (word >> 0) & 0xff;

    RGB = (R << 16)|(G << 8)|(B << 0);

    for(i=0;i<24;i++)
    {

        if((RGB & 0x800000) == 0)
        {
            write_0();
        }
        else
        {
            write_1();
        }

        RGB <<= 1;

    }
}


static uint32_t change_led(uint32_t word,uint8_t level)
{
    static uint32_t rt_word;
    uint8_t R;
    uint8_t G;
    uint8_t B;

    R = (word >> 16) & 0xff;
    G = (word >> 8) & 0xff;
    B = (word >> 0) & 0xff;

    rt_word = ((R*level/LEVEL) << 16) | ((G*level/LEVEL) << 8) | ((B*level/LEVEL) << 0);
    return rt_word;

}

void single_color_water_led(uint32_t color,uint8_t times)
{
    uint8_t i = 0;
    uint8_t j;

    while(times)
    {
        for(j=0;j<TOTAL;j++)
        {
            write_24bit(change_led(color,(j%(LEVEL+1)+i)%(LEVEL+1)));
        }

        write_RESET();
        delay_ms(5);
        i = (i+1)%LEVEL;
        if(i == (LEVEL - 1))
        {
            times--;
        }
    }
}





color_t charge_color[] =
{
    [0]     = {255, 0  , 0  },  //RED_C
    [1]     = {255, 165, 0  },  //ORANGE_C
    [2]     = {0  , 255, 0  },  //GREEN_C
    [3]     = {255, 255, 255},  //WHITE_C
    [4]     = {0  , 255, 255},  //CYAN_C

};
color_t led_color[] =
{
    [RED_C]       = {255, 0  , 0  },
    [GREEN_C]     = {0  , 255, 0  },
    [BLUE_C]      = {0  , 0  , 255},
    [ORANGE_C]    = {0xc8, 0x32, 0x00 },
    [WHITE_C]     = {255, 255, 255},
    [CYAN_C]      = {0  , 255, 255},
    [GOLD_C]      = {255, 215, 0  },
    [SETTING_C]   = {0  , 0  , 0  },
    [NONE_C]      = {0  , 0  , 0  },
};

light_mode_para_t light_mode_para[] =
{
    [LIGHTS_MODE_ERROR] =
    {
        .color      = &led_color[RED_C],
        .period     = 50,
    },
    [LIGHTS_MODE_LOW_POWER] =
    {
        .color      = &led_color[RED_C],
        .period     = 50,
    },


};
color_t  front_left_color[3] =
{
    [0]     = {235, 130 , 5  },  //
    [1]     = {0  , 255, 0  },  //GREEN_C
    [2]     = {255, 165, 0  },  //ORANGE_C
};
color_t  front_right_color[3] =
{
    [0]     = {255, 0  , 0  },  //RED_C
    [1]     = {255, 165, 0  },  //ORANGE_C
    [2]     = {0  , 255, 0  },  //GREEN_C


};
color_t  back_left_color[3] =
{
    [0]     = {255, 255, 255},  //WHITE_C
    [1]     = {0  , 255, 0  },  //GREEN_C
    [2]     = {255, 165, 0  },  //ORANGE_C

};
color_t  back_right_color[3] =
{
    [0]     = {255, 0  , 0  },  //RED_C
    [1]     = {0  , 255, 0  },  //GREEN_C
    [2]     = {255, 165, 0  },  //ORANGE_C

};
color_t  left_eye_color[2] =
{
    [0]     = {255, 255, 255},  //WHITE_C
    [1]     = {0  , 0  , 0  },  //NONE_C
};
color_t  right_eye_color[2] =
{
    [0]     = {255, 255, 255},  //WHITE_C
    [1]     = {0  , 0  , 0  },  //NONE_C
};
extern const platform_gpio_t            platform_gpio_pins[];
one_wire_led_para_t one_wire_led[] =
{
    [FRONT_RIGHT_LED] =
    {
        .gpio               = MICO_GPIO_FRONT_RIGHT_LED,
        .color              = front_right_color,
        .color_number       = 1,
        .period             = 500,
        .data_buf           = front_right_buff,
        .led_num            = FRONT_RIGHT_LED_NUM,
        .start_time         = 0,
    },
    [FRONT_LEFT_LED] =
    {
        .gpio               = MICO_GPIO_FRONT_LEFT_LED,
        .color              = front_left_color,
        .color_number       = 1,
        .period             = 500,
        .data_buf           = front_left_buff,
        .led_num            = FRONT_LEFT_LED_NUM,
        .start_time         = 0,
    },
    [BACK_RIGHT_LED] =
    {
        .gpio               = MICO_GPIO_BACK_RIGHT_LED,
        .color              = back_right_color,
        .color_number       = 1,
        .period             = 500,
        .data_buf           = back_right_buff,
        .led_num            = BACK_RIGHT_LED_NUM,
        .start_time         = 0,
    },
    [BACK_LEFT_LED] =
    {
        .gpio               = MICO_GPIO_BACK_LEFT_LED,
        .color              = back_left_color,
        .color_number       = 1,
        .period             = 500,
        .data_buf           = back_left_buff,
        .led_num            = BACK_LEFT_LED_NUM,
        .start_time         = 0,
    },

    [EYES_LED] =
    {
        .gpio               = MICO_GPIO_EYES_LED,
        .color              = right_eye_color,
        .color_number       = 1,
        .period             = 500,
        .data_buf           = eyes_buff,
        .led_num            = EYES_LED_NUM,
        .start_time         = 0,
    },
};

void OpenEyes(void)
{
    one_wire_led[EYES_LED].color[0] = led_color[WHITE_C];

    one_wire_led[EYES_LED].color_number = 1;

}

void CloseEyes(void)
{
    one_wire_led[EYES_LED].color[0] = led_color[NONE_C];

    one_wire_led[EYES_LED].color_number = 1;

}

#define SHINE_HIGH_SPEED_PERIOD         300/SYSTICK_PERIOD
#define SHINE_MEDIUM_SPEED_PERIOD       600/SYSTICK_PERIOD
#define SHINE_LOW_SPEED_PERIOD          1000/SYSTICK_PERIOD
void set_serial_leds_effect( const light_mode_t light_mode, color_t  *cur_color, const uint8_t period )
{
    static  light_mode_t pre_mode = LIGHTS_MODE_NONE;
    static  color_t      pre_color;
    static  uint8_t      pre_period;
    if((light_mode == pre_mode) && (light_mode != LIGHTS_MODE_SETTING))
    {
        return;
    }

    if(light_mode == LIGHTS_MODE_SETTING)
    {
        if( (pre_color.b == cur_color->b) && (pre_color.g == cur_color->g) && (pre_color.r == cur_color->r))
        {
            if(pre_period == period)
            {
                return;
            }

        }
    }

    pre_mode = light_mode;
    memcpy(&pre_color, cur_color, sizeof(color_t));
    pre_period = period;

    switch(light_mode)
    {
        case LIGHTS_MODE_NORMAL:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[ORANGE_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            OpenEyes();
            break;
        case LIGHTS_MODE_ERROR:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            CloseEyes();
            break;
        case LIGHTS_MODE_COM_ERROR:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            CloseEyes();
            break;

        case LIGHTS_MODE_LOW_POWER:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_HIGH_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            CloseEyes();
            break;
        case LIGHTS_MODE_CHARGING_POWER_LOW:

            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[ORANGE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_LOW_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }

            CloseEyes();
            break;
        case LIGHTS_MODE_CHARGING_POWER_MEDIUM:

            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[ORANGE_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[GREEN_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_LOW_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }


            CloseEyes();
            break;
        case LIGHTS_MODE_CHARGING_FULL:

            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[GREEN_C];
                one_wire_led[(one_wire_led_t)i].color_number = 1;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }

            CloseEyes();
            break;
        case LIGHTS_MODE_TURN_LEFT:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                if((i == FRONT_LEFT_LED) || ( i == BACK_LEFT_LED))
                {
                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[ORANGE_C];
                }
                else
                {
                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[NONE_C];
                }

                one_wire_led[(one_wire_led_t)i].color[1] = led_color[NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            OpenEyes();
            OpenEyes();
            break;
        case LIGHTS_MODE_TURN_RIGHT:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                if((i == FRONT_RIGHT_LED) || ( i == BACK_RIGHT_LED))
                {
                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[ORANGE_C];
                }
                else
                {
                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[NONE_C];
                }

                one_wire_led[(one_wire_led_t)i].color[1] = led_color[NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            OpenEyes();
            break;
        case LIGHTS_MODE_EMERGENCY_STOP:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[RED_C];//led_color[WHITE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 1;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            CloseEyes();
            break;
        case LIGHTS_MODE_SETTING:
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                memcpy(&led_color[SETTING_C], cur_color, sizeof(color_t));
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SETTING_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = period * 10;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            OpenEyes();
            break;
        default :
            break;
    }

}

inline void WriteReset(mico_gpio_t gpio)
{
    LedOutputLow((mico_gpio_t)gpio);
    delay_us(80);
}
#if 0
inline void write_bit_0(mico_gpio_t gpio)
{
    LedOutputHigh(gpio);
    delay_300ns();
    LedOutputLow(gpio);
    delay_300ns();
    delay_600ns();
}

inline void write_bit_1(mico_gpio_t gpio)
{

    LedOutputHigh(gpio);
    delay_600ns();

    LedOutputLow(gpio);
    delay_600ns();
}
#else
inline void write_bit_0(mico_gpio_t gpio)
{
    LedOutputHigh(gpio);

    delay_200ns();
    LedOutputLow(gpio);

    delay_200ns();
    delay_500ns();
}

inline void write_bit_1(mico_gpio_t gpio)
{

    LedOutputHigh(gpio);

    delay_500ns();

    LedOutputLow(gpio);

    delay_500ns();
}
#endif

void write_rgb(mico_gpio_t gpio, uint32_t word)
{
    uint8_t i;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint32_t RGB;

    R = (word >> 8) & 0xff;
    G = (word >> 16) & 0xff;
    B = (word >> 0) & 0xff;

    RGB = (R << 16)|(G << 8)|(B << 0);

    for(i=0;i<24;i++)
    {

        if((RGB & 0x800000) == 0)
        {
            write_bit_0(gpio);
        }
        else
        {
            write_bit_1(gpio);
        }

        RGB <<= 1;

    }
}

static void send_rgb_data(one_wire_led_t led)
{
    uint8_t i = one_wire_led[led].led_num;

    while(i--)
    {
        write_rgb(one_wire_led[led].gpio, one_wire_led[led].data_buf[i] );
    }

}

#define LIGHTNESS   (1.25)
static void write_color(one_wire_led_t led, color_t *color)
{

    //uint32_t word = ((color->r/LIGHTNESS)<<16) | ((color->g/LIGHTNESS)<<8) | color->b/LIGHTNESS;
    uint32_t word = (((color->r/5) * 4 )<<16) | (((color->g/5) * 4 )<<8) | ((color->b/5) * 4) ;
    uint8_t i = one_wire_led[led].led_num;


    while(i--)
    {
        one_wire_led[led].data_buf[i] = word;
    }
}

bool check_frame_sum(uint8_t *data, uint8_t data_len)
{
    uint8_t sum = 0;
    uint8_t i = 0;
    for( i = 0; i < data_len - 1; i++)
    {
        sum += data[i];
    }
    return (sum == data[data_len - 1]);
#if 0
    if(sum == data[data_len - 1])
    {
        return true;
    }
    else
    {
        return false;
    }
    //return (sum == data[i]);
#endif

}

static uint8_t cal_check_sum(uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for(uint8_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

uint8_t leds_send_buf[LED_FRAME_LEN];


static HAL_StatusTypeDef serials_leds_uart_send(uint8_t *data, uint8_t len)
{
    while ((HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX) && (HAL_UART_GetState(&huart2) != HAL_UART_STATE_READY));
    __HAL_DMA_ENABLE(&hdma_usart2_tx);
    return HAL_UART_Transmit_DMA(&huart2, data, len);

    //hdma_usart2_tx.Instance->CNDTR = len;
    //hdma_usart2_tx.Instance->CMAR = (uint32_t)data;
    //__HAL_DMA_ENABLE(&hdma_usart2_tx);

}

static void ack_version(void)
{
    uint8_t version_len = sizeof(SW_VERSION);
    memset(leds_send_buf, 0, sizeof(leds_send_buf));

    leds_send_buf[0] = FRAME_HEADER;
    leds_send_buf[1] = version_len + 5;
    leds_send_buf[2] = FRAME_TYPE_VERSION;
    memcpy(&leds_send_buf[3], SW_VERSION, version_len);

    leds_send_buf[3 + version_len] = cal_check_sum(leds_send_buf, 3 + version_len);
    leds_send_buf[4 + version_len] = FRAME_FOOTER;
    serials_leds_uart_send(leds_send_buf, 5 + version_len);
}



static void ack_serial_leds_frame(light_mode_t light_mode, color_t *cur_color, uint8_t period )
{

    memset(leds_send_buf, 0, sizeof(leds_send_buf));

    leds_send_buf[0] = FRAME_HEADER;
    leds_send_buf[1] = 0x0a;
    leds_send_buf[2] = FRAME_TYPE_LEDS_CONTROL;
    leds_send_buf[3] = light_mode;
    leds_send_buf[4] = cur_color->r;
    leds_send_buf[5] = cur_color->g;
    leds_send_buf[6] = cur_color->b;
    leds_send_buf[7] = period;
    leds_send_buf[8] = cal_check_sum(leds_send_buf, 8);
    leds_send_buf[9] = FRAME_FOOTER;
    serials_leds_uart_send(leds_send_buf, 10);
}

void proc_serial_leds(uint8_t const *data)
{
    set_serial_leds_effect( (light_mode_t)data[3], (color_t*)&data[4], data[7] );
    ack_serial_leds_frame((light_mode_t)data[3], (color_t*)&data[4], data[7] );
}

led_com_opt_t led_com_opt = {0};

void leds_protocol_period(void)
{
    uint8_t data_tmp;
    while(is_fifo_empty(fifo) == FALSE)
    {
        get_byte_from_fifo(fifo, &data_tmp);
        led_com_opt.rcv_buf[led_com_opt.rcv_cnt] = data_tmp;
        if(led_com_opt.start_flag == TRUE)
        {
            if(led_com_opt.rcv_cnt == 1)
            {
                led_com_opt.data_len = data_tmp;
            }
            if(led_com_opt.rcv_cnt == led_com_opt.data_len - 1)
            {
                if(led_com_opt.rcv_buf[led_com_opt.rcv_cnt] == FRAME_FOOTER)
                {
                    uint8_t ctype = led_com_opt.rcv_buf[2];
                    led_com_opt.end_flag = TRUE;
                    led_com_opt.start_flag = FALSE;
                    led_com_opt.rcv_cnt = 0;
                    if(check_frame_sum(led_com_opt.rcv_buf,led_com_opt.data_len - 1))
                    {
                        switch(ctype)
                        {
                            case FRAME_TYPE_LEDS_CONTROL:
                                proc_serial_leds(led_com_opt.rcv_buf);

                                break;
                            case FRAME_TYPE_VERSION:
                                ack_version();
                                break;
                            default :
                                break;
                        }
                    }

                    //printf("o\n");
                    break;
                }
                else
                {
                    led_com_opt.end_flag = FALSE;
                    led_com_opt.start_flag = FALSE;
                    led_com_opt.rcv_cnt = 0;
                }
            }
        }
        else
        {
            if(data_tmp == FRAME_HEADER)
            {
                led_com_opt.start_flag = TRUE;
                led_com_opt.end_flag = FALSE;
            }
            led_com_opt.rcv_cnt = 0;
        }

        if(led_com_opt.rcv_cnt++ >= sizeof(led_com_opt.rcv_buf) - 1)
        {
            led_com_opt.start_flag = FALSE;
            led_com_opt.end_flag = FALSE;
            led_com_opt.rcv_cnt = 0;
        }


    }
}

// period: 10ms
void serial_leds_tick( void )
{

    for(uint8_t i = FRONT_LEFT_LED; i < LED_NONE; i++)
    {
        if(os_get_time() - one_wire_led[i].start_time >= one_wire_led[i].period)
        {
            one_wire_led[i].tick++;
            one_wire_led[i].start_time = os_get_time();
        }

        if(one_wire_led[i].color_number <= sizeof(charge_color)/sizeof(charge_color[0]))
        {
            //write_color((one_wire_led_t)i, &charge_color[one_wire_led[i].tick % one_wire_led[i].color_number]);

            write_color((one_wire_led_t)i, &(one_wire_led[i].color[one_wire_led[i].tick % one_wire_led[i].color_number]));
#if 1
            DISABLE_INTERRUPTS();
            send_rgb_data((one_wire_led_t)i);
            ENABLE_INTERRUPTS();
#endif
        }
    }

#if 0
    DISABLE_INTERRUPTS();
    for(uint8_t i = FRONT_LEFT_LED; i < LED_NONE; i++)
    {
        send_rgb_data((one_wire_led_t)i);
    }
    ENABLE_INTERRUPTS();
#endif

}

/*********************END OF FILE**************/
