/*
 * vacuum_button.c
 *
 *  Created on: 2015-11-16
 *      Author: Administrator
 */
#include "../../proj/tl_common.h"

#include "vacuum_controller.h"
#include "../common/rf_frame.h"
#include "vacuum_controller_rf.h"
#include "vacuum_controller_button.h"
#include "vacuum_controller_batt.h"
//#include "vacuum_controller_iouart.h"


btn_status_t 	btn_status;
u8 	allow_repair_fun_en = 0;
u32 ctrl_btn[3] = {BTN_UP, BTN_POWER, BTN_DOWN};		//GP10,GP7,GP8
extern u16 custom_binding;

#if(USE_CURRENT_VERSION_1P6)
repair_and_reset_t  repair_and_reset = {

	0,
	0,

	0,
	0,
	0,

	1,				//auto_pair_enable
	0,				//repair(mannual pair),
	0,				//repair cnt
	0,
};
#else
repair_and_reset_t  repair_and_reset = {

	0,
	0,

	0,
	0,

	1,				//auto_pair_enable
	0,				//repair(mannual pair),
	0,				//repair cnt
	0,
};
#endif

btn_ui_t btn_ui ={

		VACUUM_BTN_POWER,
		VACUUM_BTN_UP | VACUUM_BTN_POWER,
//		VACUUM_BTN_UP | VACUUM_BTN_DOWN,
//		VACUUM_BTN_UP | VACUUM_BTN_POWER,
		VACUUM_BTN_POWER | VACUUM_BTN_DOWN,
};

vcEvt_buf_t 		vcEvt_buf;
extern vc_data_t	vc_event;
#if(WITH_SPELL_PACKET_EN)
extern vc_data_t	 vc_s_event;
#endif

extern u8 vbat_2p9_flag;

u8 PowerOn = 0;
u8 FactRestEn = 0;
u8 PairConfirmEn = 0;

u8 ConfirmPair = 0;
/*
 * function:40ms检测4次按键进行滤波
 * 4次的结果一样，并与上次的按键检测结果不同，则表示有按键变化(按下或释放)
 */
u8 KeyLastPress;
u8 KeyCurPress;

u8 KeyRelease;

// use for double key function
u8 DelayT = 10;				// delay time = DelayT * main loop time (10ms)
u8 DelayPressCnt = 0;
u8 DelayReleaseCnt = 0;

u8 DoubleKeyFlag = 0;
u8 DoubleKeySendFlag = 0;
u8 DoubleKeySave = 0;

u8 OneKeyPressSave = 0;
u8 OneKeyReleaseSave = 0;
u8 OneKeyStatusSave = 0;

u8 key_release_flg = 0;
u8 btn_filter_last = 0;
u8 btn_map[5] = {VC_UP, VC_POWER, VC_DOWN, COMBINE_KEY1_FUNC, COMBINE_KEY2_FUNC};
_attribute_ram_code_ u8 btn_debounce_filter(u8 *btn_v)
{
	u8 change = 0;

	for(int i=3; i>0; i--){
		btn_status.btn_history[i] = btn_status.btn_history[i-1];
	}
	btn_status.btn_history[0] = *btn_v;

	if(  btn_status.btn_history[0] == btn_status.btn_history[1] && btn_status.btn_history[1] == btn_status.btn_history[2] && \
		btn_status.btn_history[2] == btn_status.btn_history[3] && btn_status.btn_history[0] != btn_filter_last ){
		change = 1;
#if (WITH_RELEASE_KEY_FUNC)

		if(!btn_status.btn_history[0]){		//With invalid value，button is released
			key_release_flg = 1;
			for(int i=0; i<3; i++){
				if(btn_filter_last &  BIT(i)){
					KeyRelease = btn_map[i];
				}
			}
			btn_status.btn_not_release = 0;

		}
		else{								//button is pressed
			key_release_flg = 0;
			for(int i=0; i<3; i++){
				if(btn_filter_last & BIT(i)){
					KeyLastPress = btn_map[i];
				}
//				if(btn_status.btn_history[0] &  BIT(i)){
//					KeyCurPress = btn_map[i];
//				}
			}
			btn_status.btn_not_release = 1;
		}

#else
		if(btn_filter_last == 0){
			change = (btn_status.btn_not_release) ? 0 : change;
			btn_status.btn_not_release = (!btn_status.btn_not_release) ? 1 : btn_status.btn_not_release;
		}
		else if(btn_filter_last != 0 && btn_status.btn_history[0] == 0){
			btn_status.btn_not_release = 0;
		}
#endif
		btn_filter_last = btn_status.btn_history[0];
	}

	return change;
}

