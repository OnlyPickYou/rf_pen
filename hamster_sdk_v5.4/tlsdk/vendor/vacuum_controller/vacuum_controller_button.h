/*
 * vacuum_controller_button.h
 *
 *  Created on: 2015-11-16
 *      Author: Administrator
 */

#ifndef VACUUM_CONTROLLER_BUTTON_H_
#define VACUUM_CONTROLLER_BUTTON_H_

#include "../common/default_config.h"
#include "vacuum_controller_default_config.h"

#define 	MAX_BTN_BUF  		8
#define 	NO_BTN_VALUE    	0xff

#define 	VACUUM_BTN_UP		0x01
#define		VACUUM_BTN_POWER	0x02
#define		VACUUM_BTN_DOWN		0x04

#define		VC_UP				0x20
#define		VC_POWER			0x10
#define		VC_DOWN				0x30

#define 	BTN_VALID_LEVEL		0			//0:low level active, pull_up_resister;

#define 	BTN_UP				GPIO_GP10
#define 	BTN_POWER			GPIO_GP7
#define 	BTN_DOWN			GPIO_GP8

#define 	MAX_BTN_SIZE		3
#define 	MAX_BUF_SIZE		8

u8  		keycode_tbl[4];
#define 	KEYCODE_TO_VALUE	(keycode_tbl[0] | keycode_tbl[1]<<8 | keycode_tbl[2]<<16)
#define 	RESET_SEQ			0x302010		//First:UP, Second:DWON

typedef struct{
	u8	btn_cnt;
	u8 	wptr;
	u8 	rptr;
	u8  rsv;

	u8 	value[MAX_BUF_SIZE];
}vcEvt_buf_t;
extern 	vcEvt_buf_t vcEvt_buf;

typedef struct{
	u8  btn_history[4];		//vc history btn save

	u8	btn_not_release;
	u8 	btn_new;					//new btn  flag

}btn_status_t;
extern	btn_status_t btn_status;

typedef struct{
	u16 m_cnt;			//mannual paring cnt
	u16 m_flg;			//m_cnt开始计数的标志

	u16 r_cnt;			//reset fuction cnt
	u8  r_flg;
	u8  factory_reset;

	u8  auto_pair;
	u8  repair;

	u8	repair_cnt;	//press "power" btn 50 times, enable repair
	u8  poweron_reset;	//when press up+down+power, poweron_reset = 1;

}repair_and_reset_t;
extern 	repair_and_reset_t repair_and_reset;

typedef struct{
	u8 	factory_reset_ui;
	u8 	emi_ui;
//	u8  combine_key1_ui;		// up + down
//	u8  combine_key2_ui;		// up + power
	u8  combine_key3_ui;		// power + down

}btn_ui_t;

extern btn_ui_t btn_ui;

typedef struct{
	//注意4字节对齐
	u8 btn_wptr;
	u8 btn_rptr;
	u8 btn_history;
	u8 btn_last_valid;

	u8 btn_current_value;
	u8 btn_new;
	u8 btn_not_released;
	u8 rsv2;

	u8 btn_value[MAX_BTN_BUF];
}btn_buf_t;

extern btn_buf_t btn_buffer;

void vc_double_key_proc(void);

void btn_pair_confirm_detect(void);

extern void repair_info_load(void);

extern void callback_auto_paring(void);

void btn_mannual_pair_detect(void);

void btn_factory_reset_detect(void);

void button_detect_proc(void);

#endif /* VACUUM_CONTROLLER_BUTTON_H_ */
