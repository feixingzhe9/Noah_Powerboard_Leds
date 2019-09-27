/*
 *  Author: Kaka Xie
 *  brief: serial leds control
 */

#include "serial_led.h"
#include "delay.h"
#include <string.h>


void cal_color_method_1(volatile uint32_t *buf, color_t *color, uint8_t buf_len, uint32_t tick);
void cal_color_audi_taillight(volatile uint32_t *buf, color_t *color, uint8_t buf_len, uint32_t tick);
void cal_color_audi_taillight_single(volatile uint32_t *buf, color_t *color, uint8_t buf_len, uint32_t tick);
void cal_pure_color(volatile uint32_t *buf, color_t *color, uint8_t buf_len, uint32_t tick);


__IO uint32_t back_mid_buff[BACK_MID_LED_NUM] = {0};
__IO uint32_t front_right_buff[FRONT_RIGHT_LED_NUM] = {0};
__IO uint32_t back_right_buff[BACK_RIGHT_LED_NUM] = {0};
__IO uint32_t back_left_buff[BACK_LEFT_LED_NUM] = {0};


#define LED_LIGHTNESS_PERCENT   80
color_t led_color[] =
{
    [SERIAL_LED_COLOR_RED_C]       = {255 * LED_LIGHTNESS_PERCENT / 100,     0 * LED_LIGHTNESS_PERCENT / 100  ,     0 * LED_LIGHTNESS_PERCENT / 100},
    [SERIAL_LED_COLOR_GREEN_C]     = {0 * LED_LIGHTNESS_PERCENT / 100  ,    200 * LED_LIGHTNESS_PERCENT / 100,      0 * LED_LIGHTNESS_PERCENT / 100},
    [SERIAL_LED_COLOR_BLUE_C]      = {0 * LED_LIGHTNESS_PERCENT / 100  ,    0 * LED_LIGHTNESS_PERCENT / 100  ,      255 /** LED_LIGHTNESS_PERCENT / 100*/},
    [SERIAL_LED_COLOR_ORANGE_C]    = {0xc8 * LED_LIGHTNESS_PERCENT / 100,   0x32 * LED_LIGHTNESS_PERCENT / 100,     0x00 * LED_LIGHTNESS_PERCENT / 100},
    [SERIAL_LED_COLOR_WHITE_C]     = {255 * LED_LIGHTNESS_PERCENT / 100,    255 * LED_LIGHTNESS_PERCENT / 100,      255 * LED_LIGHTNESS_PERCENT / 100},
    [SERIAL_LED_COLOR_CYAN_C]      = {0 * LED_LIGHTNESS_PERCENT / 100  ,    255 * LED_LIGHTNESS_PERCENT / 100,      255 * LED_LIGHTNESS_PERCENT / 100},
    [SERIAL_LED_COLOR_GOLD_C]      = {255 * LED_LIGHTNESS_PERCENT / 100,    215 * LED_LIGHTNESS_PERCENT / 100,      0 * LED_LIGHTNESS_PERCENT / 100},
    [SERIAL_LED_COLOR_SETTING_C]   = {0 * LED_LIGHTNESS_PERCENT / 100  ,    0 * LED_LIGHTNESS_PERCENT / 100  ,      0 * LED_LIGHTNESS_PERCENT / 100  },
    [SERIAL_LED_COLOR_NONE_C]      = {0 * LED_LIGHTNESS_PERCENT / 100  ,    0 * LED_LIGHTNESS_PERCENT / 100  ,      0 * LED_LIGHTNESS_PERCENT / 100  },
};

