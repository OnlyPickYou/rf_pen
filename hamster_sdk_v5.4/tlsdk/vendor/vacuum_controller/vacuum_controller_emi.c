/*
 * vc_emi.c
 *
 *  Created on: 2015-10-12
 *      Author: Administrator
 */


#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"

#include "vacuum_controller.h"
//#include "vacuum_controller_custom.h"
#include "vacuum_controller_button.h"
#include "vacuum_controller_rf.h"
#include "vacuum_controller_emi.h"
#include "../../proj_lib/pm_8366.h"

#define MAX_CHN_NUM			16
#define MAX_TEST_STEP_NUM	14
#define MATH_G(m,n)		((m-n)>0)
#define MATH_L(m,n)		((m-n)<0)
#define CHN_HOP(m)		((m + 6 )& 15)

enum{
		TEST_EMI_CARRY_MODE = 0,
		TEST_EMI_CD_MODE = 1,
};

extern vc_data_t	vc_event;
extern btn_status_t btn_status;

vc_emi_data_t vc_emi_data = {
		0,
		0,			//default 2405 carrier
		0,
		0
};

vc_emi_mode_t vc_emi_mode = {
	0,
	0,
	0,
	0,

	0,
	0,
	0,
};

rf_duty_cycle_t rf_duty_cycle = {
	0,				//default duty cycle
	2,				//segment
	0,				//100% duty_cycle, 0/(1+0) = 1
	1,				//50%  duty_cycle, 1/(1+1) = 0.5
	3,				//25%¡¡duty cycle, 1/(1+3) = 0.25
	2,				//reserved, 75%
};

#if (0)
u32 vc_key_process_emi(u8 *chn_idx, u8 *test_mode_sel, u8 *duty_cycle)  //only for cd duty cycle mode
{
	u32 cmd = 0;
	if(btn_status.btn_new)		//new eventon
	{
		cmd = 0x80;
		vc_emi_data.key_cur = vc_event.keycode[0];

		if(vc_emi_data.key_cur == VC_POWER || vc_emi_mode.loop_break){	//chn++

			vc_emi_data.test_chn_idx++;
			vc_emi_data.test_chn_idx %= 16;
		}
	}
	return cmd;
}


/*****************************************************************************
test_chn_idx:
host_cmd_chn_l  = 0,   2405
host_cmd_chn_m  = 6,   2430
host_cmd_chn_h  = 14,  2470
test_mode_sel:
0 :	host_cmd_carrier
1 :	host_cmd_cd
2 :	host_cmd_rx
3 :	host_cmd_tx
*****************************************************************************/
static u16 mode_sel_cnt;

