/*
 * mouse_batt.c
 *
 *  Created on: Feb 14, 2014
 *      Author: xuzhen
 */
#include "../../proj/tl_common.h"
#include "vacuum_controller.h"
#include "vacuum_controller_button.h"
#include "../../proj_lib/pm.h"
//#include "vacuum_controller_custom.h"
#include "vacuum_controller_batt.h"

#ifndef DBG_BATT_LOW
#define DBG_BATT_LOW    0
#endif

/*
(btn_status.btn_not_release << 1 ) | (key_repeat.key_repeat_flg)

=   0   ->	release status
=   3   ->  repeat  status
=   2   ->  press   status
*/
#if(!MANUAL_PAIR_EN)

u8 valid_batt_value;
u8 vacuum_deepsleep_cnt = 0;

/*
 *return value 1: vin>2.2, or vin<1.9
 *return value 0£º1.63<vin<2.0, 0x0a:2.0<vin<2.1, 0x14:2.1<vin<2.2, 0x64: vin>2.2v
 */
u8 low_batt_det;

_attribute_ram_code_ u8 vacuum_battery_low_detect(u8 vbat_chn, u8 high_ref, u8 high_scal)
{

	low_batt_det = 0;

    if(!battery_low_by_set (vbat_chn, high_ref, high_scal)){			//vin > high_ref
#if(WITH_NEW_DETECT_BATT_FUNC)
    	low_batt_det |= BATT_GE_2P3V_AREA;
#else
    	low_batt_det = BATT_DETECT_HIGH;				// 2.13v < vin < 2.28v
#endif
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_1050_MV, SCALING_SELECT_HALF)){

#if(WITH_NEW_DETECT_BATT_FUNC)
    	low_batt_det |= BATT_GE_2P1V_AREA;
#else
    	low_batt_det = BATT_DETECT_2P1;				// 2.13v < vin < 2.28v
#endif
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_1015_MV, SCALING_SELECT_HALF)){

#if(WITH_NEW_DETECT_BATT_FUNC)
    	low_batt_det |=  BATT_GE_2P0V_AREA;
#else
    	low_batt_det = BATT_DETECT_2P0;				// 2.02 < vin < 2.13v
#endif
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_955_MV, SCALING_SELECT_HALF)){

#if(WITH_NEW_DETECT_BATT_FUNC)
    	low_batt_det |=  BATT_LESS_2P0V_AREA;
#else
    	low_batt_det = BATT_DETECT_1P9;				//1.91 < vin <2.02v
#endif
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_910_MV, SCALING_SELECT_HALF)){

#if(WITH_NEW_DETECT_BATT_FUNC)
    	low_batt_det |= BATT_LESS_2P0V_AREA ;
#else
    	low_batt_det = BATT_DETECT_1P9;							//1.8 < vin <1.9v
#endif
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_860_MV, SCALING_SELECT_HALF)){

#if(WITH_NEW_DETECT_BATT_FUNC)
    	low_batt_det |= BATT_LESS_2P0V_AREA;
#else
    	low_batt_det = BATT_DETECT_1P9;							//1.8 < vin <1.9v
#endif
    }
    else{
#if(WITH_NEW_DETECT_BATT_FUNC)
    	low_batt_det |= BATT_LESS_2P0V_AREA;
#else
    	low_batt_det = BATT_DETECT_1P9;							//1.8 < vin <1.9v
#endif
    }

    return low_batt_det;
}
#else
u8 vbat_2p9_flag = 1;
u8 vacuum_battery_low_detect(u8 vbat_chn, u8 high_ref, u8 high_scal)
{
	u8 low_batt_det;

    if(!battery_low_by_set (vbat_chn, high_ref, high_scal)){			//vin > high_ref
    	vbat_2p9_flag = 1;
    	low_batt_det = BATT_DETECT_HIGH;
    	return low_batt_det;
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_1160_MV, SCALING_SELECT_3QUARTER)){
    	low_batt_det = BATT_DETECT_HIGH;
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_1050_MV, SCALING_SELECT_3QUARTER)){
    	low_batt_det = BATT_DETECT_2P1;				// 1.4 < vin < 1.54v
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_1015_MV, SCALING_SELECT_3QUARTER)){
    	low_batt_det = BATT_DETECT_2P0;				// 1.35 < vin < 1.4v
    }
    else if(!battery_low_by_set (vbat_chn, REF_VOLTAGE_SEL_955_MV, SCALING_SELECT_3QUARTER)){
    	low_batt_det = BATT_DETECT_1P9;				//1.91 < vin <2.02v
    }
    else{
    	low_batt_det = 0;
    }
    vbat_2p9_flag = 0;
    return low_batt_det;
}
#endif



_attribute_ram_code_ void batt_detect_and_fliter(void)
{
#if(WITH_NEW_DETECT_BATT_FUNC)
	batt_info.batt_value[0] = vacuum_battery_low_detect(COMP_GP3, REF_VOLTAGE_SEL_1160_MV, SCALING_SELECT_HALF);
	//batt_value > 2.3V -> 0, 2.2 > batt_value > 2.1V -> 1, 2.1 > batt_value > 2.0V -> 2, batt_value < 2.0V -> 3
	if(batt_info.batt_value[0] > BATT_GE_2P3V_AREA){
		//detect again to filter three times
		for(int k=1; k<MAX_BATT_DETECT; k++){
			sleep_us(5);
			batt_info.batt_value[k] = vacuum_battery_low_detect(COMP_GP3, REF_VOLTAGE_SEL_1160_MV, SCALING_SELECT_HALF);
		}
		if((batt_info.batt_value[0] == batt_info.batt_value[1]) \
				&& (batt_info.batt_value[1] == batt_info.batt_value[2])){
			valid_batt_value = batt_info.batt_value[0];
		}

		if( valid_batt_value  == BATT_LESS_2P0V_AREA){
			vacuum_deepsleep_cnt++;
			vc_sleep.mode = SLEEP_MODE_DEEPSLEEP;
		}
		else{
			vacuum_deepsleep_cnt = 0;
		}
	}
	else{
		valid_batt_value = 0;
	}

#else
	batt_info.batt_value[0] = vacuum_battery_low_detect(COMP_GP3, REF_VOLTAGE_SEL_1160_MV, SCALING_SELECT_HALF);

	if(batt_info.batt_value[0] < 0x64){
		//detect again to filter three times
		for(int k=1; k<MAX_BATT_DETECT; k++){
			batt_info.batt_value[k] = vacuum_battery_low_detect(COMP_GP3, REF_VOLTAGE_SEL_1160_MV, SCALING_SELECT_HALF);
		}
		if((batt_info.batt_value[0] == batt_info.batt_value[1]) \
				&& (batt_info.batt_value[1] == batt_info.batt_value[2])){
			valid_batt_value = batt_info.batt_value[0];
			if(!valid_batt_value){
				vacuum_deepsleep_cnt++;
				batt_info.batt_status = STATE_LOW_BATT;
				if(vacuum_deepsleep_cnt > 0)
					vc_sleep.mode = SLEEP_MODE_DEEPSLEEP;
			}
			else
				vacuum_deepsleep_cnt = 0;
		}
	}
	else{
		valid_batt_value = 0x64;
		batt_info.batt_status = STATE_HIGH_BATT;
	}
#endif

}

