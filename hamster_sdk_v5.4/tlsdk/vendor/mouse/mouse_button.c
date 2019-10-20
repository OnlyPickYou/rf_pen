/*
 * mouse_button.c
 *
 *  Created on: Feb 12, 2014
 *      Author: xuzhen
 */

#include "../../proj/tl_common.h"
#include "../common/rf_frame.h"
#include "mouse.h"
#include "mouse_custom.h"
#include "mouse_rf.h"
#include "mouse_custom.h"
#include "mouse_button.h"
#include "trace.h"

u8 button_last;  //current button
static u8 button_pre;   //history button

rc_btn_ctrl_t rc_btn_ctrl;
//u32 btn_cnt[RC_MAX_BUTTON_VALUE] = {0};
u8 start_key_cnt = 0;
u32 rc_btn_cnt = 0;

#if(MOUSE_R150_RF_PEN || MOUSE_SIM_RF_PEN)
#define BUTTON_REPEAT_CNT	60
#else
#define BUTTON_REPEAT_CNT	20
#endif
#define BUTTON_THRESH_CNT   550

#if 0
kb_data_t btn_map_value[RC_MAX_DATA_VALUE] = {
		{1, 0, VK_PAGE_DOWN, 0, 0, 0, 0, 0},
		{1, VK_MSK_SHIFT, VK_F5, 0, 0, 0, 0, 0},
		{1, 0, VK_ESC, 0, 0, 0, 0, 0},
		{1, 0, VK_PAGE_UP, 0, 0, 0, 0, 0},
		{1, 0, VK_PERIOD, 0, 0, 0, 0, 0},
};
#else


const u8 btn_map_value[RC_MAX_DATA_VALUE][8] = {
		{1, 0, VK_PAGE_UP, 0, 0, 0, 0, 0},		//0
		{0, 0, 0, 0, 0, 0, 0, 0, },
		{1, 0, VK_PAGE_DOWN, 0, 0, 0, 0, 0},
		{0xff, 0xff, 0xff, VK_SWITCH0, 0, 0, 0, 0},
		{1, 0, VK_TAB, 0, 0, 0, 0, 0},				//4
		{1, 0, VK_VOL_DN, 0, 0, 0, 0, 0},
		{1, 0, VK_VOL_UP, 0, 0, 0, 0, 0},

		{1, 0, VK_ESC, 0, 0, 0, 0, 0},
		{1, 0, VK_B, 0, 0, 0, 0, 0},
		{1, 0, VK_TAB, 0, 0, 0, 0, 0},				//9
		{1, VK_MSK_ALT, VK_TAB,0, 0, 0, 0, 0},		//10
		{1, 0, VK_ENTER, 0, 0, 0, 0, 0},
		{0, VK_MSK_ALT, 0, 0, 0, 0, 0, 0},
};

kb_data_t btn_map_switch[2] = {
		{1, VK_MSK_SHIFT, VK_F5, 0, 0, 0, 0, 0},
		{1, (VK_MSK_LWIN | VK_MSK_LALT), VK_P, 0, 0, 0, 0, 0},
};

#endif

#if(!MOUSE_BUTTON_GPIO_REUSABILITY)
	static u32 gpio_btn_all;
	u32 gpio_btn_valid[MAX_MOUSE_BUTTON];
#endif

void rc_button_init(rc_hw_t *rc_hw)
{
    int i;
    u32 level;
    u32 spec_pin;
	for(i=0;i<MAX_MOUSE_BUTTON;i++){
#if(!MOUSE_BUTTON_GPIO_REUSABILITY)  //no gpio reusability
		spec_pin = rc_hw->button[i];
		level = (rc_hw->gpio_level_button[i] == U8_MAX);  //0xff：pullup   others:pulldown
		gpio_btn_valid[i] =	level ? 0 : spec_pin;  //1:低有效  0:高有效
		gpio_btn_all |= spec_pin;
		gpio_setup_up_down_resistor(spec_pin, level);
#endif


#if MOUSE_GPIO_FULL_RE_DEF
		gpio_set_func(spec_pin,AS_GPIO);
		gpio_set_output_en(spec_pin, 0);
		gpio_set_input_en(spec_pin, 1);
#endif
	}
}