void vc_emi_process(void)
{

    u32 cmd = 0;
    u32 time_loop_tick;
    u8  time_loop_cnt = 0;
    u8  time_hop_chn_cnt;
    u32 static time_duty_cycle;
    u32 emi_start_tick;

    //cmd = vc_key_process_emi( &vc_emi_data.test_chn_idx, &vc_emi_data.test_mode_sel, &rf_duty_cycle.duty_idx );
    cmd |= !vc_emi_data.flg_emi_init;
    if( !vc_emi_data.flg_emi_init ){
    	vc_emi_data.flg_emi_init = 1;
    	sleep_us(2000);
    }
    if(mode_sel_cnt){
    	time_loop_cnt = 6;
    	emi_process( 0x80 , vc_emi_data.test_chn_idx, 1, (u8 *)&pkt_km, vc_cust_tx_power_emi );
    	while(time_loop_cnt){
    		time_loop_cnt--;
    	    time_loop_tick = clock_time();
    	    button_detect_proc();
    	    while(!clock_time_exceed(time_loop_tick, 3500));

    	    if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
    	    	vc_emi_data.test_chn_idx++;
    	    	vc_emi_data.test_chn_idx %= 16;
    	        break;
    	     }
    	 }
    	mode_sel_cnt = !mode_sel_cnt;
    	//sleep_us(2000);
    }
    else{
    	time_loop_cnt = 18;
        emi_process( 0x80 , vc_emi_data.test_chn_idx, 2, (u8 *)&pkt_km, vc_cust_tx_power_emi );
        while(time_loop_cnt){
        	time_loop_cnt--;
			time_loop_tick = clock_time();
			button_detect_proc();
			while(!clock_time_exceed(time_loop_tick, 3500));

			if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
    	    	vc_emi_data.test_chn_idx++;
    	    	vc_emi_data.test_chn_idx %= 16;
				break;
			 }
		 }
        mode_sel_cnt = !mode_sel_cnt;
        //sleep_us(6000);
    }
}
#else
u32 vc_key_process_emi(u8 *chn_idx, u8 *test_mode_sel, u8 *duty_cycle)  //zjs need debug
{
	u32 cmd = 0;
	if(btn_status.btn_new || vc_emi_mode.loop_break)		//new event
	{
		cmd = 0x80;
		vc_emi_data.key_cur = vc_event.keycode[0];
		if(vc_emi_data.key_cur == VC_POWER || vc_emi_mode.loop_break){			//mode + 1
			vc_emi_mode.loop_break = 0;
			vc_emi_mode.sel_cnt++;
			vc_emi_mode.sel_cnt %= MAX_TEST_STEP_NUM;
			if(MATH_L(vc_emi_mode.sel_cnt, 3)){
				*test_mode_sel = 0;
				*chn_idx = vc_emi_mode.sel_cnt;
			}
			else if(vc_emi_mode.sel_cnt == 3){

				vc_emi_mode.carry_switch = 1;
			}
			else if(MATH_G(vc_emi_mode.sel_cnt, 3) && MATH_L(vc_emi_mode.sel_cnt, 7)){
				*test_mode_sel = 1;
				*chn_idx = vc_emi_mode.sel_cnt % 4;
				//vc_emi_mode.cd_switch = 0;
			}
			else if(vc_emi_mode.sel_cnt == 7){
				vc_emi_mode.cd_switch = 1;
			}
			else if(MATH_G(vc_emi_mode.sel_cnt, 7) && MATH_L(vc_emi_mode.sel_cnt, 11)){
				*test_mode_sel = 1;
				*chn_idx = vc_emi_mode.sel_cnt % 4;
				vc_emi_mode.cd_duty_mode = 1;

			}
			else if(vc_emi_mode.sel_cnt == 11){

				*test_mode_sel = 1;
				*chn_idx = 0;
				vc_emi_mode.cd_switch_duty = 1;

			}
			else if(vc_emi_mode.sel_cnt == 12){
				*test_mode_sel = 1;
				*chn_idx = 0;
				vc_emi_mode.cd_hop_chn = 1;
			}
			else{
				*test_mode_sel = 2;
				vc_emi_mode.carry_switch = 0;
				vc_emi_mode.cd_switch = 0;
				vc_emi_mode.cd_duty_mode = 0;
				vc_emi_mode.cd_switch_duty = 0;
				vc_emi_mode.cd_hop_chn = 0;
			}
		}


		if(*chn_idx == 0){
				*chn_idx = 0;
			}
		else if(*chn_idx == 1){
				*chn_idx = 10;
			}
		else if(*chn_idx == 2){
				*chn_idx = 15;
		}
	}

	return cmd;
}


/*****************************************************************************
test_chn_idx:
host_cmd_chn_l  = 0,   2405
host_cmd_chn_m  = 6,   2430
host_cmd_chn_h  = 14,  2470
test_mode_sel:
0 :	host_cmd_carrier
1 :	host_cmd_cd
2 :	host_cmd_rx
3 :	host_cmd_tx
*****************************************************************************/
static u16 mode_sel_cnt;

void vc_emi_process(void)
{

    u32 cmd = 0;
    u32 time_loop_tick;
    u8  time_loop_cnt = 0;
    u8  time_hop_chn_cnt;
    u32 static time_duty_cycle;
    u32 emi_start_tick;

    cmd = vc_key_process_emi( &vc_emi_data.test_chn_idx, &vc_emi_data.test_mode_sel, &rf_duty_cycle.duty_idx );
    cmd |= !vc_emi_data.flg_emi_init;
    if( !vc_emi_data.flg_emi_init ){
    	vc_emi_data.flg_emi_init = 1;
    }

    if(vc_emi_mode.carry_switch || vc_emi_mode.cd_switch){
    	if(vc_emi_mode.carry_switch)
   			vc_emi_data.test_mode_sel = 0;
    	else
   			vc_emi_data.test_mode_sel = 1;

    	for(int i=0; i<MAX_CHN_NUM; i++){
    		time_loop_cnt = 100;
    		vc_emi_data.test_chn_idx = i;
    		emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );

    		while(time_loop_cnt--){
    			time_loop_tick = clock_time();

    			button_detect_proc();
    			while(!clock_time_exceed(time_loop_tick, 10000));

        		if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
        			break;
        		}
    		}
			if(vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER)){
				vc_emi_mode.loop_break = 1;
				break;
			}
    	}
    	vc_emi_mode.carry_switch = 0;
    	vc_emi_mode.cd_switch = 0;
    }
