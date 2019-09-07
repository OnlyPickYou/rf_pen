#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"
#include "../common/device_power.h"

#include "../link_layer/rf_ll.h"

#include "device_info.h"
#include "mouse.h"
#include "mouse_rf.h"
#include "mouse_button.h"
#include "mouse_batt.h"
#include "mouse_rf.h"
#include "mouse_custom.h"
#include "trace.h"

#ifndef MOUSE_GOLDEN_VPATTERN
#define MOUSE_GOLDEN_VPATTERN   0
#endif

#ifndef MOUSE_NO_HW_SIM
#define MOUSE_NO_HW_SIM         (MOUSE_GOLDEN_VPATTERN || MOUSE_EMI_4_FCC || 0)
#endif

#define	MOUSE_V_PATERN_MODE	    (MOUSE_NO_HW_SIM || 0)

/*
 * user_init -mouse_hw 只有gpio和led
 * platform_init -- 初始化button和led
 * 其他不需要
 *
 */

rc_status_t   rc_status;
#if MOUSE_CAVY_RAM

#else
rc_hw_t       rc_hw;
#endif
kb_data_t     rc_data;

extern u32 button_last;
void mouse_sleep_wakeup_init( rc_hw_t *pHW ){
#ifdef reg_gpio_wakeup_en    
	reg_gpio_wakeup_en |= FLD_GPIO_WAKEUP_EN;	//gpio wakeup initial
#endif	
	reg_wakeup_en = FLD_WAKEUP_SRC_GPIO;        //core wakeup gpio enbable

	int i;

	for ( i = MAX_MOUSE_BUTTON - 1; i>=0; i-- ){  //gpio复用时 左中右唤醒
			u32 suspend_wakeuppin = pHW->button[i];
			u32 suspend_wakeupin_level = (pHW->gpio_level_button[i] == U8_MAX);
			cpu_set_gpio_wakeup(suspend_wakeuppin, !suspend_wakeupin_level, 1);  //PAD wakeup
	}

#if MOUSE_DEEPSLEEP_EN

    device_sleep.mcu_sleep_en = CUST_MCU_SLEEP_EN;
    
    //u32 tick = ((p_custom_cfg->slp_tick << 1) + p_custom_cfg->slp_tick) >> 6; //tick = 200ms * 256 * 256 * 3 / 64 = 614s
    device_sleep.thresh_100ms = 5;		//614 s

#else
	device_sleep.thresh_100ms = U16_MAX;
#endif	
}

//button_init,Wheel init, Titl wheel init, Sensor init, led_init
void platform_init( rc_status_t *pStatus )
{
	rc_hw_t *pHW = pStatus->hw_define;
	rc_button_init(pHW);

#if MOUSE_SW_CUS
#define led_cnt_rate    (8 << (pStatus->high_end == MS_HIGHEND_250_REPORTRATE))
	mouse_led_init(pHW->led_cntr, pHW->gpio_level_led, led_cnt_rate);

/* LED2 CTRL Init*/
#if (SWS_CONTROL_LED2_EN)
	gpio_set_func(M_HW_LED2_CTL,AS_GPIO);
	gpio_set_input_en( M_HW_LED2_CTL, 0 ); //input disable
    gpio_set_output_en( M_HW_LED2_CTL, 1 );//output enable
    gpio_write(M_HW_LED2_CTL, 0);
#endif

#endif

#if MOUSE_DEEPSLEEP_EN
	mouse_sleep_wakeup_init(pStatus->hw_define);
#endif
}

void  user_init(void)
{
	swire2usb_init();

	rc_status.hw_define = &rc_hw;
	rc_status.led_define = &device_led;

	rc_status.data = &rc_data;
	rc_status.no_ack = 1;    //no_ack == 0, wakeup sensor from deep-sleep every time

    gpio_pullup_dpdm_internal( FLD_GPIO_DM_PULLUP | FLD_GPIO_DP_PULLUP );
    rc_custom_init( &rc_status );

	device_info_load(&rc_status);
#if ( !MOUSE_NO_HW_SIM )
	platform_init(&rc_status);

#if(MOUSE_WHEEL_WAKEUP_DEEP_EN )
	cpu_set_gpio_wakeup(rc_status.hw_define->wheel[1], 0, 0);
#endif

	rc_button_pull_and_detect(&rc_status);
	rc_button_process_and_mapping(&rc_status);
    
    if ( rc_status.mouse_mode == STATE_POWERON )
        mouse_led_setup( rc_led_cfg[E_LED_POWER_ON] );
#endif
    rc_rf_init(&rc_status);
    rf_set_power_level_index (rc_cust_tx_power_paring);

}