const u32	button_seq_button_middle_click2 =	// LRM - LR - LRM - LR
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT | FLAG_BUTTON_MIDDLE) << 24) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT) << 16) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT | FLAG_BUTTON_MIDDLE) << 8) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT) << 0);

const u32	button_seq_paring =	                // LR - LRM - LR
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT | 0x80) << 16) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT | FLAG_BUTTON_MIDDLE) << 8) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT) << 0);

const u32	button_seq_emi =                    // LRM - LR - LRM - LR
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT | FLAG_BUTTON_MIDDLE | 0x80) << 24) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT) << 16) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT | FLAG_BUTTON_MIDDLE) << 8) |
		((FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT) << 0);

static inline u32 mouse_button_special_seq( u32 button, u32 multi_seq, u32 power_on, u32 paring_anytime ){
    u32 button_seq;
	u32 specail_seq;
    u32 seq = 0;
    u32  mouse_pairing_ui = rc_btn_ui.paring_ui;
    u32  mouse_emi_ui = rc_btn_ui.emi_ui;

    if( multi_seq ){
    }
    else{
        button_seq = button;
        specail_seq = power_on;
    }

    if( specail_seq ){
        if( button_seq == mouse_pairing_ui ){
            seq = BTN_INTO_PAIRING;
    	}
    	else if (button_seq == mouse_emi_ui) {
           seq = BTN_INTO_EMI;
    	}
    }
    else if ( paring_anytime && (button_seq == mouse_pairing_ui) ){
        seq = BTN_INTO_PAIRING;
    }
    
    return seq;
}

static inline void rc_button_process_test_mode( u8 *mouse_mode, u8 *dbg_mode, u32 button, u32 test_mode ){
    if( (test_mode == BTN_INTO_PAIRING) && !button ) {
        *mouse_mode = STATE_PAIRING;
	}
    else if( (test_mode == BTN_INTO_EMI) && !button ) {            
       	*mouse_mode = STATE_EMI;
    }
#if MOUSE_BUTTON_FULL_FUNCTION    
    if( (test_mode == BTN_INTO_SPECL_MODE) && (button & FLAG_BUTTON_LEFT) && (button & FLAG_BUTTON_RIGHT) ){
        *dbg_mode |= STATE_TEST_0_BIT;        
        led_cfg_t mouse_led_test_0 = { 2, 4, 3, 0};        
        mouse_led_setup( mouse_led_test_0 );
    }
    else if ( button == 0 )
        *dbg_mode &= ~STATE_TEST_0_BIT;
    
    if( (test_mode == BTN_INTO_SPECL_MODE)&& !(button & FLAG_BUTTON_LEFT) && (button & FLAG_BUTTON_RIGHT)  )
        *dbg_mode |= STATE_TEST_EMI_BIT;
    else
        *dbg_mode &= ~STATE_TEST_EMI_BIT;
    
    if( (test_mode == BTN_INTO_SPECL_MODE) && (button & FLAG_BUTTON_LEFT) && !(button & FLAG_BUTTON_RIGHT) ){
        *dbg_mode |= STATE_TEST_V_BIT;
        led_cfg_t mouse_led_test_v = { 8, 16, 3, 0};
        mouse_led_setup( mouse_led_test_v );
    }
    else
        *dbg_mode &= ~STATE_TEST_V_BIT;
#endif
}