light_mode_para_t light_mode_para[] =
{
    [LIGHTS_MODE_ERROR] =
    {
        .color      = &led_color[SERIAL_LED_COLOR_RED_C],
        .period     = 50,
    },
    [LIGHTS_MODE_LOW_POWER] =
    {
        .color      = &led_color[SERIAL_LED_COLOR_RED_C],
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


one_wire_led_para_t one_wire_led[] =
{
    [BACK_MID] =
    {
        .gpio               = PLATFORM_GPIO_SERIAL_LED_BACK_MID,
//        .color              = front_left_color,
        .color_number       = 1,
        .period             = 5 * OS_TICKS_PER_SEC / 10,
        .data_buf           = back_mid_buff,
        .led_num            = BACK_MID_LED_NUM,
        .start_time         = 0,
        .method             = (cal_color_method_fn)cal_pure_color,
    },
    [FRONT_RIGHT_LED] =
    {
        .gpio               = PLATFORM_GPIO_SERIAL_LED_FRONT_RIGHT,
//        .color              = front_right_color,
        .color_number       = 1,
        .period             = 5 * OS_TICKS_PER_SEC / 10,
        .data_buf           = front_right_buff,
        .led_num            = FRONT_RIGHT_LED_NUM,
        .start_time         = 0,
        .method             = (cal_color_method_fn)cal_color_method_1,
    },
    [BACK_RIGHT_LED] =
    {
        .gpio               = PLATFORM_GPIO_SERIAL_LED_BACK_RIGHT,
//        .color              = back_right_color,
        .color_number       = 1,
        .period             = 5 * OS_TICKS_PER_SEC / 10,
        .data_buf           = back_right_buff,
        .led_num            = BACK_RIGHT_LED_NUM,
        .start_time         = 0,
        .method             = (cal_color_method_fn)cal_color_audi_taillight_single,
    },
    [BACK_LEFT_LED] =
    {
        .gpio               = PLATFORM_GPIO_SERIAL_LED_BACK_LEFT,
//        .color              = back_left_color,
        .color_number       = 1,
        .period             = 5 * OS_TICKS_PER_SEC / 10,
        .data_buf           = back_left_buff,
        .led_num            = BACK_LEFT_LED_NUM,
        .start_time         = 0,
        .method             = (cal_color_method_fn)cal_color_audi_taillight_single,
    },
};


static inline void write_bit_0(platform_gpio_e gpio)
{
    serial_led_output_high(gpio);
    delay_high_0();

    serial_led_output_low(gpio);
    delay_low_0();
}

static inline void write_bit_1(platform_gpio_e gpio)
{
    serial_led_output_high(gpio);
    delay_high_1();

    serial_led_output_low(gpio);
    delay_low_1();
}



void write_rgb(platform_gpio_e gpio, uint32_t word)
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

    for(i = 0; i < 24; i++)
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
    uint8_t i = 0;

    while(i < one_wire_led[led].led_num)
    {
        write_rgb(one_wire_led[led].gpio, one_wire_led[led].data_buf[i]);
        i++;
    }

}



void cal_pure_color(volatile uint32_t *buf, color_t *color, uint8_t buf_len, uint32_t tick)
{

    uint32_t color_int = 0;


    color_int = (color->r << 16) | (color->g << 8) | (color->b);
    for(uint8_t i = 0; i < buf_len; i++)
    {
        buf[i] = color_int;
    }
}


void cal_color_method_1(volatile uint32_t *buf, color_t *color_, uint8_t buf_len, uint32_t tick)
{
    color_t color = {0 , 0 ,0};
    uint32_t color_int = 0;

    if((tick++ % 2) == 1)
    {
        memcpy((void *)&color, (void *)&led_color[SERIAL_LED_COLOR_ORANGE_C], sizeof(color_t));
    }
    else
    {
        memcpy((void *)&color, (void *)&led_color[SERIAL_LED_COLOR_NONE_C], sizeof(color_t));
    }

    color_int = (color.r << 16) | (color.g << 8) | (color.b);
    for(uint8_t i = 0; i < buf_len / 3; i++)
    {
        buf[i] = color_int;
    }

    color_int = (led_color[SERIAL_LED_COLOR_RED_C].r << 16) | (led_color[SERIAL_LED_COLOR_RED_C].g << 8) | (led_color[SERIAL_LED_COLOR_RED_C].b);
    for(uint8_t i = buf_len / 3; i < (buf_len / 3) * 2; i++)
    {
        buf[i] = color_int;
    }

    color_int = (color.r << 16) | (color.g << 8) | (color.b);
    for(uint8_t i = (buf_len / 3) * 2; i < buf_len; i++)
    {
        buf[i] = color_int;
    }
}


/*
cal_color_method_2: 类似于奥迪尾灯效果
*/
void cal_color_audi_taillight(volatile uint32_t *buf, color_t *color_, uint8_t buf_len, uint32_t tick)
{
#define AUDI_TAILLIGHT_ON_DELAY             4
#define AUDI_TAILLIGHT_OFF_DELAY            4
#define AUDI_TAILLIGHT_RESOLUTION           13
#define AUDI_TAILLIGHT_TICK_MAX             (AUDI_TAILLIGHT_ON_DELAY + AUDI_TAILLIGHT_OFF_DELAY + AUDI_TAILLIGHT_RESOLUTION)
    color_t color = led_color[SERIAL_LED_COLOR_ORANGE_C];;
    uint32_t color_int = 0;
    //tick *= 2;    //speed up the effection
#if METHOD_2_TICK_MAX > METHOD_2_RESOLUTION
    uint32_t tick_mod = tick % (METHOD_2_TICK_MAX);
#else
    uint32_t tick_mod = tick % (AUDI_TAILLIGHT_TICK_MAX + 1);
#endif
    color_int = (color.r << 16) | (color.g << 8) | (color.b);

#if METHOD_2_TICK_MAX > METHOD_2_RESOLUTION
    if(tick_mod < METHOD_2_RESOLUTION)
#else
    if(tick_mod <= AUDI_TAILLIGHT_RESOLUTION)
#endif
    {
        /*前 1/3 的灯*/
        for(uint8_t i = 0; i < (buf_len / 3) * (AUDI_TAILLIGHT_RESOLUTION - tick_mod) / AUDI_TAILLIGHT_RESOLUTION; i++)
        {
            buf[i] = 0;
        }

        for(uint8_t i = (buf_len / 3) * (AUDI_TAILLIGHT_RESOLUTION - tick_mod) / AUDI_TAILLIGHT_RESOLUTION; i < buf_len / 3; i++)
        {
            buf[i] = color_int;
        }

        /*中间 1/3 的灯, 全亮红色 */
        color_int = (led_color[SERIAL_LED_COLOR_RED_C].r << 16) | (led_color[SERIAL_LED_COLOR_RED_C].g << 8) | (led_color[SERIAL_LED_COLOR_RED_C].b);
        for(uint8_t i = buf_len / 3; i < (buf_len / 3) * 2; i++)
        {
            buf[i] = color_int;
        }

        /*后 1/3 的灯*/
        color_int = (color.r << 16) | (color.g << 8) | (color.b);
        for(uint8_t i = (buf_len / 3) * 2; i < (buf_len / 3) * 2 + (buf_len / 3) * (tick_mod) / AUDI_TAILLIGHT_RESOLUTION; i++)
        {
            buf[i] = color_int;
        }

        for(uint8_t i = (buf_len / 3) * 2 + (buf_len / 3) * (tick_mod) / AUDI_TAILLIGHT_RESOLUTION; i < buf_len; i++)
        {
            buf[i] = 0;
        }
    }
    else if(tick_mod < AUDI_TAILLIGHT_RESOLUTION + AUDI_TAILLIGHT_ON_DELAY)
    {
        color = led_color[SERIAL_LED_COLOR_ORANGE_C];
        color_int = (color.r << 16) | (color.g << 8) | (color.b);
        for(uint8_t i = 0; i < buf_len / 3; i++)
        {
            buf[i] = color_int;
        }

        for(uint8_t i = (buf_len / 3) * 2; i < buf_len; i++)
        {
            buf[i] = color_int;
        }
    }
    else
    {
        for(uint8_t i = 0; i < buf_len / 3; i++)
        {
            buf[i] = 0;
        }

        for(uint8_t i = (buf_len / 3) * 2; i < buf_len; i++)
        {
            buf[i] = 0;
        }
    }
    
}



/*
cal_color_method_2: 类似于奥迪尾灯效果
*/
void cal_color_audi_taillight_single(volatile uint32_t *buf, color_t *color, uint8_t buf_len, uint32_t tick)
{
#define AUDI_TAILLIGHT_ON_DELAY             4
#define AUDI_TAILLIGHT_OFF_DELAY            4
#define AUDI_TAILLIGHT_RESOLUTION           13
#define AUDI_TAILLIGHT_TICK_MAX             (AUDI_TAILLIGHT_ON_DELAY + AUDI_TAILLIGHT_OFF_DELAY + AUDI_TAILLIGHT_RESOLUTION)

    uint32_t color_int = (color->r << 16) | (color->g << 8) | (color->b);
    //tick *= 2;    //speed up the effection
#if METHOD_2_TICK_MAX > METHOD_2_RESOLUTION
    uint32_t tick_mod = tick % (METHOD_2_TICK_MAX);
#else
    uint32_t tick_mod = tick % (AUDI_TAILLIGHT_TICK_MAX + 1);
#endif

#if METHOD_2_TICK_MAX > METHOD_2_RESOLUTION
    if(tick_mod < METHOD_2_RESOLUTION)
#else
    if(tick_mod <= AUDI_TAILLIGHT_RESOLUTION)
#endif
    {
        for(uint8_t i = 0; i < buf_len * tick_mod / AUDI_TAILLIGHT_RESOLUTION; i++)
        {
            buf[i] = color_int;
        }

        for(uint8_t i = buf_len * tick_mod / AUDI_TAILLIGHT_RESOLUTION; i < buf_len; i++)
        {
            buf[i] = 0;
        }
    }
    else if(tick_mod < AUDI_TAILLIGHT_RESOLUTION + AUDI_TAILLIGHT_ON_DELAY)
    {
        for(uint8_t i = 0; i < buf_len; i++)
        {
            buf[i] = color_int;
        }
    }
    else
    {
        for(uint8_t i = 0; i < buf_len; i++)
        {
            buf[i] = 0;
        }
    }
}



static void write_color(one_wire_led_t led, color_t *color, uint8_t len, uint32_t tick, cal_color_method_fn method)
{
    method(one_wire_led[led].data_buf, color, len, tick);
}


void open_eyes(void)
{
}

void close_eyes(void)
{
}

#define SHINE_HIGH_SPEED_PERIOD         3 * OS_TICKS_PER_SEC / 10
#define SHINE_MEDIUM_SPEED_PERIOD       6 * OS_TICKS_PER_SEC / 10
#define SHINE_LOW_SPEED_PERIOD          1 * OS_TICKS_PER_SEC
#define UPDATE_PERIOD                   1 * OS_TICKS_PER_SEC
void set_serial_leds_effect(const light_mode_t light_mode, color_t  *cur_color, const uint8_t period)
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
        case LIGHTS_MODE_NORMAL:    //test effect

            one_wire_led[FRONT_RIGHT_LED].period = 1 * OS_TICKS_PER_SEC / 20;
            one_wire_led[FRONT_RIGHT_LED].tick = 0;
            one_wire_led[FRONT_RIGHT_LED].color_number = 1;
            one_wire_led[FRONT_RIGHT_LED].color[0] = led_color[SERIAL_LED_COLOR_WHITE_C];
            one_wire_led[FRONT_RIGHT_LED].method = cal_pure_color;

            one_wire_led[BACK_LEFT_LED].period = 1 * OS_TICKS_PER_SEC / 20;
            one_wire_led[BACK_LEFT_LED].tick = 0;
            one_wire_led[BACK_LEFT_LED].color_number = 1;
            one_wire_led[BACK_LEFT_LED].color[0] = led_color[SERIAL_LED_COLOR_ORANGE_C];
            one_wire_led[BACK_LEFT_LED].method = cal_color_audi_taillight_single;


            one_wire_led[BACK_RIGHT_LED].period = 1 * OS_TICKS_PER_SEC / 20;
            one_wire_led[BACK_RIGHT_LED].tick = 0;
            one_wire_led[BACK_RIGHT_LED].color_number = 1;
            one_wire_led[BACK_RIGHT_LED].color[0] = led_color[SERIAL_LED_COLOR_ORANGE_C];
            one_wire_led[BACK_RIGHT_LED].method = cal_color_audi_taillight_single;

            one_wire_led[BACK_MID].period = 1 * OS_TICKS_PER_SEC;
            one_wire_led[BACK_MID].tick = 0;
            one_wire_led[BACK_MID].color_number = 1;
            one_wire_led[BACK_MID].color[0] = led_color[SERIAL_LED_COLOR_CYAN_C];
            one_wire_led[BACK_MID].method = cal_pure_color;
            open_eyes();
            break;

        case LIGHTS_MODE_ERROR:
            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            close_eyes();
            break;
        case LIGHTS_MODE_COM_ERROR:
            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            close_eyes();
            break;

        case LIGHTS_MODE_LOW_POWER:
            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_HIGH_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            close_eyes();
            break;
        case LIGHTS_MODE_CHARGING_POWER_LOW:

            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_ORANGE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_LOW_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }

            close_eyes();
            break;
        case LIGHTS_MODE_CHARGING_POWER_MEDIUM:

            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_ORANGE_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_GREEN_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_LOW_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }

            close_eyes();
            break;
        case LIGHTS_MODE_CHARGING_FULL:

            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_GREEN_C];
                one_wire_led[(one_wire_led_t)i].color_number = 1;
                one_wire_led[(one_wire_led_t)i].period = UPDATE_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }

            close_eyes();
            break;
