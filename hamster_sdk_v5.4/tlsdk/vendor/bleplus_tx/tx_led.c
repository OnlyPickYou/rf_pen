/*
 * kb_led.c
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */
#include "../../proj/tl_common.h"

#include "tx_emi.h"
#include "tx_rf.h"
#include "tx_custom.h"
#include "tx_info.h"
#include "tx_batt.h"
#include "tx_pm.h"
#include "tx_led.h"
#include "tx_test.h"

#if (__PROJECT_TX_8267__)


tx_device_led_t  tx_led;

const tx_led_cfg_t tx_led_cfg[] = {
	    {1000,    0,      1,      0x00,	 },    //power-on, 1s on
	    {100,	  0 ,	  0xff,	  0x02,  },    //audio on, long on
	    {0,	      100 ,   0xff,	  0x02,  },    //audio off, long off
	    {500,	  500 ,   3,	  0x04,	 },    //1Hz for 3 seconds
	    {250,	  250 ,   6,	  0x04,  },    //2Hz for 3 seconds
};



void device_led_on_off(u8 on)
{
	gpio_write( tx_led.gpio_led, on^tx_led.polar );
	gpio_set_output_en(tx_led.gpio_led,on);
	tx_led.isOn = on;
}

void tx_device_led_init(u32 gpio,u8 polarity){  //polarity: 1 for high led on, 0 for low led on
	tx_led.gpio_led = gpio;
	tx_led.polar = !polarity;
    gpio_set_func(tx_led.gpio_led,AS_GPIO);
    gpio_set_input_en(tx_led.gpio_led,0);
    gpio_set_output_en(tx_led.gpio_led,0);

    device_led_on_off(0);
}

int tx_device_led_setup(tx_led_cfg_t led_cfg)
{

	if( tx_led.repeatCount &&  tx_led.priority >= led_cfg.priority){
		return 0; //new led event priority not higher than the not ongoing one
	}
	else{
		tx_led.onTime_ms = led_cfg.onTime_ms;
		tx_led.offTime_ms = led_cfg.offTime_ms;
		tx_led.repeatCount = led_cfg.repeatCount;
		tx_led.priority = led_cfg.priority;

        if(led_cfg.repeatCount == 0xff){ //for long on/long off
        	tx_led.repeatCount = 0;
        }
        else{ //process one of on/off Time is zero situation
        	if(!tx_led.onTime_ms){  //onTime is zero
        		tx_led.offTime_ms *= tx_led.repeatCount;
        		tx_led.repeatCount = 1;
        	}
        	else if(!tx_led.offTime_ms){
        		tx_led.onTime_ms *= tx_led.repeatCount;
        		tx_led.repeatCount = 1;
        	}
        }

        tx_led.startTick = clock_time();
        device_led_on_off(tx_led.onTime_ms ? 1 : 0);

		return 1;
	}
}


void led_proc(void)
{
	if(tx_led.isOn){
		if(clock_time_exceed(tx_led.startTick,(tx_led.onTime_ms-5)*1000)){
			device_led_on_off(0);
			if(tx_led.offTime_ms){ //offTime not zero
				tx_led.startTick += tx_led.offTime_ms*CLOCK_SYS_CLOCK_1MS;
			}
			else{
				tx_led.repeatCount = 0;
			}
		}
	}
	else{
		if(clock_time_exceed(tx_led.startTick,(tx_led.offTime_ms-5)*1000)){
			if(--tx_led.repeatCount){
				device_led_on_off(1);
				tx_led.startTick += tx_led.onTime_ms*CLOCK_SYS_CLOCK_1MS;
			}
		}
	}
}


#endif  //end of __PROJECT_TX_8267__
