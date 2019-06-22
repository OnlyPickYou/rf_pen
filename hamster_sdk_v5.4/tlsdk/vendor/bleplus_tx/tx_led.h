/*
 * kb_led.h
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#ifndef KB_LED_H_
#define KB_LED_H_

#include "..\..\proj\common\types.h"

//led management
typedef struct{
	unsigned short onTime_ms;
	unsigned short offTime_ms;

	unsigned char  repeatCount;  //0xff special for long on(offTime_ms=0)/long off(onTime_ms=0)
	unsigned char  priority;     //0x00 < 0x01 < 0x02 < 0x04 < 0x08 < 0x10 < 0x20 < 0x40 < 0x80
} tx_led_cfg_t;

typedef struct {
	unsigned char  isOn;
	unsigned char  polar;
	unsigned char  repeatCount;
	unsigned char  priority;


	unsigned short onTime_ms;
	unsigned short offTime_ms;

	unsigned int gpio_led;
	unsigned int startTick;
}tx_device_led_t;
extern tx_device_led_t  tx_led;


enum{
	LED_POWER_ON = 0,
	LED_AUDIO_ON,	//1
	LED_AUDIO_OFF,	//2
	LED_SHINE_SLOW,
	LED_SHINE_FAST,
};


#define  DEVICE_LED_BUSY	(tx_led.repeatCount)
static inline void tx_device_led_process(void)
{
	if(DEVICE_LED_BUSY){
		led_proc();
	}
}

extern const  tx_led_cfg_t tx_led_cfg[];

extern void tx_device_led_init(u32 gpio,u8 polarity);
extern int tx_device_led_setup(tx_led_cfg_t led_cfg);

#endif /* KB_LED_H_ */
