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

#define RC_BUTTON_OVER_THRESH 	120		//120 * 12ms = 1440ms
#define	RC_MAX_BUTTON_VALUE 	3

#define SWS_CONTROL_LED2_EN			1
#define RC_BUTTON_REPEAT_THRESH	20			//12 * 16ms = 200ms
#define RC_DOUBLE_TAB_THRESH	12


#define RC_BUTTON_FIRST			0x01
#define RC_BUTTON_KEEP			0x02
#define RC_BUTTON_REPEAT		0x04
#define RC_BUTTON_RELEASE		0x08

#if 0
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
#else

#if(MOUSE_R150_RF_PEN)
enum{
	RC_INVALID_VALUE        =  0,
	RC_ONLY_UP_VALUE	    = 0x1,
	RC_ONLY_RF_LED_VALUE    = 0x2,
	RC_ONLY_DOWN_VALUE      = 0x4,
	RC_ONLY_START_VALUE     = 0x8,
	RC_ONLY_TAB_VALUE       = 0x10,
	RC_ONLY_VOL_DOWN_VALUE  = 0x20,
	RC_ONLY_VOL_UP_VALUE    = 0x40,
};
#elif(MOUSE_R250_RF_PEN)
enum{
	RC_INVALID_VALUE        =  0,
	RC_ONLY_UP_VALUE	    = 0x1,
	RC_ONLY_RF_LED_VALUE    = 0x2,
	RC_ONLY_DOWN_VALUE      = 0x4,
	RC_ONLY_START_VALUE     = 0x8,
	RC_ONLY_VOL_DOWN_VALUE  = 0x10,
	RC_ONLY_VOL_UP_VALUE    = 0x20,
};
#endif

enum{
	RC_DATA_UP = 0,
	RC_DATA_RF_LED = 1,
	RC_DATA_DOWN = 2,
	RC_DTAT_START = 3,
	RC_DATA_TAB = 4,
	RC_DATA_VOL_DOWN = 5,
	RC_DATA_VOL_UP = 6,		//6

	RC_DATA_START_OVR = 7,	//7
	RC_DATA_TAB_OVR = 8,	//8

	RC_DATA_LONG_START = 9,
	RC_DATA_LONG_TAB = 0xa,

	RC_DATA_DOUBLE_TAB = 0xb,
	RC_DATA_REPEAT_ALT = 0xc,

	RC_MAX_DATA_VALUE,
};

typedef struct{
	u32 LastPreTick;
	u32 LastEscTick;
	u8 KeyNumSave[2];
}rc_btn_ctrl_t;
#endif

extern kb_data_t btn_map_value[RC_MAX_DATA_VALUE];


void rc_button_init(rc_hw_t *rc_hw);
u32 rc_button_process(rc_status_t * rc_status);
u32 rc_button_process_and_mapping(rc_status_t * rc_status, u32 deepsleep);
u32 rc_button_process_emi(s8 *chn_idx, u8 *test_mode_sel, u32 btn_pro_end);

u32 rc_button_detect(rc_status_t  * rc_status, u32 detect_level);
u32 rc_button_pull_and_detect(rc_status_t  * rc_status);

#define MOUSE_BTN_HIGH    1
#define MOUSE_BTN_LOW     0

#define BTN_INTO_PAIRING        1
#define BTN_INTO_EMI            2
#define BTN_INTO_SPECL_MODE     4

#endif /* MOUSE_BUTTON_H_ */
