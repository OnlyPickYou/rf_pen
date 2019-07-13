/*
 * mouse_batt.c
 *
 *  Created on: Feb 14, 2014
 *      Author: xuzhen
 */
#include "../../proj/tl_common.h"
#include "mouse.h"
#include "../../proj_lib/pm.h"
#include "mouse_custom.h"
#include "mouse_batt.h"

#ifndef DBG_BATT_LOW
#define DBG_BATT_LOW    0
#endif

#if MOSUE_BATTERY_LOW_DETECT

void mouse_batt_det_process( rc_status_t *rc_status ){
    #define batt_chn (rc_status->hw_define->vbat_channel)
    if (  batt_chn & MOUSE_BATT_CHN_REUSE_FLAG ){   
        u32 gpio_batt_btn = rc_status->hw_define->button[(batt_chn >> 5) & 3];
        gpio_setup_up_down_resistor( gpio_batt_btn, PM_PIN_UP_DOWN_FLOAT );
        WaitUs(50);
    }
    if ( !(batt_chn & MOUSE_BATT_CHN_REUSE_FLAG) ){
        //battery low, led alarm
        u8 vbat_chn = MOUSE_BATT_CHN_REAL_MASK & batt_chn;
        if ( DBG_BATT_LOW || battery_low_detect_auto( vbat_chn == U8_MAX ? COMP_GP6 : vbat_chn ) ){
            //mouse_led_setup( rc_led_cfg[E_LED_BAT_LOW] );
        }
    }
    if ( batt_chn & MOUSE_BATT_CHN_REUSE_FLAG ){   
        u32 gpio_batt_btn = rc_status->hw_define->button[(batt_chn >> 5) & 3];
        gpio_setup_up_down_resistor( gpio_batt_btn, PM_PIN_PULLUP_1M );
    }
}


#endif