//        case LIGHTS_MODE_TURN_LEFT:
//            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
//            {
//                if((i == FRONT_LEFT_LED) || ( i == BACK_LEFT_LED))
//                {
//                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_ORANGE_C];
//                }
//                else
//                {
//                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_NONE_C];
//                }

//                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_NONE_C];
//                one_wire_led[(one_wire_led_t)i].color_number = 2;
//                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
//                one_wire_led[(one_wire_led_t)i].tick = 0;
//            }
//            open_eyes();
//            break;
        case LIGHTS_MODE_TURN_RIGHT:
            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                if((i == FRONT_RIGHT_LED) || ( i == BACK_RIGHT_LED))
                {
                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_ORANGE_C];
                }
                else
                {
                    one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_NONE_C];
                }

                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_MEDIUM_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            open_eyes();
            break;
        case LIGHTS_MODE_EMERGENCY_STOP:
            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_RED_C];//led_color[WHITE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 1;
                one_wire_led[(one_wire_led_t)i].period = UPDATE_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            close_eyes();
            break;
        case LIGHTS_MODE_SETTING:
            for(uint8_t i = BACK_MID; i <= BACK_RIGHT_LED; i++)
            {
                memcpy(&led_color[SERIAL_LED_COLOR_SETTING_C], cur_color, sizeof(color_t));
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[SERIAL_LED_COLOR_SETTING_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[SERIAL_LED_COLOR_NONE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = period;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
            open_eyes();
            break;
        default :
            break;
    }
}

void serial_leds_tick(void)
{
    for(uint8_t i = BACK_MID; i < LED_NONE; i++)
    {
        if(get_tick() - one_wire_led[i].start_time >= one_wire_led[i].period)
        {
            one_wire_led[i].tick++;
            one_wire_led[i].start_time = get_tick();

            if(one_wire_led[i].color_number <= 2)
            {
                write_color((one_wire_led_t)i, &(one_wire_led[i].color[one_wire_led[i].tick % one_wire_led[i].color_number]),one_wire_led[i].led_num, one_wire_led[i].tick, one_wire_led[i].method);
                OS_ENTER_CRITICAL();
                send_rgb_data((one_wire_led_t)i);
                OS_EXIT_CRITICAL();
            }
        }
    }
}