u8 btn_press;
u8 key_status = 0;		// press , repeat , release


u8 vc_detect_button(int read_key)
{
	u8 btn_changed,i;
	memset(&vc_event,0,sizeof(vc_data_t));			//clear vc_event
	memset(&vc_s_event, 0, sizeof(vc_data_t));		//clear vc_s_event
	//vc_event.cnt = 0;
	btn_press = 0;
	for(i=0; i<MAX_BTN_SIZE; i++){
		if(BTN_VALID_LEVEL != !gpio_read(ctrl_btn[i])){
			btn_press |= BIT(i);
		}
	}

	if( !km_dat_sending && !btn_press){					//no button, no rf event, then detect battery
		batt_detect_and_fliter();
	}

#if(WITH_KEYCODE_SEND)
//	if(btn_press == btn_ui.combine_key1_ui){				//up + down
//		btn_press = 0x08;
//		DoubleKeyFuncEn = 1;
//	}
//	else if(btn_press == btn_ui.combine_key2_ui){		//up + power
//		btn_press = 0x10;
//		DoubleKeyFuncEn = 1;
//	}

	if(btn_press ==  btn_ui.combine_key3_ui){			//power + down
		ConfirmPair = 1;
	}
	else{
		ConfirmPair = 0;
	}
#endif

	btn_changed = btn_debounce_filter(&btn_press);




#if(WITH_REPEAT_FUNC)					//support key repeat function
	if(btn_changed){
		key_repeat.key_repeat_cnt = 0;
		key_repeat.key_change_flg = KEY_CHANGE;

	}
	else{
		if(key_repeat.key_change_flg == KEY_CHANGE){
			key_repeat.key_change_flg = KEY_SAME;
		}
		else if(key_repeat.key_change_flg == KEY_SAME && !ConfirmPair){			// repeat key without confirm pair function
			if(repair_and_reset.poweron_reset == 2 && repair_and_reset.r_cnt < 410){
				//press "POWER" key then power on within 3.5s, no repeat function
			}
			else{
				key_repeat.key_repeat_cnt++;
				if(key_repeat.key_repeat_cnt > REPEAT_TIME_THRESH ){
					btn_changed = 1;
					key_repeat.key_repeat_cnt = 0;
				}
			}
		}
	}
#endif


	if(btn_changed && read_key){
		vcEvt_buf.value[vcEvt_buf.wptr & 7] = btn_press;
		vcEvt_buf.wptr++;
		//buff full, cover the oldest vc_data
		if( ((vcEvt_buf.wptr - vcEvt_buf.rptr) & 15) > (MAX_BUF_SIZE - 1) ){
			vcEvt_buf.rptr ++;
		}

		u8 btn_value = vcEvt_buf.value[vcEvt_buf.rptr & 7];
		vcEvt_buf.rptr++;

		for(i=0; i<3; i++){
			if(btn_value & BIT(i)){
				vc_event.keycode[vc_event.cnt] = btn_map[i];
				vc_event.cnt++;
			}
		}

		return 1;
	}
	if(vcEvt_buf.wptr == vcEvt_buf.rptr){
		return 0;
	}

}
extern int km_dat_sending;
void vc_double_key_proc(void)
{
	//if power on detect combined key is up + down

	if( (VC_POWER == vc_event.keycode[0] && VC_DOWN == vc_event.keycode[1] ) \
			|| (VC_DOWN == vc_event.keycode[0] && VC_POWER  == vc_event.keycode[1])){
		repair_and_reset.m_cnt ++;
		repair_and_reset.m_flg = 1;

		DoubleKeyFlag = 0;
		//km_dat_sending = 0;
		//vc_event.cnt = 0;	//clear,did not sent data
	}

	else if((VC_DOWN == vc_event.keycode[0] && VC_UP == vc_event.keycode[1] ) \
			|| (VC_UP == vc_event.keycode[0] && VC_DOWN  == vc_event.keycode[1])){

		//UP + DOWN
		DoubleKeyFlag = 1;
		DoubleKeySave = COMBINE_KEY1_FUNC;
		vc_event.keycode[0] = COMBINE_KEY1_FUNC;
		vc_event.keycode[1] = 0;

	}
	else if((VC_POWER == vc_event.keycode[0] && VC_UP == vc_event.keycode[1] ) \
			|| (VC_UP == vc_event.keycode[0] && VC_POWER  == vc_event.keycode[1])){

		//POWER + UP
		DoubleKeyFlag = 1;
		DoubleKeySave = COMBINE_KEY2_FUNC;
		vc_event.keycode[0] = COMBINE_KEY2_FUNC;
		vc_event.keycode[1] = 0;

	}
}

