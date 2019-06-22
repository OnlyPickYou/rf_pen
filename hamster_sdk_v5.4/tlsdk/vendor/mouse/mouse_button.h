/*
 * mouse_button.h
 *
 *  Created on: Feb 12, 2014
 *      Author: xuzhen
 */

#ifndef MOUSE_BUTTON_H_
#define MOUSE_BUTTON_H_


#ifndef MOUSE_BUTTON_FULL_FUNCTION
#define MOUSE_BUTTON_FULL_FUNCTION     1
#endif

#define RC_BUTTON_OVER_THRESH 250		//250 * 12ms = 3000ms
#define	RC_MAX_BUTTON_VALUE 3

#define SWS_CONTROL_LED2_EN			0

enum{
	RC_INVALID_VALUE    =  0,
	RC_ONLY_LEFT_VALUE	=  1,
	RC_ONLY_RIGHT_VALUE =  2,
	RC_LEFT_AND_RIGHT_VALUE = 3,
	RC_ONLY_MID_VALUE = 4,
};


enum{
	RC_DATA_SHORT_LEFT = 0,
	RC_DATA_LONG_LEFT,
	RC_DATA_LONG_LEFT_OVER,
	RC_DATA_SHORT_RIGHT,
	RC_DATA_LONG_RIGHT,
	RC_MAX_DATA_VALUE,
};


extern kb_data_t btn_map_value[RC_MAX_DATA_VALUE];


void rc_button_init(rc_hw_t *rc_hw);
u32 rc_button_process(rc_status_t * rc_status);
u32 rc_button_process_and_mapping(rc_status_t * rc_status);
u32 rc_button_process_emi(s8 *chn_idx, u8 *test_mode_sel, u32 btn_pro_end);

u32 rc_button_detect(rc_status_t  * rc_status, u32 detect_level);
u32 rc_button_pull_and_detect(rc_status_t  * rc_status);

#define MOUSE_BTN_HIGH    1
#define MOUSE_BTN_LOW     0

#define BTN_INTO_PAIRING        1
#define BTN_INTO_EMI            2
#define BTN_INTO_SPECL_MODE     4

#endif /* MOUSE_BUTTON_H_ */
