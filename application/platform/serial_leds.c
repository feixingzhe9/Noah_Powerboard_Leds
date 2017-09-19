/* 
*  Author: Kaka
*  Date:2017/09/10
*/
#include "serial_leds.h"

#include "mico.h"

#include "fifo.h"


#define LIGHTS_DEBUG

#define LEDS_MODES_N                    LIGHTS_MODE_MAX
#define LED_EFFECT_N                    10

#define serial_leds_log(M, ...) custom_log("SerialLeds", M, ##__VA_ARGS__)
#define serial_leds_log_trace() custom_log_trace("SerialLeds")

__IO uint32_t buff[TOTAL] = {0};
serial_leds_t *serial_leds;



__IO uint32_t front_left_buff[FRONT_LEFT_LED_NUM] = {0};
__IO uint32_t front_right_buff[FRONT_RIGHT_LED_NUM] = {0};
__IO uint32_t back_right_buff[BACK_RIGHT_LED_NUM] = {0};
__IO uint32_t back_left_buff[BACK_LEFT_LED_NUM] = {0};

__IO uint32_t eyes_buff[EYES_LED_NUM] = {0};


OSStatus SerialLeds_Init( void )
{ 
    OSStatus err = kNoErr;

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

    serial_leds_log("serial leds init success!");

    SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );

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
void SetSerialLedsEffect( const light_mode_t light_mode, color_t  *cur_color, const uint8_t period )
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
          if( (pre_color.b == cur_color->b)  && (pre_color.g == cur_color->g) && (pre_color.r == cur_color->r))
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
    case LIGHTS_MODE_NOMAL:
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
            one_wire_led[(one_wire_led_t)i].period = SHINE_HIGH_SPEED_PERIOD; 
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
    case LIGHTS_MODE_CHARGING: 
        if(1 < 46000)     //charging low power -- test code 
        {
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[RED_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[ORANGE_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_LOW_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
        }
        else if(2 < 50000)    //charging  power medium -- test code 
        {
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[ORANGE_C];
                one_wire_led[(one_wire_led_t)i].color[1] = led_color[GREEN_C];
                one_wire_led[(one_wire_led_t)i].color_number = 2;
                one_wire_led[(one_wire_led_t)i].period = SHINE_LOW_SPEED_PERIOD;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
        }
        else        //charging power full -- test code 
        {
            for(uint8_t i = FRONT_LEFT_LED; i <= BACK_RIGHT_LED; i++)
            {
                one_wire_led[(one_wire_led_t)i].color[0] = led_color[GREEN_C];
                one_wire_led[(one_wire_led_t)i].color_number = 1;
                one_wire_led[(one_wire_led_t)i].tick = 0;
            }
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
inline void Write_0(mico_gpio_t gpio)
{
    LedOutputHigh(gpio);
    delay_300ns();
    LedOutputLow(gpio);
    delay_300ns();
    delay_600ns();
}

inline void Write_1(mico_gpio_t gpio)
{
        
    LedOutputHigh(gpio);
    delay_600ns();

    LedOutputLow(gpio);
    delay_600ns();
}
#else
inline void Write_0(mico_gpio_t gpio)
{
    LedOutputHigh(gpio);

    delay_200ns();
    LedOutputLow(gpio);

    delay_200ns();
    delay_500ns();
}

inline void Write_1(mico_gpio_t gpio)
{
        
    LedOutputHigh(gpio);

    delay_500ns();

    LedOutputLow(gpio);

    delay_500ns();
}
#endif

void Write24bit(mico_gpio_t gpio, uint32_t word)
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
			Write_0(gpio);
		}
		else
		{
			Write_1(gpio);
		}			
		
		RGB <<= 1;

	}
}
	
void SendData(one_wire_led_t led)
{
    uint8_t i = one_wire_led[led].led_num;
    
    while(i--)
	{
        Write24bit(one_wire_led[led].gpio, one_wire_led[led].data_buf[i] );
	}
    
}
  