u8 get_repair_allow(void )
{
	allow_repair_fun_en = (custom_binding == U16_MAX || custom_binding == 0 ) ? 1 : 0;
	return allow_repair_fun_en;
}

extern u8* kb_rf_pkt ;
#if(USE_CURRENT_VERSION_1P6)
extern u32 custom_binding;
extern u32 get_device_id;
void repair_info_load(void)
{
	vc_status.mode_link = 0;
	vc_status.vc_mode  = STATE_PAIRING;
	rf_set_power_level_index (vc_cust_tx_power);
//	rf_set_tx_pipe (PIPE_PARING);


	if(repair_and_reset.repair){
		if( vc_status.dongle_id != 0 ){		//confirm pair
			rf_set_tx_pipe(PIPE_VACUUM);
			pkt_conf.did = pkt_km.did;
			kb_rf_pkt = (u8 *)&pkt_conf;

		}
		else{								//manual pair
			rf_set_tx_pipe(PIPE_PARING);
			rf_set_access_code1 ( U32_MAX );
			pkt_manual.did = pkt_pairing.did;
			kb_rf_pkt = (u8*)&pkt_manual;
			//rf_set_tx_pipe (PIPE_VACUUM);

		}
	}										//factory reset
	else if(repair_and_reset.factory_reset \
			&& custom_binding != U32_MAX && custom_binding != 0){
		//rf_set_tx_pipe(PIPE_VACUUM);
		pkt_conf.type = FRAME_TYPE_VACUUM_RESET;
		pkt_conf.did = pkt_km.did;
		kb_rf_pkt = (u8 *)&pkt_conf;
	}
	else if(repair_and_reset.auto_pair){			//auto pair
		kb_rf_pkt = (u8 *)&pkt_pairing;
		rf_set_tx_pipe(PIPE_PARING);
		rf_set_access_code1 ( U32_MAX );
		rf_set_power_level_index(vc_cust_tx_power_paring);
	}

	vc_status.loop_cnt = KB_PARING_POWER_ON_CNT;

}


void callback_auto_paring(void)
{

	repair_and_reset.auto_pair = 1;

#if (!TEST_TX_PAIR_MODE)
	set_device_id_in_firmware(0);		//old rx id -> 0
#endif

	repair_info_load();

}