//    else if( vc_emi_mode.cd_duty_mode){
//    	while(1){
//    		//time_loop_tick = clock_time();
//    		emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );
//
//    		sleep_us(2000);
//    		emi_process( cmd , vc_emi_data.test_chn_idx, 2, (u8 *)&pkt_km, vc_cust_tx_power_emi );
//    		sleep_us(6000);
//    		button_detect_proc();
//    		if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
//    			break;
//    		}
//    	}
//    	if(vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER)){
//    		vc_emi_mode.loop_break = 1;
//    		break;
//    	}
//    	vc_emi_mode.cd_duty_mode = 0;
//    }
    else if( vc_emi_mode.cd_switch_duty ){
    	while(1){
    		for(int i=0; i<MAX_CHN_NUM; i++){
    			time_loop_cnt = 85;
    			vc_emi_data.test_chn_idx = i;
    			while(time_loop_cnt--){
    				emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );
    				sleep_us(2000);
    				emi_process( cmd , vc_emi_data.test_chn_idx, 2, (u8 *)&pkt_km, vc_cust_tx_power_emi );
    				sleep_us(6000);
    				button_detect_proc();
    				if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
    					break;
    				}
    			}
    			if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
    			    break;
    			}
    		}
    	    if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
    	    	vc_emi_mode.loop_break = 1;
    	        break;
    	    }
    	}
    }
    else if(vc_emi_mode.cd_hop_chn){			//mode 13, random channel hopping
    	while(1){
			time_loop_cnt = 62;			// 62 * 8ms = 500ms
			//vc_emi_data.test_chn_idx =	CHN_HOP(vc_emi_data.test_chn_idx);
			u32 tick = clock_time();
			vc_emi_data.test_chn_idx = (u8)(tick) & 0x0f;
			while(time_loop_cnt--){
				emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );
				sleep_us(2000);
				emi_process( cmd , vc_emi_data.test_chn_idx, 2, (u8 *)&pkt_km, vc_cust_tx_power_emi );
				sleep_us(6000);
				button_detect_proc();
				if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
					break;
				}
			}
			if( vc_event.cnt == 1 && (vc_event.keycode[0] == VC_POWER) ){
				break;
			}
		}

    }
    else{
        if( vc_emi_mode.cd_duty_mode){
        	mode_sel_cnt++;
        	if(mode_sel_cnt & 1){
        		emi_process( 0x80 , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );
        		sleep_us(2000);
        	}
        	else{
        		emi_process( 0x80 , vc_emi_data.test_chn_idx, 2, (u8 *)&pkt_km, vc_cust_tx_power_emi );
        		sleep_us(6000);
        	}
        }
        else{
        	emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );
        }

//    	if(vc_emi_data.test_mode_sel == TEST_EMI_CD_MODE ){
//    		mode_sel_cnt++;
//    		if(mode_sel_cnt & 0x01){

//    			write_reg8 (0x800f02, RF_TRX_OFF | BIT(4));
//    			sleep_us(2000);
//    			rf_set_txmode();
    			//sleep_us(200);

//    			emi_process( 0x80 , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );
//
//    			sleep_us(2000);
//
//    		}
//    		else{

//    			write_reg8 (0x800f02, RF_TRX_OFF);
//    			sleep_us(2000);
//
//    			write_reg8 (0x800f02, RF_TRX_OFF | BIT(4));
//    			write_reg8 (0x800f02, RF_TRX_OFF);
    			//sleep_us(2000);
    			//rf_set_rxmode();
//    			emi_process( 0x80 , vc_emi_data.test_chn_idx, 2, (u8 *)&pkt_km, vc_cust_tx_power_emi );
//
//    			sleep_us(6000);
//    			//rf_set_txmode();
//
//
//    		}
//    	}
//    	else{
    		//emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&pkt_km, vc_cust_tx_power_emi );
//    	}
    }
}
#endif