static u8 test_mode_pending;
u32 rc_button_process(rc_status_t * rc_status)
{   
    u32 button = button_last;
    static u16 btn_lr_cnt = 0; 
    
    if ( button_pre || button_last ){
        if( (button & FLAG_BUTTON_LEFT) && (button & FLAG_BUTTON_RIGHT) )
            btn_lr_cnt++;
        else
            btn_lr_cnt = 0;
    }
    
    if (button_pre != button) {            //new event
        u32 multi_seq = (btn_lr_cnt > 800);
        u32 paring_any_time = !rc_cust_paring_only_pwon && device_never_linked && !test_mode_pending;
        test_mode_pending |= mouse_button_special_seq( \
            button, multi_seq, (rc_status->mouse_mode == STATE_POWERON), paring_any_time );
        
    	rc_button_process_test_mode( &rc_status->mouse_mode, &rc_status->dbg_mode, button, test_mode_pending);

       if( !button )
            test_mode_pending = 0;    
    }
	button_pre = button;
    return button;
}

#if 0
static u32 rc_btn_value_mapping(rc_status_t *rc_status, u32 btn_last, u32 btn_prev)
{
   u32 map_value = RC_MAX_DATA_VALUE;
   static u8 long_left_cnt = 0;

   test_mode_pending = 0;

	if(btn_prev == RC_ONLY_LEFT_VALUE){
		if(btn_cnt[RC_ONLY_LEFT_VALUE - 1] == RC_BUTTON_OVER_THRESH){
			if(long_left_cnt){
				map_value = RC_DATA_LONG_LEFT_OVER;
			}
			else{
				map_value = RC_DATA_LONG_LEFT;
			}
			gpio_write(M_HW_LED_CTL, 1);
			long_left_cnt = (long_left_cnt + 1) & 1;
		}
	}
	else if(btn_prev == RC_ONLY_RIGHT_VALUE){
		if(btn_cnt[RC_ONLY_RIGHT_VALUE - 1] == RC_BUTTON_OVER_THRESH){
			map_value = RC_DATA_LONG_RIGHT;
			gpio_write(M_HW_LED_CTL, 1);
		}
	}

	else if(btn_prev == RC_ONLY_LEFT_VALUE){
	    u32 multi_seq = (btn_cnt[RC_ONLY_LEFT_VALUE - 1] > 800);
	    u32 paring_any_time = !rc_cust_paring_only_pwon && device_never_linked && !test_mode_pending;
	    test_mode_pending |= mouse_button_special_seq( \
	    		btn_last, multi_seq, (rc_status->mouse_mode == STATE_POWERON), paring_any_time );
	    rc_button_process_test_mode( &rc_status->mouse_mode, &rc_status->dbg_mode, btn_last, test_mode_pending);
	}
	//btn_cnt[btn_prev - 1] = 0;
	return map_value;
}
#else
static u32 rc_btn_value_mapping(rc_status_t *rc_status, u32 btn_last, u32 btn_prev, u32 btn_status, u32 deepsleep)
{

    static u32 rc_btn_proc = 0;
    static u8 tab_repeat_flag = 0;
	u32 map_value = RC_MAX_DATA_VALUE;

	u32 i = 0;
	static u8 TabPressFlag = 0;
	static u8 EscPressFlag = 0;

	static u8 tab_repeat_cnt = 0;

	test_mode_pending = 0;
	rc_btn_proc += 8;

#if(MOUSE_SIM_RF_PEN)
	if( TabPressFlag && (abs(rc_btn_proc - rc_btn_ctrl.LastPreTick) > BUTTON_THRESH_CNT )){
		TabPressFlag = 0;
		if(RC_BUTTON_RELEASE == btn_status){
			return rc_btn_ctrl.KeyNumSave[0];
		}
	}

	if(EscPressFlag && (abs(rc_btn_proc - rc_btn_ctrl.LastEscTick) > BUTTON_THRESH_CNT )){
		EscPressFlag = 0;
		return rc_btn_ctrl.KeyNumSave[1];
	}

	if(RC_BUTTON_FIRST == btn_status){
		if( RC_ONLY_RF_LED_VALUE == btn_last ){
			EscPressFlag = 0;
			if(!TabPressFlag){
				TabPressFlag = 1;
				rc_btn_ctrl.KeyNumSave[0] = RC_DATA_TAB_OVR;
			}
			else{
				if (abs(rc_btn_proc - rc_btn_ctrl.LastPreTick) <= BUTTON_THRESH_CNT){
					TabPressFlag = 0;
					//if(start_key_cnt){
					map_value = RC_DATA_DOUBLE_TAB;
					//}
				}
			}
			if(deepsleep){
				rc_btn_ctrl.KeyNumSave[0] = RC_DATA_TAB_OVR;
				TabPressFlag = 1;
			}

			rc_btn_ctrl.LastPreTick = rc_btn_proc;
		}
		else if(RC_ONLY_START_VALUE == btn_last){
			TabPressFlag = 0;						//release
			if(!EscPressFlag){
				EscPressFlag = 1;
			}
			start_key_cnt = (start_key_cnt + 1) & 1;
			rc_btn_ctrl.KeyNumSave[1] = RC_DTAT_START + (start_key_cnt << 2);
			//map_value = RC_DTAT_START + (start_key_cnt << 2);
			rc_btn_ctrl.LastEscTick = rc_btn_proc;
			if(deepsleep){
				map_value = rc_btn_ctrl.KeyNumSave[1];
				EscPressFlag = 0;
			}

		}
		else{
			TabPressFlag = 0;
			EscPressFlag = 0;
			if(RC_ONLY_UP_VALUE == btn_last){
				map_value = RC_DATA_UP;
			}
			else if(RC_ONLY_DOWN_VALUE == btn_last){
				map_value = RC_DATA_DOWN;
			}
			else if(RC_ONLY_VOL_DOWN_VALUE == btn_last){
				map_value = RC_DATA_VOL_DOWN;
			}
			else if(RC_ONLY_VOL_UP_VALUE == btn_last){
				map_value = RC_DATA_VOL_UP;
			}
		}
	}
	else if(RC_BUTTON_KEEP == btn_status)
	{
		if( tab_repeat_flag && (RC_ONLY_RF_LED_VALUE == btn_last) ){

			map_value = RC_DATA_REPEAT_ALT;
		}

	}
	else if(RC_BUTTON_REPEAT == btn_status){
		TabPressFlag = 0;
		EscPressFlag = 0;
		if(RC_ONLY_RF_LED_VALUE == btn_last){
			tab_repeat_flag = 1;
			map_value = (tab_repeat_cnt++ & 1) ? RC_DATA_LONG_TAB : RC_DATA_REPEAT_ALT;
		}
		else{
			if(RC_ONLY_UP_VALUE == btn_last){
				map_value = RC_DATA_UP;
			}
			else if(RC_ONLY_DOWN_VALUE == btn_last){
				map_value = RC_DATA_DOWN;
			}
			else if(RC_ONLY_VOL_DOWN_VALUE == btn_last){
				map_value = RC_DATA_VOL_DOWN;
			}
			else if(RC_ONLY_VOL_UP_VALUE == btn_last){
				map_value = RC_DATA_VOL_UP;
			}
			/*
			for(i=MAX_MOUSE_BUTTON-1; i>=0; i--)
			{
				if( btn_last == BIT(i) ){
					map_value = i;
					break;
				}
			}
			*/
		}
	}
	else{
		tab_repeat_flag = 0;
	}
#elif(MOUSE_R150_RF_PEN)
	if( TabPressFlag && (abs(rc_btn_proc - rc_btn_ctrl.LastPreTick) > BUTTON_THRESH_CNT )){
		TabPressFlag = 0;
		if(RC_BUTTON_RELEASE == btn_status){
			return rc_btn_ctrl.KeyNumSave[0];
		}
	}

	if(EscPressFlag && (abs(rc_btn_proc - rc_btn_ctrl.LastEscTick) > BUTTON_THRESH_CNT )){
		EscPressFlag = 0;
		return rc_btn_ctrl.KeyNumSave[1];
	}

	if(RC_BUTTON_FIRST == btn_status){
		if( RC_ONLY_TAB_VALUE == btn_last ){
			EscPressFlag = 0;
			if(!TabPressFlag){
				TabPressFlag = 1;
				rc_btn_ctrl.KeyNumSave[0] = RC_DATA_TAB_OVR;
			}
			else{
				if (abs(rc_btn_proc - rc_btn_ctrl.LastPreTick) <= BUTTON_THRESH_CNT){
					TabPressFlag = 0;
					//if(start_key_cnt){
					map_value = RC_DATA_DOUBLE_TAB;
					//}
				}
			}

			if(deepsleep){
				rc_btn_ctrl.KeyNumSave[0] = RC_DATA_TAB_OVR;
				TabPressFlag = 1;
			}
			rc_btn_ctrl.LastPreTick = rc_btn_proc;
		}
		else if(RC_ONLY_START_VALUE == btn_last){
			TabPressFlag = 0;						//release
			if(!EscPressFlag){
				EscPressFlag = 1;
			}
			start_key_cnt = (start_key_cnt + 1) & 1;
			rc_btn_ctrl.KeyNumSave[1] = RC_DTAT_START + (start_key_cnt << 2);
			//map_value = RC_DTAT_START + (start_key_cnt << 2);
			rc_btn_ctrl.LastEscTick = rc_btn_proc;

			if(deepsleep){
				map_value = rc_btn_ctrl.KeyNumSave[1];
				EscPressFlag = 0;
			}

		}
		else{
			TabPressFlag = 0;
			EscPressFlag = 0;

			if(RC_ONLY_UP_VALUE == btn_last){
				map_value = RC_DATA_UP;
			}
			else if(RC_ONLY_RF_LED_VALUE == btn_last){
				map_value = RC_DATA_RF_LED;
			}
			else if(RC_ONLY_DOWN_VALUE == btn_last){
				map_value = RC_DATA_DOWN;
			}
			else if(RC_ONLY_VOL_DOWN_VALUE == btn_last){
				map_value = RC_DATA_VOL_DOWN;
			}
			else if(RC_ONLY_VOL_UP_VALUE == btn_last){
				map_value = RC_DATA_VOL_UP;
			}


//			for(i=MAX_MOUSE_BUTTON-1; i>=0; i--)
//			{
//				if( btn_last == BIT(i) ){
//					map_value = i;
//					break;
//				}
//			}
		}
	}
	else if(RC_BUTTON_KEEP == btn_status)
	{
		//rc_btn_proc -= 6;
		if( tab_repeat_flag && (RC_ONLY_TAB_VALUE == btn_last) ){
			map_value = RC_DATA_REPEAT_ALT;
		}

	}
	else if(RC_BUTTON_REPEAT == btn_status){
		TabPressFlag = 0;
		EscPressFlag = 0;
		if(RC_ONLY_START_VALUE == btn_last){
			map_value = RC_DATA_LONG_START;
		}
		else if(RC_ONLY_TAB_VALUE == btn_last){
			tab_repeat_flag = 1;
			map_value = (tab_repeat_cnt++ & 1) ? RC_DATA_LONG_TAB : RC_DATA_REPEAT_ALT;
		}
		else{

			if(RC_ONLY_UP_VALUE == btn_last){
				map_value = RC_DATA_UP;
			}
			else if(RC_ONLY_RF_LED_VALUE == btn_last){
				map_value = RC_DATA_RF_LED;
			}
			else if(RC_ONLY_DOWN_VALUE == btn_last){
				map_value = RC_DATA_DOWN;
			}
			else if(RC_ONLY_VOL_DOWN_VALUE == btn_last){
				map_value = RC_DATA_VOL_DOWN;
			}
			else if(RC_ONLY_VOL_UP_VALUE == btn_last){
				map_value = RC_DATA_VOL_UP;
			}

//			for(i=MAX_MOUSE_BUTTON-1; i>=0; i--)
//			{
//				if( btn_last == BIT(i) ){
//					map_value = i;
//					break;
//				}
//			}
		}
	}
	else{
		tab_repeat_flag = 0;
	}
#elif(MOUSE_R250_RF_PEN)
	static u8 repeat_cnt = 0;
	if( (RC_BUTTON_FIRST == btn_status) || (RC_BUTTON_REPEAT == btn_status) ){
		if(RC_ONLY_UP_VALUE == btn_last){
			if((RC_BUTTON_REPEAT == btn_status) && (++repeat_cnt > 2)){
				map_value = RC_DATA_UP;
				repeat_cnt = 0;
			}
			else if(RC_BUTTON_FIRST == btn_status){
				map_value = RC_DATA_UP;
			}
		}
		else if(RC_ONLY_RF_LED_VALUE == btn_last){
			map_value = RC_DATA_RF_LED;
		}
		else if(RC_ONLY_DOWN_VALUE == btn_last){
			if(RC_BUTTON_REPEAT == btn_status && (++repeat_cnt > 2)){
				map_value = RC_DATA_DOWN;
				repeat_cnt = 0;
			}
			else if(RC_BUTTON_FIRST == btn_status){
				map_value = RC_DATA_DOWN;
			}
		}
		else if(RC_ONLY_START_VALUE == btn_last){
			if(RC_BUTTON_REPEAT == btn_status){
				if(++repeat_cnt > 5 ){
					map_value = RC_DATA_TAB_OVR;
					repeat_cnt = 0;
				}
			}
			else{
				map_value = RC_DTAT_START + (start_key_cnt << 2);
				start_key_cnt = (start_key_cnt + 1) & 1;
			}

		}
		else if(RC_ONLY_VOL_DOWN_VALUE == btn_last){
			map_value = RC_DATA_VOL_DOWN;
		}
		else if(RC_ONLY_VOL_UP_VALUE == btn_last){
			map_value = RC_DATA_VOL_UP;
		}
	}
	else if(RC_BUTTON_RELEASE == btn_status){
		repeat_cnt = 0;
	}
#endif


	//u32 multi_seq = (btn_cnt[RC_ONLY_LEFT_VALUE - 1] > 800);
	u32 multi_seq = 0;
	u32 paring_any_time = !rc_cust_paring_only_pwon && device_never_linked && !test_mode_pending;
	test_mode_pending |= mouse_button_special_seq( \
			btn_last, multi_seq, (rc_status->mouse_mode == STATE_POWERON), paring_any_time );
	rc_button_process_test_mode( &rc_status->mouse_mode, &rc_status->dbg_mode, btn_last, test_mode_pending);

	return map_value;
}
#endif

