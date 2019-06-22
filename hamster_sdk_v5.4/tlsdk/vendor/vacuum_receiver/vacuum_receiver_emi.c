/*
 * receiver_emi.c
 *
 *  Created on: 2015-10-21
 *      Author: zjs
 */

#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"
#include "vacuum_receiver_emi.h"
#include "vacuum_receiver_custom.h"


#define MAX_CHN_NUM		16
#define MAX_TEST_STEP_NUM	11
#define MATH_G(m,n)		((m-n)>0)
#define MATH_L(m,n)		((m-n)<0)
#define CHN_HOP(m)		((m + 6 )& 15)

typedef struct{
		u8 sel_cnt;			//mode and chn sel
		u8 carry_switch;	//carry mode and chn switch flag;
		u8 cd_switch;		//cd mode and chn switch flag;
		u8 cd_duty_mode;
		u8 cd_switch_duty;
		u8 loop_break;		//power btn was pressed and break switch;

		u8 new_emi_cmd;

}vc_emi_mode_t;

vc_emi_data_t vc_emi_data = {
		0,
		0,			//default 2405 carrier
		0,
		0,

		0,
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

#if EMI_UART_RX_EN


extern uart_pkt_data_emi_t uart_pkt_data_emi;
extern uart_pkt_ack_emi_t uart_pkt_ack_emi;

u8 *pkt_data_emi = (u8 *)(&uart_pkt_data_emi);
u8 *pkt_ack_emi = (u8 *)(&uart_pkt_ack_emi);


u8 *p_data = (u8 *)(&uart_pkt_data_emi);
u8 *p_ack = (u8 *)(&uart_pkt_ack_emi);

#endif

volatile u32 uart_tx_emi_time;				//10ms
volatile u32 uart_tx_delay_emi_time;		//10ms
volatile u32 uart_trans_en_emi_time;		//10ms


void uart_rx_emi_init(void)
{
#if EMI_UART_RX_EN
	//u32 uart_en_flag = gpio_read(EMI_UART_RECV_EN);
	u32 crc_cnt = 0;

	//if(uart_en_flag){
		//uart_trans_en_emi_time = clock_time();
		//uart_tx_delay_emi_time = clock_time();
		for(int i=0; i<uart_pkt_ack_emi.len-2; i++){		//len-1:crc, len:end_byte
			crc_cnt += *p_ack;
			p_ack++;
		}
		crc_cnt = (u8)(crc_cnt & 0xff) ^ 0x55;   //crc rule
		uart_pkt_ack_emi.crc = (u8)crc_cnt;
		p_ack = &(uart_pkt_ack_emi);

		//while(!clock_time_exceed(uart_tx_delay_emi_time,EMI_TX_DELAY_TIME));	//sleep 100ms
	//}
#endif
}


void uart_rx_emi_proc(void)
{
#if EMI_UART_RX_EN

	while(1){
		uart_tx_emi_time = clock_time();
		if(uart_rx_and_detect(EMI_UART_RX, p_data, sizeof(uart_pkt_data_emi_t)) && (uart_pkt_data_emi.type == 0x20)){
			vc_emi_mode.new_emi_cmd = 1;
			vc_emi_data.cmd_cur = *(p_data + 3);
			memset(p_data, 0, sizeof(uart_pkt_data_emi_t));
			while(!clock_time_exceed(uart_tx_emi_time,EMI_TX_AND_RX_TIME));	//sleep 10ms to wait tx
			uart_tx_emi_time = clock_time();
			uart_send_pkt(EMI_UART_TX, pkt_ack_emi, sizeof(uart_pkt_ack_emi_t));
			while(!clock_time_exceed(uart_tx_emi_time,EMI_TX_AND_RX_TIME));	//sleep 10ms

			if(vc_emi_mode.new_emi_cmd == 1){
				break;
			}
		}
	}

#endif
}

#if (SPECIAL_EMI_MODE_EN)

u8 mode_sel = 0;
u32 mode_sel_cnt = 300;
u32 vc_cmd_process_emi(u8 *chn_idx, u8 *test_mode_sel)  //zjs need debug
{
	u32 cmd = 0;

	if(vc_emi_mode.new_emi_cmd == 1)		//new event
	{
		cmd = 0x80;
		mode_sel_cnt = 300;
		//vc_emi_data.key_cur = vc_event.keycode[0];
		memset(&vc_emi_mode, 0, sizeof(vc_emi_mode_t) );
		*test_mode_sel = 1;
		vc_emi_mode.sel_cnt = vc_emi_data.cmd_cur>>4;
		if((vc_emi_mode.sel_cnt < 0x10)  || vc_emi_mode.loop_break){			//mode + 1
			vc_emi_mode.loop_break = 0;
			*chn_idx = vc_emi_mode.sel_cnt;
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
extern rf_packet_ack_pairing_t	ack_pairing;
extern rf_ack_empty_t ack_empty;


void vc_emi_process(void)
{
    //kb_proc_key();
    u32 cmd = 0;
    u32 time_loop_cnt;

    cmd = vc_cmd_process_emi( &vc_emi_data.test_chn_idx, &vc_emi_data.test_mode_sel );
    cmd |= !vc_emi_data.flg_emi_init;
    if( !vc_emi_data.flg_emi_init ){
    	vc_emi_data.flg_emi_init = 1;
    }

	irq_disable();						//shut down irq
	reg_tmr_ctrl &= ~FLD_TMR1_EN;		//shut down timer1

	while(mode_sel_cnt--){
		if(!mode_sel){
			mode_sel = !mode_sel;
			emi_process( 0x80, vc_emi_data.test_chn_idx, 1, (u8 *)&ack_pairing, RF_POWER_8dBm );
			sleep_us(2000);
		}
		else{
			mode_sel = !mode_sel;
			emi_process( 0x80, vc_emi_data.test_chn_idx, 2, (u8 *)&ack_pairing, RF_POWER_8dBm );
			sleep_us(6000);
		}
	}


}
#else

u32 vc_cmd_process_emi(u8 *chn_idx, u8 *test_mode_sel)  //zjs need debug
{
	u32 cmd = 0;

	if(vc_emi_mode.new_emi_cmd == 1)		//new event
	{
		cmd = 0x80;
		//vc_emi_data.key_cur = vc_event.keycode[0];
		memset(&vc_emi_mode, 0, sizeof(vc_emi_mode_t) );
		vc_emi_mode.sel_cnt = vc_emi_data.cmd_cur>>4;
		if((vc_emi_mode.sel_cnt < MAX_TEST_STEP_NUM)  || vc_emi_mode.loop_break){			//mode + 1
			vc_emi_mode.loop_break = 0;
//			vc_emi_mode.sel_cnt++;
//			vc_emi_mode.sel_cnt %= 9;
			if(MATH_L(vc_emi_mode.sel_cnt, 3)){
				*test_mode_sel = 0;
				*chn_idx = vc_emi_mode.sel_cnt;
			}
			else if(vc_emi_mode.sel_cnt == 3){
			    *test_mode_sel = 0;
				vc_emi_mode.carry_switch = 1;
			}
			else if(MATH_G(vc_emi_mode.sel_cnt, 3) && MATH_L(vc_emi_mode.sel_cnt, 7)){
				*test_mode_sel = 1;
				*chn_idx = vc_emi_mode.sel_cnt % 4;
			}
			else if(vc_emi_mode.sel_cnt == 7){
				*test_mode_sel = 1;
				vc_emi_mode.cd_switch = 1;
			}
			else if(vc_emi_mode.sel_cnt == 8){
				*test_mode_sel = 1;
				*chn_idx = 0;
				vc_emi_mode.cd_duty_mode = 1;

			}
			else if(vc_emi_mode.sel_cnt == 9){

				*test_mode_sel = 1;
				*chn_idx = 0;
				vc_emi_mode.cd_switch_duty = 1;

			}
			else{
				*test_mode_sel = 2;
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
extern rf_packet_ack_pairing_t	ack_pairing;
extern rf_ack_empty_t ack_empty;

void vc_emi_process(void)
{
    //kb_proc_key();
    u32 cmd = 0;
    u32 time_loop_cnt;
    static u32 mode_sel_cnt;

    cmd = vc_cmd_process_emi( &vc_emi_data.test_chn_idx, &vc_emi_data.test_mode_sel );
    cmd |= !vc_emi_data.flg_emi_init;
    if( !vc_emi_data.flg_emi_init ){
    	vc_emi_data.flg_emi_init = 1;
    }
    //kb_device_led_setup( kb_led_emi_cfg[kb_emi_data.test_mode_sel] );
    //kb_device_led_process();
    //cmd = 0x81;
	irq_disable();						//shut down irq
	reg_tmr_ctrl &= ~FLD_TMR1_EN;		//shut down timer1
    if(vc_emi_mode.carry_switch || vc_emi_mode.cd_switch){
    	for(u8 i=0; i<MAX_CHN_NUM; i++){				//MAX_CHN_NUM
    		cmd = 1;
    		vc_emi_data.test_chn_idx = i;

    		emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&ack_pairing, RF_POWER_8dBm );// dongle_cust_tx_power_emi );
    		sleep_us(1000000);

    		cmd = 0;
    	}
    	vc_emi_mode.carry_switch = 0;
    	vc_emi_mode.cd_switch = 0;
    }
    else if( vc_emi_mode.cd_duty_mode ){
    	mode_sel_cnt = 4999;
    	while(mode_sel_cnt > 0){
    		mode_sel_cnt--;
    		if(mode_sel_cnt & 0x01){
    			emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&ack_pairing, RF_POWER_8dBm );
    			sleep_us(2000);
    		}
    		else{
    			emi_process( cmd , vc_emi_data.test_chn_idx, 2, (u8 *)&ack_pairing, RF_POWER_8dBm );
    			sleep_us(6000);
    		}
    	}


    }
    else if( vc_emi_mode.cd_switch_duty ){

    	for(int i=0; i<MAX_CHN_NUM; i++){
    	    time_loop_cnt = 80;
    	    vc_emi_data.test_chn_idx = i;
    	    while(time_loop_cnt--){
    	    	emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&ack_pairing, RF_POWER_8dBm );
    	    	sleep_us(2000);
    	    	emi_process( cmd , vc_emi_data.test_chn_idx, 2, (u8 *)&ack_pairing, RF_POWER_8dBm );
    	    	sleep_us(6000);
    	    }
    	}
    }
    else{
    	emi_process( cmd , vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&ack_pairing, RF_POWER_8dBm );//dongle_cust_tx_power_emi );
    }
}
#endif













#if 0


u8      dongle_emi_cd_mode;
u8		dongle_host_cmd1;

extern u32     rf_paring_tick;


/******************************************************************************************
 * old
 *  host_cmd[1]
 *	0  1  2   no use
 *	3                        软件配对时间允许
 *	4  5  6   carrier          test_mode_sel = 0
 *	7  8  9   cd               test_mode_sel = 1
 *	10 11 12  rx               test_mode_sel = 2
 *
 *	4  7  10  fre_7     2440   chn_idx = 1   host_cmd_chn_m
 *	5  8  11  fre_15    2476   chn_idx = 2   host_cmd_chn_h
 *	6  9  12  fre_1     2409   chn_idx = 0   host_cmd_chn_l
 *****************************************************************************************/
#if(0)
const u8	dongle_chn_mode[13] = {
		0, 0, 0,
		0,
		chn_idx_m | test_mode_carrier<<2,
		chn_idx_h | test_mode_carrier<<2,
		chn_idx_l | test_mode_carrier<<2,

		chn_idx_m | test_mode_cd<<2,
		chn_idx_h | test_mode_cd<<2,
		chn_idx_l | test_mode_cd<<2,

		chn_idx_m | test_mode_rx<<2,
		chn_idx_h | test_mode_rx<<2,
		chn_idx_l | test_mode_rx<<2,
		};
#endif



#define  PARING_ENABLE_CMD_VALUE  0x88
/******************************************************************************************
 * 	new
 *  host_cmd[1]    chn	     mode
 *  01			   2409		carrier
 *  02			   2409		cd
 *  03			   2409		rx
 *  04			   2409		tx
 *  05			   2435		carrier
 *  06			   2435		cd
 *  07			   2435		rx
 *  08			   2435		tx
 *  09			   2476		carrier
 *  0a			   2476		cd
 *  0b			   2476		rx
 *  0c			   2476		tx
 *****************************************************************************************/
enum {
	chn_idx_l          = 0,   //2409
	chn_idx_m          = 1,   //2435
	chn_idx_h          = 2,	  //2476
	test_mode_carrier  = 0,
	test_mode_cd	   = 1,
	test_mode_rx	   = 2,
	test_mode_tx	   = 3,
};

#define  CARRIER_MODE   0


void usb_host_cmd_proc(u8 *pkt)
{
	extern u8		host_cmd[8];
	extern u8		host_cmd_paring_ok;

	u8   chn_idx;
	u8   test_mode_sel;
	u8 	 cmd = 0;
	static emi_flg;


	if((host_cmd[0]==0x5) && (host_cmd[2]==0x3) )
	{
		host_cmd[0] = 0;
		dongle_host_cmd1 = host_cmd[1];

		if (dongle_host_cmd1 > 12 && dongle_host_cmd1 < 16){  //soft paring
			host_cmd_paring_ok = 0;
			rf_paring_tick = clock_time();  //update paring time

			if(dongle_host_cmd1 == 13){     //kb and mouse tolgether
				mouse_paring_enable = 1;
				keyboard_paring_enable = 1;
			}
			else if(dongle_host_cmd1 == 14){ //mouse only
				mouse_paring_enable = 1;
			}
			else if(dongle_host_cmd1 == 15){  //keyboard only
				keyboard_paring_enable = 1;
			}
		}
		else if(dongle_host_cmd1 > 0 && dongle_host_cmd1 < 13)  //1-12:进入EMI
		{
			emi_flg = 1;
			cmd = 1;

			irq_disable();
			reg_tmr_ctrl &= ~FLD_TMR1_EN;
			//rf_stop_trx ();

			chn_idx = (dongle_host_cmd1-1)/4;
			test_mode_sel = (dongle_host_cmd1-1)%4;
		}
	}

	if(emi_flg){
		emi_process(cmd, chn_idx,test_mode_sel, pkt, dongle_cust_tx_power_emi);
	}
}

#endif