void btn_pair_confirm_detect(void)				//按了50次power键，进pair confirm
{
	if(repair_and_reset.repair_cnt > 50){
		repair_and_reset.repair_cnt = 0;
		if(repair_and_reset.auto_pair){
			repair_and_reset.repair = 1;
			repair_info_load();
		}
	}
}

void btn_mannual_pair_detect(void)
{
	if(repair_and_reset.m_flg){
		if(repair_and_reset.m_cnt++ > MANNUAL_PARING_CNT){
			repair_and_reset.m_cnt = 0;
			repair_and_reset.m_flg = 0;

			repair_and_reset.repair = 1;
			repair_info_load();
		}
	}

}


//u32 factory_reset_tick;
void btn_factory_reset_detect(void)
{
	if(repair_and_reset.poweron_reset == 2 && repair_and_reset.r_cnt++ < 340){
		if(RESET_SEQ == KEYCODE_TO_VALUE){
			//callback_auto_paring();
			repair_and_reset.r_cnt = 0;
			repair_and_reset.poweron_reset = 0;
			repair_and_reset.factory_reset = 1;

			repair_info_load();

		}
	}
	else{
		repair_and_reset.poweron_reset = 0;
	}
}
#else
void repair_info_load(void)
{
	vc_status.mode_link = 0;

	kb_rf_pkt = (u8*)&pkt_pairing;				//paring packet
	rf_set_tx_pipe (PIPE_PARING);
	rf_set_access_code1 ( U32_MAX );

	vc_status.vc_mode  = STATE_PAIRING;

	if(repair_and_reset.auto_pair)
		rf_set_power_level_index (vc_cust_tx_power_paring);
	else
		rf_set_power_level_index (vc_cust_tx_power);

	vc_status.loop_cnt = KB_PARING_POWER_ON_CNT;
}

void callback_auto_paring(void)
{

	repair_and_reset.auto_pair = 1;

#if (!TEST_TX_PAIR_MODE)

#if(MANUAL_PAIR_EN)
	if(vbat_2p9_flag){
		set_device_id_in_firmware(0);	//old rx id -> 0
	}
#else
	set_device_id_in_firmware(0);		//old rx id -> 0
#endif

#endif

	repair_info_load();

}

void btn_pair_confirm_detect(void)				//按了50次power键，进pair confirm
{
	if(repair_and_reset.repair_cnt > POWER_KEY_PRESS_CNT && get_repair_allow()){
		repair_and_reset.repair_cnt = 0;
//		if(repair_and_reset.auto_pair){
#if(MANUAL_PAIR_EN)
			if(vbat_2p9_flag){
				set_device_id_in_firmware(vc_status.dongle_id);
				if(custom_binding != U16_MAX && custom_binding != 0){
					vc_event.cnt = 1;
					btn_status.btn_new = 1;
					vc_event.keycode[0] = VC_POWER;
				}
			}
#else
			set_device_id_in_firmware(vc_status.dongle_id);
			if(custom_binding != U16_MAX && custom_binding != 0){
				vc_event.cnt = 1;
				btn_status.btn_new = 1;
				vc_event.btn_ctrl = KEY_PRESSED;
				vc_event.keycode[0] = VC_POWER;
			}
#endif
			//repair_and_reset.repair = 1;
			//repair_info_load();
		//}
	}
}

void btn_mannual_pair_detect(void)
{
	if(repair_and_reset.m_flg){
		if(repair_and_reset.m_cnt++ > MANNUAL_PARING_CNT && get_repair_allow()){
			PairConfirmEn = 1;
			repair_and_reset.m_cnt = 0;
			repair_and_reset.m_flg = 0;
#if(MANUAL_PAIR_EN)
			if(vbat_2p9_flag){
				set_device_id_in_firmware(vc_status.dongle_id);
				if(custom_binding != U16_MAX && custom_binding != 0){
					vc_event.cnt = 1;
					btn_status.btn_new = 1;
					vc_event.keycode[0] = VC_POWER;
				}
			}
#else
			set_device_id_in_firmware(vc_status.dongle_id);

			if(custom_binding != U16_MAX && custom_binding != 0){
				vc_event.cnt = 1;
				btn_status.btn_new = 1;

				//key_status = KEY_PRESSED;
				vc_event.btn_ctrl = KEY_PRESSED;
				vc_event.keycode[0] = VC_POWER;
			}
#endif
//			repair_and_reset.repair = 1;
//			repair_info_load();
		}
	}

}

