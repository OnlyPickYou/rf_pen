/*
 * kb_pm.c
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */



#include "../../proj/tl_common.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/rf_drv.h"
#include "../common/rf_frame.h"

#include "tx_emi.h"
#include "tx_rf.h"
#include "tx_custom.h"
#include "tx_info.h"
#include "tx_batt.h"
#include "tx_pm.h"
#include "tx_led.h"
#include "tx_test.h"
#include "tx_keyscan.h"

#if (__PROJECT_TX_8267__)

extern u32	scan_pin_need;

kb_slp_cfg_t kb_sleep = {
	SLEEP_MODE_BASIC_SUSPEND,  //mode
	0,                  //device_busy
	0,					//quick_sleep
	0,                  //wakeup_src

    0,
    166,  //12ms unit
    0,
    19,  //100 ms unit

    0,
    0,
};

//_attribute_ram_code_
inline void kb_slp_mode_machine(kb_slp_cfg_t *s_cfg)
{
    if ( s_cfg->device_busy ){
        s_cfg->mode = SLEEP_MODE_BASIC_SUSPEND;
        s_cfg->cnt_basic_suspend = 0;
        s_cfg->cnt_long_suspend = 0;
    }
    else if (s_cfg->mode == SLEEP_MODE_BASIC_SUSPEND){
        s_cfg->cnt_basic_suspend ++;
        if ( s_cfg->cnt_basic_suspend >= s_cfg->thresh_basic_suspend ){
            s_cfg->mode = SLEEP_MODE_LONG_SUSPEND;
            s_cfg->cnt_long_suspend = 0;
        }
    }


    if ( s_cfg->mode == SLEEP_MODE_LONG_SUSPEND ) {
		s_cfg->cnt_long_suspend ++;
		if ( s_cfg->cnt_long_suspend > s_cfg->thresh_long_suspend )	{
			s_cfg->mode = SLEEP_MODE_DEEPSLEEP;
		}
	}

	if ( s_cfg->quick_sleep ) {
        s_cfg->mode = SLEEP_MODE_DEEPSLEEP;
	}
}


extern
void kb_pm_init(void)
{
	u32 pin[] = KEYBOARD_DRIVE_PINS;
	for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		gpio_set_wakeup(pin[i],1,1);  	   //drive pin core(gpio) high wakeup suspend
		cpu_set_gpio_wakeup (pin[i],1,1);  //drive pin pad high wakeup deepsleep
	}
	gpio_core_wakeup_enable_all(1);


#if(KEYSCAN_IRQ_TRIGGER_MODE)
	gpio_core_irq_enable_all(1);
	reg_irq_src = FLD_IRQ_GPIO_EN;
#endif

	kb_sleep.wakeup_tick = clock_time();
}


#define DEBUG_KB_NO_SUSPEND  1

//_attribute_ram_code_
void kb_pm_proc(void)
{
#if(DEBUG_KB_NO_SUSPEND)
    cpu_suspend_wakeup_sim (KB_MAIN_LOOP_TIME_MS*1000);
#else
	kb_sleep.device_busy = ( kb_status.rf_sending || DEVICE_LED_BUSY );
	kb_sleep.quick_sleep = HOST_NO_LINK;
	if ( kb_status.kb_mode <= STATE_PAIRING && kb_status.loop_cnt < KB_NO_QUICK_SLEEP_CNT){
		kb_sleep.quick_sleep = 0;
	}

	kb_slp_mode_machine( &kb_sleep );


	kb_sleep.wakeup_src = PM_WAKEUP_TIMER;
	kb_sleep.next_wakeup_tick = kb_sleep.wakeup_tick + 11400*16;
	if ( kb_sleep.mode ==  SLEEP_MODE_LONG_SUSPEND){
    	kb_sleep.wakeup_src = PM_WAKEUP_TIMER | PM_WAKEUP_CORE;
    	kb_sleep.next_wakeup_tick = kb_sleep.wakeup_tick + 100*CLOCK_SYS_CLOCK_1MS;
	}
	else if ( kb_sleep.mode == SLEEP_MODE_DEEPSLEEP){
		kb_info_save();
		kb_sleep.wakeup_src = PM_WAKEUP_PAD;
	}

    while ( !clock_time_exceed (kb_sleep.wakeup_tick, 500) );
    cpu_sleep_wakeup (kb_sleep.mode == SLEEP_MODE_DEEPSLEEP, kb_sleep.wakeup_src, kb_sleep.next_wakeup_tick);
	kb_sleep.wakeup_tick = clock_time();
#endif
}

#endif  //end of __PROJECT_TX_8267__
