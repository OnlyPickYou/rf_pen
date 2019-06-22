/*
 * dongle_emi.h
 *
 *  Created on: 2014-5-21
 *      Author: hp
 */

#ifndef VACUUM_RECEIVER_EMI_H_
#define VACUUM_RECEIVER_EMI_H_

#include "vacuum_receiver_iouart.h"
//void usb_host_cmd_proc(u8 *);

typedef struct{
		u8 cmd_cur;
		u8 test_chn_idx; 	//default 2430 carrier
		u8 test_mode_sel;
		u8 flg_emi_init;
}vc_emi_data_t;


void uart_rx_emi_init(void);

void uart_tx_emi_proc(void);

void uart_rx_emi_proc(void);

u32 vc_cmd_process_emi(u8 *chn_idx, u8 *test_mode_sel);

void vc_emi_process(void);

#endif /* DONGLE_EMI_H_ */