//u32 factory_reset_tick;
void btn_factory_reset_detect(void)
{
	if(repair_and_reset.poweron_reset == 2 && repair_and_reset.r_cnt++ < 400){
		if(RESET_SEQ == KEYCODE_TO_VALUE){
			FactRestEn = 1;
			callback_auto_paring();
			repair_and_reset.r_cnt = 0;
			repair_and_reset.poweron_reset = 0;

			//repair_info_load();

		}
	}
	else{
		repair_and_reset.poweron_reset = 0;
	}
}
#endif

static u8 press_btn_cnt = 0;

extern u8 valid_batt_value;

u8 *m_r_addr = (u8 *)&repair_and_reset;
u8 btn_save = 0;

void button_detect_proc(void)
{

	key_status = 0;						//clear key status;

	static u8 last_btn_cnt = 0;
	static u8 cur_btn_cnt = 0;
	static u8 press_key_save;
	static u8 one_key_press_save;
	static u8 double_key_press_save;


	if(repair_and_reset.poweron_reset == 1){
		repair_and_reset.poweron_reset = 2;
	}
//	u8 btn_change = vc_detect_button(!km_dat_sending);

#if (WITH_SPELL_PACKET_EN)

	if(vc_detect_button(!km_dat_sending)){
	/*************************************************************************************************
	 * cur_btn_cnt  last_btn_cnt
	 *-----------------------------
	 *		0			 1			->  Release key, when DoubleKeySendFlag is true, release double key
	 *		0			 2			->  Release double key
	 *-----------------------------
	 *		1			 0			-> 	Press key status,
	 *		1            1			->  Repeat key status
	 *		1            2			->  Release double key, and press the last key
	 *-----------------------------
	 *		2			 0			->  Double key, press key status
	 *		2			 1			->  Release first key, and press the double key
	 *		2			 2			->  Double Key, Repeat key status
	 ****************************************************************************************************/
			memset(m_r_addr, 0, 8);				//btn changed, m_cnt and r_cnt clear
			cur_btn_cnt = vc_event.cnt;

			if( !cur_btn_cnt ){
				if(last_btn_cnt == 1 ){			//release key
					vc_event.cnt = 1;
					vc_event.btn_ctrl = KEY_RELEASED;

//					if(!KeyRelease){
						vc_event.keycode[0] = one_key_press_save;
//					}
//					else{
//						vc_event.keycode[0] = KeyRelease;
//					}

					if(!FactRestEn)
						btn_status.btn_new = 1;
					else
						FactRestEn = 0;

					//key_status = KEY_RELEASED;
				}
	#if(WITH_DOUBLE_KEY_FUNC)
				else if(last_btn_cnt == 2 ){		//double key release
					if(DoubleKeySendFlag){

						DoubleKeySendFlag = 0;
						btn_status.btn_new = 1;

						vc_event.cnt = 1;
						vc_event.btn_ctrl = KEY_RELEASED;
						vc_event.keycode[0] = double_key_press_save;

						//key_status = KEY_RELEASED;

					}
				}
	#endif
			}
			else if(cur_btn_cnt == 1){
				if(!last_btn_cnt){				//press key

					btn_status.btn_new = 1;

					//key_status = KEY_PRESSED;
					vc_event.btn_ctrl = KEY_PRESSED;
					//OneKeyPressSave = vc_event.keycode[0];
					one_key_press_save = vc_event.keycode[0];

#if (WITH_POWER_KEY_FOR_MANNUAL_PAIR)
					if(VC_POWER == vc_event.keycode[0])			//RX's id hasn't been stored
						repair_and_reset.repair_cnt++;
#endif


					keycode_tbl[press_btn_cnt & 3] = vc_event.keycode[0];		//store keycode
					press_btn_cnt++;

				}
				else if(last_btn_cnt == 1){		//repeat key

					btn_status.btn_new = 1;

					vc_event.btn_ctrl = KEY_REPEAT;
					//key_status = KEY_REPEAT;

				}
				else if(last_btn_cnt == 2){

#if(WITH_DOUBLE_KEY_FUNC)
					if(!DoubleKeySendFlag){		//press key
						btn_status.btn_new = 1;

						vc_event.btn_ctrl =  KEY_PRESSED;
						//vc_event.keycode[0] = KeyCurPress;

						one_key_press_save =  vc_event.keycode[0];
						//key_status = KEY_PRESSED;
					}
					else{
						DoubleKeySendFlag = 0;					//clear this flag
						btn_status.btn_new = 1;

						vc_s_event.cnt = 1;						//press current key
						vc_s_event.btn_ctrl = KEY_PRESSED;
						vc_s_event.keycode[0] = vc_event.keycode[0];

						vc_event.cnt = 1;						//release double key
						vc_event.btn_ctrl = KEY_RELEASED;
						vc_event.keycode[0] = double_key_press_save;



						one_key_press_save = vc_s_event.keycode[0];;
					}
#endif
				}

			}
			else if(cur_btn_cnt == 2){

				vc_double_key_proc();
#if(WITH_DOUBLE_KEY_FUNC)
				if( !last_btn_cnt ){							//double key press
					if(DoubleKeyFlag){
						DoubleKeyFlag = 0;					// Clear DoubleKeyFlag;
						DoubleKeySendFlag = 1;

						btn_status.btn_new = 1;

						//key_status = KEY_PRESSED;
						vc_event.btn_ctrl = KEY_PRESSED;
						double_key_press_save = vc_event.keycode[0];

					}

				}
				else if(last_btn_cnt == 1 ){
					if( DoubleKeyFlag  ){				// 先release, 再press
						btn_status.btn_new = 1;


						DoubleKeyFlag = 0;						// Clear DoubleKeyFlag;
						DoubleKeySendFlag = 1;

						vc_event.cnt = 1;
						vc_event.btn_ctrl = KEY_RELEASED;
						vc_event.keycode[0] = one_key_press_save;

						vc_s_event.cnt = 1;
						vc_s_event.btn_ctrl = KEY_PRESSED;
						vc_s_event.keycode[0] = DoubleKeySave;

						//key_status = KEY_PRESSED;
						double_key_press_save = DoubleKeySave;//vc_event.keycode[0];
					}
					else{
						btn_status.btn_new = 1;
						//key_status = KEY_RELEASED;
						vc_event.btn_ctrl = KEY_RELEASED;
						//vc_event.keycode[0] = KeyLastPress;
						vc_event.keycode[0] = one_key_press_save;
					}

				}
				else if(last_btn_cnt == 2){
					if(DoubleKeySendFlag){
						btn_status.btn_new = 1;

						vc_event.btn_ctrl = KEY_REPEAT;
						//key_status = KEY_REPEAT;
					}

				}
#endif

			}

			last_btn_cnt  = cur_btn_cnt;
		}
		else{
			btn_mannual_pair_detect();			//mannual pair
		}


#else
	if(vc_detect_button(!km_dat_sending)){
/*************************************************************************************************
 * cur_btn_cnt  last_btn_cnt
 *-----------------------------
 *		0			 1			->  Release key, when DoubleKeySendFlag is true, release double key
 *		0			 2			->  Release double key
 *-----------------------------
 *		1			 0			-> 	Press key status, when delay timeout, send key code.
 *		1            1			->  Repeat key status
 *		1            2			->  Within delay time, send double key, else release the first key
 *-----------------------------
 *		2			 0			->  Double key, press key status
 *		2			 1			->  When DoubleKeySendFlag is true, do nothing. Else Press key status
 *		2			 2			->  Double Key, Repeat key status
 ****************************************************************************************************/
		memset(m_r_addr, 0, 8);				//btn changed, m_cnt and r_cnt clear

		cur_btn_cnt = vc_event.cnt;

		if( !cur_btn_cnt ){
			if(last_btn_cnt == 1 ){			//release key
				vc_event.cnt = 1;

#if(WITH_DOUBLE_KEY_FUNC)

				if(DoubleKeySendFlag){
					DoubleKeySendFlag = 0;
					vc_event.keycode[0] = DoubleKeySave;
				}
				else{
					if(!KeyRelease){
						vc_event.keycode[0] = press_key_save;
					}
					else{
						vc_event.keycode[0] = KeyRelease;
					}
				}

#else
				if(!KeyRelease){
					vc_event.keycode[0] = press_key_save;
				}
				else{
					vc_event.keycode[0] = KeyRelease;
				}
#endif
				if(!FactRestEn)
					btn_status.btn_new = 1;
				else
					FactRestEn = 0;

				key_status = KEY_RELEASED;
			}
#if(WITH_DOUBLE_KEY_FUNC)
			else if(last_btn_cnt == 2 ){		//double key release
				if(DoubleKeySendFlag){
					DoubleKeySendFlag = 0;

					vc_event.cnt = 1;
					vc_event.keycode[0] = DoubleKeySave;

					key_status = KEY_RELEASED;
					btn_status.btn_new = 1;
				}
			}
#endif
		}
		else if(cur_btn_cnt == 1){
			if(!last_btn_cnt){				//press key

#if(WITH_DOUBLE_KEY_FUNC)
				DelayPressCnt = 1;
				OneKeyPressSave = KeyCurPress;
				OneKeyStatusSave = KEY_PRESSED;

				press_key_save =  KeyCurPress;
#else
				btn_status.btn_new = 1;
				key_status = KEY_PRESSED;
				press_key_save =  KeyCurPress;
#endif


				keycode_tbl[press_btn_cnt & 3] = vc_event.keycode[0];		//store keycode
				press_btn_cnt++;

			}
			else if(last_btn_cnt == 1){		//repeat key
#if(WITH_DOUBLE_KEY_FUNC)
				if(DoubleKeySendFlag){
											//do nothing
				}
				else {
					btn_status.btn_new = 1;
					key_status = KEY_REPEAT;
				}

#else
				btn_status.btn_new = 1;
				key_status = KEY_REPEAT;
#endif

			}
			else if(last_btn_cnt == 2){							//press key

#if(WITH_DOUBLE_KEY_FUNC)
				if(!DoubleKeySendFlag){
					btn_status.btn_new = 1;
					vc_event.keycode[0] = KeyCurPress;
					press_key_save =  KeyCurPress;
					key_status = KEY_PRESSED;
				}
				else{
							// do nothing
				}

#else
				btn_status.btn_new = 1;
				vc_event.keycode[0] = KeyCurPress;
				press_key_save =  KeyCurPress;
				key_status = KEY_PRESSED;
#endif
			}

		}
		else if(cur_btn_cnt == 2){

			vc_double_key_proc();
#if(WITH_DOUBLE_KEY_FUNC)
			if( !last_btn_cnt ){							//double key press
				if(DoubleKeyFlag){
					DoubleKeyFlag = 0;					// Clear DoubleKeyFlag;
					DoubleKeySendFlag = 1;

					btn_status.btn_new = 1;

					key_status = KEY_PRESSED;
					press_key_save = vc_event.keycode[0];

				}

			}
			else if(last_btn_cnt == 1 ){
				if(DoubleKeyFlag && DelayPressCnt  ){		// DelayPressCnt don't be clear
					btn_status.btn_new = 1;

					DelayPressCnt = 0;						// Clear DelayPressCnt
					DoubleKeyFlag = 0;						// Clear DoubleKeyFlag;
					DoubleKeySendFlag = 1;

					key_status = KEY_PRESSED;
					press_key_save = vc_event.keycode[0];
				}
				else{
					btn_status.btn_new = 1;
					key_status = KEY_RELEASED;

					vc_event.keycode[0] = KeyLastPress;
				}

			}
			else if(last_btn_cnt == 2){
				if(DoubleKeySendFlag){
					btn_status.btn_new = 1;

					key_status = KEY_REPEAT;
				}

			}

#else
			if(last_btn_cnt == 1){
				btn_status.btn_new = 1;
				key_status = KEY_RELEASED;

				vc_event.keycode[0] = KeyLastPress;
#endif

		}

		//vc_double_key_proc();
		last_btn_cnt  = cur_btn_cnt;
	}
	else{
		btn_mannual_pair_detect();			//mannual pair
	}


#if 0
	if(vc_detect_button(!km_dat_sending)){

//		dbg_btn_cnt++;

		cur_btn_cnt = vc_event.cnt;





#if(USE_CURRENT_VERSION_1P6)
		memset(m_r_addr, 0, 7);				//btn changed, m_cnt and r_cnt clear
#else
		memset(m_r_addr, 0, 8);				//btn changed, m_cnt and r_cnt clear
#endif

#if(!WITH_RELEASE_KEY_FUNC)
		if(cur_btn_cnt == 1 && last_btn_cnt < 2 )
#else
		//if( (cur_btn_cnt == 1 && last_btn_cnt < 2) || \
		//		(cur_btn_cnt == 2 && ((vc_event.keycode[0] == COMBINE_KEY1_FUNC) || (vc_event.keycode[0] == COMBINE_KEY2_FUNC))) )
#endif
		{
			if(!DoubleKeyFuncEn ){
				btn_save =  vc_event.keycode[0];
			}
			else{
				if( !DoubleKeyCnt ){
					//DoubleKeyCnt = 0, and btn_save = 0
					DoubleKeyCnt++;
					btn_save =  vc_event.keycode[0];
				}
			}

			btn_status.btn_new = 1;
			keycode_tbl[press_btn_cnt & 3] = vc_event.keycode[0];		//store keycode
			press_btn_cnt++;
//			if(VC_POWER == vc_event.keycode[0])			//RX's id hasn't been stored
//				repair_and_reset.repair_cnt++;



		}

		vc_double_key_proc();
		batt_detect_and_fliter();			//battery detect

#if(WITH_RELEASE_KEY_FUNC)
		if(key_release_flg){
			vc_event.cnt = 1;
			key_release_flg = 0;
			btn_status.btn_new = 1;
			vc_event.keycode[0] = Release_key;
			DoubleKeyCnt = 0;
			DoubleKeyFuncEn = 0;

		}
#endif

#if MANUAL_PAIR_EN
		btn_pair_confirm_detect();			//pair confirm
#endif

		last_btn_cnt  = cur_btn_cnt;
	}
	else{
		btn_mannual_pair_detect();			//mannual pair
	}
#endif

#if(WITH_DOUBLE_KEY_FUNC)
	if(DelayPressCnt && (DelayPressCnt++ > DelayT)){			//delay timeout， send the key

		DelayPressCnt = 0;

		btn_status.btn_new = 1;
		key_status = OneKeyStatusSave;

		vc_event.cnt = 1;
		vc_event.keycode[0] =  OneKeyPressSave;

	}
#endif

#endif
	//dust sensor data get

#if(WITH_POWER_KEY_FOR_MANNUAL_PAIR)
	btn_pair_confirm_detect();
#endif
	btn_factory_reset_detect();

}


