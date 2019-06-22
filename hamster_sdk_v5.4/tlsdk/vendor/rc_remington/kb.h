/*
 * kb.h
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#pragma once


#include "..\..\proj\common\types.h"


typedef struct {
	u8  kb_mode;
	u8  mode_link;
	u8  rf_sending;
	u8  kb_pipe_rssi;

	u8  tx_power;
	u8  cust_tx_power;
	u8  tx_retry;
    u8  host_keyboard_status;

    u16 no_ack;
    u8 pre_host_status;
    u8 rsv1;

	u16 led_gpio_lvd;
	u16 led_gpio_scr;
	u16 led_gpio_cap;
	u16 led_gpio_num;

	u8  led_level_lvd;
	u8  led_level_scr;
	u8  led_level_cap;
	u8  led_level_num;

	u32 dongle_id;
    u32 loop_cnt;
} kb_status_t;


typedef enum{
	STATE_POWERON = 0,
	STATE_SYNCING,
	STATE_PAIRING ,
	STATE_NORMAL,
	STATE_EMI,
	STATE_WAIT_DEEP,
}KB_MODE;

extern kb_status_t  kb_status;
/////////////////// set default   ////////////////
#include "../common/default_config.h"