_attribute_ram_code_ void irq_handler(void)
{

	u32 src = reg_irq_src;
#if(!CHIP_8366_A1)
	if(src & FLD_IRQ_GPIO_RISC2_EN){
		gpio_user_irq_handler();
		reg_irq_src = FLD_IRQ_GPIO_RISC2_EN;
	}
#endif
	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_device_rx();
		//printf("It is rx interrupt\n");
	}

	if(src_rf & FLD_RF_IRQ_TX){
		irq_device_tx();
	}
}

void mouse_sync_process(rc_status_t * rc_status){
    static u16 pairing_time = 0;
    u32 pairing_end = 0;
#if MOUSE_GOLDEN_VPATTERN
    if ( HOST_NO_LINK ){
        rc_status->mouse_mode = STATE_POWERON;
    }
#endif
    static u8 led_linked_host = 1;      //mouse, wakeup from deep-sleep, can blinky 3 times when linked with dongle
    if ( rc_status->mouse_mode == STATE_POWERON ) {
        //debug mouse get access code-1 from flash/otp
        if ( GET_HOST_ACCESS_CODE_FLASH_OTP != U16_MAX ){
		    rf_set_access_code1 ( rf_access_code_16to32( GET_HOST_ACCESS_CODE_FLASH_OTP ) );            
        }
        else{            
            rf_set_access_code1 ( U32_MAX );            //powe-on pkt link fail
        }

        mouse_sync_status_update( rc_status );
	}
#if DEVICE_LED_MODULE_EN
    else{
        //if linked with dongle, led blinky 3 times
        if( led_linked_host && (rc_status->no_ack == 0) ){
            led_linked_host = 0;
        }
    }
#endif
    if( rc_status->mouse_mode == STATE_PAIRING ){
    	if( rc_status->no_ack == 0 ){
    	    pairing_end = U8_MAX;        //pairing OK
    	}
        if( pairing_time >= 1024 ){
            pairing_end = 1;             //pairing fail, time out
        }
        else{            
            pairing_time += 1;
        }
        
        if( pairing_end ){
            pairing_time = 0;           //pairing-any-time could re-try many time
            //mouse_led_setup( mouse_led_pairing_end_cfg_cust(pairing_end) );
    	    //gpio_write(M_HW_LED_CTL, 1);		//led power on
            mouse_sync_status_update( rc_status );
        }
        else{
            mouse_led_setup( rc_led_cfg[E_LED_PAIRING] );
        }
    }
    else if ( rc_status->mouse_mode != STATE_EMI ){
        mouse_sync_status_update( rc_status );
    }
}
//u32 debug_last_wakeup_level;

#define   DEBUG_NO_SUSPEND      0
static inline void mouse_power_saving_process( rc_status_t *rc_status ){
    log_event( TR_T_MS_MACHINE);
#if MOUSE_DEEPSLEEP_EN
    device_sleep.quick_sleep = HOST_NO_LINK && QUICK_SLEEP_EN;

    if ( rc_status->mouse_mode <= STATE_PAIRING ){
        if ( rc_status->loop_cnt < 4096 )       //sync dongle time 4096 *8 ms most
            device_sleep.quick_sleep = 0;        
    }
#endif
    device_sleep.device_busy = DEVICE_PKT_ASK || LED_EVENT_BUSY || button_last;
    if ( DEBUG_NO_SUSPEND || (rc_status->dbg_mode & STATE_TEST_0_BIT) ){
        device_sleep.mode = M_SUSPEND_0;
    }
    else{
        if ( device_sleep.mode == M_SUSPEND_0 ){
#if(!MOUSE_GOLDEN_VPATTERN)					//use as golden
            while(1);     //watch dog reset
#endif
        }
        device_sleep_mode_machine( &device_sleep );
    }

    u32 wkup_sensor = ( (rc_status->no_ack == 0) && DEVICE_PKT_ASK );

	if ( M_SUSPEND_MCU_SLP & device_sleep.mode ){
    	device_info_save(rc_status, 1);    	        //Save related information to 3.3V analog register
	}
#if	MOUSE_SW_CUS
	if ( rc_status->high_end == MS_HIGHEND_250_REPORTRATE )
    	device_sleep.wakeup_time = 8;
	else
    	device_sleep.wakeup_time = 12;
#else
	device_sleep.wakeup_time = 8;
#endif
#if(CHIP_8366_A1)
    device_sleep.wakeup_src = PM_WAKEUP_TIMER;  //A1 wheel no need wkup 8ms suspend
#else
	device_sleep.wakeup_src = PM_WAKEUP_CORE | PM_WAKEUP_TIMER;  //wheel and timer can wakeup 8_ms sleep
#endif
	//Suspend, deep sleep GPIO, 32K setting
	if ( device_sleep.mode & M_SUSPEND_100MS ){
        device_sleep.wakeup_time = 100;
        device_sleep.wakeup_src = PM_WAKEUP_CORE | PM_WAKEUP_PAD | PM_WAKEUP_TIMER;
        device_sync = 0;        //ll_channel_alternate_mode ();
	}

#if MOUSE_DEEPSLEEP_EN
	else if ( M_SUSPEND_MCU_SLP & device_sleep.mode ){
		device_sleep.wakeup_time = 0;		
		device_sleep.wakeup_src = PM_WAKEUP_PAD;
        device_sync = 0;        //ll_channel_alternate_mode ();
	}
#endif
    log_event( TR_T_MS_SLEEP );
#if MOUSE_NO_HW_SIM
    device_sleep.mode = M_SUSPEND_0;
#endif
}

