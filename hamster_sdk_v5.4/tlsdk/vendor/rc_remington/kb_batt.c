/*
 * kb_batt.c
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#include "../../proj/tl_common.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/rf_drv.h"
#include "../common/rf_frame.h"

#include "kb_batt.h"
#include "kb_led.h"
#include "kb_custom.h"


u32 batt_det_count = 0;


#define ADC_SAMPLE_NUM   			7   //5  7  9 12
#define BAT_CHN						COMP_GP23
u16 adc_sample[ADC_SAMPLE_NUM+1];
u16 adc_result;


void kb_batt_det_init(void)
{
	gpio_set_input_en(GPIO_GP23,0);
	//AD clk = src_clk * step/mod = 32M(OSC) * 4/32 = 4M    T = 0.25us
	//ad time = sample_time + bit_store_time = (cycle + res_len) * T =
	REG_ADDR8(0x69) = 4;  //step
	REG_ADDR8(0x6a) = CLK_FHS_MZ;  //mod
	REG_ADDR8(0x6b) = 0x80;  //adc clk en

	REG_ADDR8(0x33) = 0x00;  //no auto
	REG_ADDR8(0x2b) = 0x00;  //ref:Vbg 1.224V
	REG_ADDR8(0x2c) = 0x04;  //GP23
	REG_ADDR8(0x3c) = 0x18;  //10 bit 3cycle  13*0.25us = 3.25us
}


u8 kb_batt_det_process(void)
#if BAT_DET
{
	static u8 dectet_cnt = 0;
	if(batt_det_count&0x01 || kb_status.kb_mode != STATE_NORMAL){
		return;
	}

	static int first_flg = 0;
	u16 result;
	int i,j;

	u16 temp;
	for(i=0;i<ADC_SAMPLE_NUM;i++){
		REG_ADDR8(0x35) = 0x80; //start
		sleep_us(5);
		adc_sample[i] = REG_ADDR16(0x38) & 0x3ff;

		if(i){
			if(adc_sample[i] < adc_sample[i-1]){
				temp = adc_sample[i];
				adc_sample[i] = adc_sample[i-1];
				for(j=i-1;j>=0 && adc_sample[j] > temp;j--){
					adc_sample[j+1] = adc_sample[j];
				}
				adc_sample[j+1] = temp;
			}
		}
	}

	analog_write(0x06,0xff);   //ana_06<0> : 1 ->Power down SAR ADC
	adc_sample[ADC_SAMPLE_NUM] = (adc_sample[6]+adc_sample[7]+adc_sample[8])/3;


	if(!first_flg){
		first_flg = 1;
		adc_result = adc_sample[ADC_SAMPLE_NUM];
	}
	else{
		adc_result = ((adc_result*7) + adc_sample[ADC_SAMPLE_NUM])>>3;
	}

	if(batt_det_count >= 400){
		batt_det_count = 0;
		result = (adc_result*1224)/1023;  //mV
		if( result< 900){  //0-0.9V   1.6-1.8V
			dectet_cnt++;
		}
	}
	if(dectet_cnt>= 15)
	{
		dectet_cnt = 0;
		return 1;
	}
	else
		return 0;
}
#else
{

}
#endif