u32 rc_button_process_and_mapping(rc_status_t * rc_status, u32 deepsleep)
{
	u32 map_value = RC_MAX_DATA_VALUE;
    u32 button = button_last;
    u32 button_flag = RC_BUTTON_RELEASE;

    if(RC_INVALID_VALUE == button_pre ){
        /*detect whether is new button is pressed */
    	if( button_last ){
    		button_flag = RC_BUTTON_FIRST;
    		if(button_last == RC_ONLY_RF_LED_VALUE){
#if(SWS_CONTROL_LED2_EN)
    		   	gpio_write(M_HW_LED2_CTL , 1);
#endif
    		}
    		else{
    			gpio_write(M_HW_LED_CTL, 1);	//enable LED
    		}
    	}
    }
    else{
    	if(button_pre == button_last){
		/*detect whether is button is long pressed */
    		rc_btn_cnt++;
    		if(button_last == RC_ONLY_RF_LED_VALUE){
    			gpio_write(M_HW_LED_CTL, 1);	//enable LED
#if(SWS_CONTROL_LED2_EN)
    		   	gpio_write(M_HW_LED2_CTL , 1);
#endif
    		}
    		if(0 == (rc_btn_cnt % BUTTON_REPEAT_CNT) ){
    			button_flag = RC_BUTTON_REPEAT;
    		}
    		else{
    			button_flag = RC_BUTTON_KEEP;
    		}
    	}
    	else if(RC_INVALID_VALUE == button_last ){
		/*detect whether is button is released */
    		rc_btn_cnt = 0;
    		gpio_write(M_HW_LED_CTL, 0);	//shut down LED
#if(SWS_CONTROL_LED2_EN)
        	gpio_write(M_HW_LED2_CTL , 0);
#endif

    	}
    }

	map_value = rc_btn_value_mapping(rc_status, button_last, button_pre, button_flag, deepsleep);

	button_pre = button;
    return map_value;
}