#define LIGHTNESS   (1.25)
static void WriteColor(one_wire_led_t led, color_t *color)
{
    
    //uint32_t word = ((color->r/LIGHTNESS)<<16) | ((color->g/LIGHTNESS)<<8) | color->b/LIGHTNESS;
    uint32_t word = (((color->r/5) * 4 )<<16) | (((color->g/5) * 4 )<<8) | ((color->b/5) * 4) ;
    uint8_t i = one_wire_led[led].led_num;
	

	while(i--)
	{
        one_wire_led[led].data_buf[i] = word;	
	}
}

bool CheckFrameSum(uint8_t *data, uint8_t data_len)
{
    uint8_t sum = 0;
    uint8_t i = 0;
    for( i = 0; i < data_len - 1; i++)
    {
        sum += data[i];
    }
    if(sum == data[data_len - 1])
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    //return (sum == data[i]);
    
}






static uint8_t CalCheckSum(uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for(uint8_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}
extern UART_HandleTypeDef huart2;
void LedsSendFrame(rcv_serial_leds_frame_t *leds_frame)
{
    uint8_t leds_send_buf[LED_FRAME_LEN];
    leds_send_buf[0] = FRAME_HEADER;
    leds_send_buf[1] = 0x0a;
    leds_send_buf[2] = FRAME_TYPE_LEDS_CONTROL;
    memcpy(&leds_send_buf[3], (uint8_t*)leds_frame, sizeof(rcv_serial_leds_frame_t));
    leds_send_buf[8] = CalCheckSum(leds_send_buf, 8);
    leds_send_buf[9] = FRAME_FOOTER;
    
    HAL_StatusTypeDef uart_err = HAL_UART_Transmit(&huart2, leds_send_buf, sizeof(leds_send_buf), 10);
}
   
void AckLedsFrame(light_mode_t light_mode, color_t *cur_color, uint8_t period )
{
    rcv_serial_leds_frame_t leds_frame;
    leds_frame.color.r = cur_color->r;
    leds_frame.color.g = cur_color->g;
    leds_frame.color.b = cur_color->b;
    leds_frame.cur_light_mode = light_mode;
    leds_frame.period = period;
    
    LedsSendFrame(&leds_frame); 
}
void SerialLedsProc(uint8_t const *data)
{
    SetSerialLedsEffect( (light_mode_t)data[3], (color_t*)&data[4], data[7] );
    AckLedsFrame((light_mode_t)data[3], (color_t*)&data[4], data[7] );
}
led_com_opt_t led_com_opt = {0};

void leds_protocol_period(void)
{
    uint8_t data_tmp;
    while(IsFifoEmpty(fifo) == FALSE)
    {
        FifoGet(fifo, &data_tmp);
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
                    if(CheckFrameSum(led_com_opt.rcv_buf,led_com_opt.data_len - 1) == TRUE)
                    {
                        switch(ctype)
                        {
                        case FRAME_TYPE_LEDS_CONTROL:
                            SerialLedsProc(led_com_opt.rcv_buf);
                            
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
void serialLedsTick( void )
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
            //WriteColor((one_wire_led_t)i, &charge_color[one_wire_led[i].tick % one_wire_led[i].color_number]);

            WriteColor((one_wire_led_t)i, &(one_wire_led[i].color[one_wire_led[i].tick % one_wire_led[i].color_number]));  
#if 1
            DISABLE_INTERRUPTS();
            SendData((one_wire_led_t)i);      
            ENABLE_INTERRUPTS();
#endif
        }     
    }
  
#if 0   
    DISABLE_INTERRUPTS();
    for(uint8_t i = FRONT_LEFT_LED; i < LED_NONE; i++)
    {
        SendData((one_wire_led_t)i);      
    }
    ENABLE_INTERRUPTS();
#endif          

}

/*********************END OF FILE**************/
