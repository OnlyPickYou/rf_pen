/*
 * mouse_emi.h
 *
 *  Created on: Feb 14, 2014
 *      Author: xuzhen
 */

#ifndef VACUUM_CONTROLLER_EMI_H_
#define VACUUM_CONTROLLER_EMI_H_

#define RF_FULL_DUTY_CYCLE				0				//100% duty cycle, only send packet continuously
#define RF_HALF_DUTY_CYCLE				1				// 50% duty cycle,delay send packet time
#define RF_QUARTER_DUTY_CYCLE			3				// 25% duty cycle

#define	SPECIAL_EMI_MODE_EN				0

typedef struct{
		u8 key_cur;
		u8 test_chn_idx; //default 2430 carrier
		u8 test_mode_sel;
		u8 flg_emi_init;
}vc_emi_data_t;

typedef struct{
	u8 sel_cnt;			//mode and chn sel
	u8 carry_switch;	//carry mode and chn switch flag;
	u8 cd_switch;		//cd mode and chn switch flag;
	u8 cd_duty_mode;	//16 channel, random send data pakect
	u8 cd_switch_duty;
	u8 cd_hop_chn;
	u8 loop_break;		//power btn was pressed and break switch;

}vc_emi_mode_t;

typedef struct{

	u8 duty_idx;		//duty cycle index
	u8 duty_sgm;		//duty cycle segment
	u8 duty_tbl[4];

}rf_duty_cycle_t;



extern void vc_emi_process(void);

#endif /* MOUSE_EMI_H_ */