u32 rc_button_process_emi(s8 *chn_idx, u8 *test_mode_sel, u32 btn_pro_end)
{ 
    u32 cmd = 0;    
    if (button_pre != button_last) {     //new event
        cmd = 0x80;
        if (!(button_pre & FLAG_BUTTON_MIDDLE) && (button_last & FLAG_BUTTON_MIDDLE)) {                
            *test_mode_sel = (*test_mode_sel+1) & 3;  //mode change: carrier, cd, rx, tx
        }
        else if (!(button_pre & FLAG_BUTTON_LEFT) && (button_last & FLAG_BUTTON_LEFT)) {
            *chn_idx += 1;               //channel up
        }
        else if (!(button_pre & FLAG_BUTTON_RIGHT) && (button_last & FLAG_BUTTON_RIGHT)) {                
            *chn_idx -= 1;               //channel down
        }
        else {
            cmd &= 0x0f;
        }
        
        if (*chn_idx < 0) {
            *chn_idx = 2;
        }
        else if (*chn_idx > 2) {
            *chn_idx = 0;
        }
    }
    if( btn_pro_end )
        button_pre = button_last;
    return cmd;
}

static inline u8 mouse_button_debounce(u8 btn_cur, u8 btn_last, u8 debouce_len){
    static u8 s_btn_cnt = 0;
	if(	s_btn_cnt >= debouce_len && btn_last != btn_cur ){
		btn_last = btn_cur;
		s_btn_cnt = 0;
	}else{
		if(btn_last != btn_cur)
			s_btn_cnt ++;
		else
			s_btn_cnt = 0;
	}
    return btn_last;
}