void mouse_task_when_rf ( void ){
    dbg_led_high;
	//clear mouse_event_data	
	//*((u32 *)(rc_status.data)) = 0;
    static u32 btn_value = 0;
    u32 map_value = RC_MAX_DATA_VALUE;
	memset(&rc_data, 0, sizeof(kb_data_t));

#if MOUSE_NO_HW_SIM
    mouse_rf_post_proc(&rc_status);
    mouse_sync_process(&rc_status);
    mouse_power_saving_process(&rc_status);
#else
    mouse_button_pull(&rc_status, MOUSE_BTN_HIGH);
    mouse_led_process(rc_status.led_define);
    mouse_rf_post_proc(&rc_status);
    mouse_sync_process(&rc_status);

    log_event( TR_T_MS_BTN );
    mouse_power_saving_process(&rc_status);
    
    btn_value = rc_button_detect(&rc_status, MOUSE_BTN_LOW);
    //if ( (rc_status.high_end != MS_HIGHEND_250_REPORTRATE) || (rc_status.loop_cnt & 1) ){
    map_value = rc_button_process_and_mapping(&rc_status);
    if(RC_MAX_DATA_VALUE != map_value){
    	memcpy(&rc_data, &btn_map_value[map_value], sizeof(kb_data_t));
    }
    //}
#if    MOSUE_BATTERY_LOW_DETECT
    static u16 batt_det_count = 0;    
    if( (rc_status.rf_mode != RF_MODE_IDLE) && (device_sleep.mode == M_SUSPEND_8MS) ){
        if ( ++batt_det_count >= mouse_batt_detect_time ){
            mouse_batt_det_process(&rc_status);
            batt_det_count = 0;
        }
    }
    else{
        batt_det_count = mouse_batt_detect_time;
    }
#endif
#endif
    rc_status.loop_cnt ++;
    dbg_led_low;
}

_attribute_ram_code_ void mouse_task_in_ram( void ){
    p_task_when_rf = mouse_task_when_rf;
    cpu_rc_tracking_en (RC_TRACKING_32K_ENABLE);
    mouse_rf_process(&rc_status);
    cpu_rc_tracking_disable;
    if (p_task_when_rf != NULL) {
       (*p_task_when_rf) ();
    }
    dbg_led_high;
#if MOUSE_SW_CUS
	if ( rc_status.high_end == MS_HIGHEND_250_REPORTRATE )
	    ll_add_clock_time (4000);
    else        
        ll_add_clock_time (8000);
#else
	ll_add_clock_time (8000);
#endif

    device_sleep_wakeup( &device_sleep );
    dbg_led_low;
}

void main_loop(void)
{
	if( MOUSE_EMI_4_FCC || (rc_status.mouse_mode == STATE_EMI) \
        || (rc_status.dbg_mode & STATE_TEST_EMI_BIT) ){
		mouse_emi_process(&rc_status);
	}
	else{
        log_event( TR_T_MS_EVENT_START );
        mouse_task_in_ram();
	}
}
