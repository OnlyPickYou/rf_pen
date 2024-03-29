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


u32 btn_cnt[RC_MAX_BUTTON_VALUE] = {0};

kb_data_t btn_map_value[RC_MAX_DATA_VALUE] = {
		{1, 0, VK_PAGE_DOWN, 0, 0, 0, 0, 0},
		{1, VK_MSK_SHIFT, VK_F5, 0, 0, 0, 0, 0},
		{1, 0, VK_ESC, 0, 0, 0, 0, 0},
		{1, 0, VK_PAGE_UP, 0, 0, 0, 0, 0},
		{1, 0, VK_PERIOD, 0, 0, 0, 0, 0},
};

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
    static u32  button_seq = 0;
    static u32  button_seq1 = 0;
    static u32  button_seq2 = 0;    
    u32 specail_seq;
    u32 seq = 0;
    u32  mouse_pairing_ui = rc_btn_ui.paring_ui;
    u32  mouse_emi_ui = rc_btn_ui.emi_ui;

    if( multi_seq ){
        if( power_on )
            button |= 0x80;
        button_seq = (button_seq << 8) | (button_seq1>>24);
        button_seq1 = (button_seq1 << 8) | (button_seq2>>24);
        button_seq2 = (button_seq2 << 8) | button;
        specail_seq = (button_seq_button_middle_click2 == button_seq1)\
               && (button_seq_button_middle_click2 == button_seq2);
        mouse_pairing_ui = button_seq_paring;
        mouse_emi_ui = button_seq_emi;
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
        else if ( button_seq == button_seq1 ){
            seq = BTN_INTO_SPECL_MODE;
            button_seq2 = 0xff;     //next spcail sequence must be the same with the last one
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


u32 rc_button_process_and_mapping(rc_status_t * rc_status)
{
	u32 map_value = RC_MAX_DATA_VALUE;
    u32 button = button_last;

    static u32 btn_proc_cnt = 0;

    /*  invalid button value, return */
    if( RC_ONLY_MID_VALUE == button_pre ){
    	if( button == button_pre ){
    		gpio_write(M_HW_LED_CTL, 1);
#if (SWS_CONTROL_LED2_EN)
    		gpio_write(M_HW_LED2_CTL, 1);
#endif
    	}else{
    		gpio_write(M_HW_LED_CTL, 0);
#if (SWS_CONTROL_LED2_EN)
    		gpio_write(M_HW_LED2_CTL, 0);
#endif
    	}
		button_pre = button;
		return map_value;
    }

    /* button_last is valid, and button_pre is the same as button last */
    if( button_pre != button_last ){
    	if( RC_INVALID_VALUE == button_last ){
    	//	map_value = rc_btn_value_mapping(rc_status, button_last, button_pre);
    		if((btn_cnt[button_pre - 1] > 0) && (btn_cnt[button_pre - 1] < RC_BUTTON_OVER_THRESH)){
    			if(button_pre == RC_ONLY_LEFT_VALUE){
    				map_value = RC_DATA_SHORT_LEFT;
    			}else if(button_pre == RC_ONLY_RIGHT_VALUE){
    				map_value = RC_DATA_SHORT_RIGHT;
    			}
    		}
    		else{
    			gpio_write(M_HW_LED_CTL, 0);
    		}
    		btn_cnt[button_pre - 1] = 0;
    	}
    }
    else{
    	if(RC_INVALID_VALUE != button_last){
    		btn_cnt[button_last - 1]++;		//the button has been pressed
    		map_value = rc_btn_value_mapping(rc_status, button_last, button_pre);
    	}
    }

	button_pre = button;
	btn_proc_cnt++;
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