void mouse_button_pull(rc_status_t  * rc_status, u32 prepare_level ){
    u32 pull_level = 0;
    u32 spec_pin = 0;
    int i = 0;
    for ( i = MAX_MOUSE_BUTTON - 1; i >= 0; i-- ){
        spec_pin = rc_status->hw_define->button[i];
        pull_level = MOUSE_BTN_HIGH && rc_status->hw_define->gpio_level_button[i];
        if ( pull_level == prepare_level ){
            gpio_setup_up_down_resistor( spec_pin, pull_level );            
        }
    }
}

/// \param detect_level - 0: detect low after button been pullup,  else: detect high after button been pulldown
/// \mouse should detect high, then detect low
u32 rc_button_detect(rc_status_t  * rc_status, u32 detect_level)
{
    static u8 btn_cur = 0;
    u32 pull_level = 0;
    u32 spec_pin = 0;
    int i = 0;
    for ( i = MAX_MOUSE_BUTTON - 1; i >= 0; i-- ){
        spec_pin = rc_status->hw_define->button[i];
        pull_level = MOUSE_BTN_HIGH && rc_status->hw_define->gpio_level_button[i];
        if ( pull_level != detect_level ){
            if( !gpio_read(spec_pin) ^ !pull_level ){
                btn_cur |= (1<<i);
            }
        }
    }

    if ( detect_level )
        return 0;
    
#if 0
    u32 debouce_len = (rc_status->mouse_mode == STATE_POWERON) ? 0 : 3;
    btn_real = mouse_button_debounce( btn_cur, button_last, debouce_len );
#endif
    u8 btn_real = btn_cur;
    btn_cur = 0;
    
    //mask button event on power-on
    static u8 btn_power_on_mask = 0x0;
    if ( !btn_real )
        btn_power_on_mask = 0xff;
    
    //rc_status->data->btn = btn_real & btn_power_on_mask;
    button_last = btn_real;
    return button_last;
}

u32 rc_button_pull_and_detect(rc_status_t  * rc_status){
    mouse_button_pull(rc_status, MOUSE_BTN_HIGH);
    WaitUs(100);
    return rc_button_detect(rc_status, MOUSE_BTN_LOW);
}

