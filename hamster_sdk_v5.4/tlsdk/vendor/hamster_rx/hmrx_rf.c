/*
 * dongle_rf.c
 *
 *  Created on: Feb 13, 2014
 *      Author: xuzhen
 */
#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"


#include "hmrx_custom.h"
#include "hmrx_usb.h"
#include "hmrx_emi.h"
#include "hmrx_suspend.h"
#include "hmrx_rf.h"


u8		chn_mask = 0x80;

#define	CHANNEL_HOPPING_ALWAYS				1


#ifndef			CHANNEL_SLOT_TIME
#define			CHANNEL_SLOT_TIME			8000
#endif

#ifndef		PKT_BUFF_SIZE
#define		PKT_BUFF_SIZE		80
#endif

#define		FH_CHANNEL_PKT_RCVD_MAX			8
#define		LL_CHANNEL_SYNC_TH				2
#define		LL_CHANNEL_SEARCH_TH			60
#define		LL_CHANNEL_SEARCH_FLAG			BIT(16)
#define		LL_NEXT_CHANNEL(c)				((c + 6) & 14)


unsigned char  		rf_rx_buff[PKT_BUFF_SIZE*2] __attribute__((aligned(4)));
int		rf_rx_wptr;
u8		host_channel = 0;

u8		ll_rssi;
u16		ll_chn_tick;
u32		ll_chn_rx_tick;
u32		ll_chn_mask = LL_CHANNEL_SEARCH_FLAG;

u8		ll_chn_sel;
u8		ll_chn_pkt[16] = {0};
u32		ll_clock_time;
int		device_packet_received;

/////////////////////////////////////////////////////////////////////
//	link management functions
/////////////////////////////////////////////////////////////////////

u8	get_next_channel_with_mask(u32 mask, u8 chn)
{
	int chn_high = (mask >> 4) & 0x0f;

	if (mask & LL_CHANNEL_SEARCH_FLAG) {
		return LL_NEXT_CHANNEL (chn);
	}
	else if (chn_high != chn) {
		ll_chn_sel = 1;
		return chn_high;
	}
	else {
		ll_chn_sel = 0;
		return mask & 0x0f;
	}
}

u32	tick_fh = 0;
u32	fh_num = 0;
u8	update_channel_mask(u32 mask, u8 chn, u8* chn_pkt)
{
	static int ll_chn_sel_chg, ll_chn_hold;

#if CHANNEL_HOPPING_ALWAYS
	if (device_packet_received == 1) {
		tick_fh = clock_time ();
	}
	else if (clock_time_exceed(tick_fh, 3000000))  {  //@@@@@@
		chn_pkt[ll_chn_sel_chg] = 0;
		chn_pkt[!ll_chn_sel_chg] = 10;  //@@@@@
	}
#endif

	if (ll_chn_hold) {
		ll_chn_hold--;
		chn_pkt[0] = chn_pkt[1] = 0;
	}
	int diff = chn_pkt[ll_chn_sel] - chn_pkt[!ll_chn_sel];
	int hit_th = diff > 8;   //@@@@@@
	if (chn_pkt[ll_chn_sel] >= 10 || hit_th) {  //@@@@
		int dual_chn[2];
		dual_chn[0] = mask & 0x0f;
		dual_chn[1] = mask >> 4;
		if (hit_th) { //change channel
			ll_chn_hold = 4;
			chn = dual_chn[!ll_chn_sel];
			for (int i=0; i<8; i++) {
				chn = LL_NEXT_CHANNEL (chn);
				if ((ll_chn_sel && chn != dual_chn[1])) {
					mask = (mask & 0xf0) | chn;
					break;
				}
				else if (!ll_chn_sel && chn != dual_chn[0]) {
					mask = (mask & 0x0f) | (chn << 4);
					break;
				}
			}
			tick_fh = clock_time ();
			fh_num++;
			ll_chn_sel_chg = !ll_chn_sel;		//remember latest channel change
		}
		chn_pkt[0] = chn_pkt[1] = 0;
	}

	return mask;
}

/////////////////////////////////////////////////////////////////////
//	timer1: beacon alignment
//	rx interrupt: buffer management->preprocess->postprocess
//	tx interrupt: post process
/////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
static u32 tick_last_tx;
static u32 tick_timer1_irq;

///////////////////////////////////////////////////////////////////////////////
//	host
///////////////////////////////////////////////////////////////////////////////
u8 *	p_debug_pkt = 0;
void ll_host_init (u8 * pkt)
{
	p_debug_pkt = pkt;

	rf_multi_receiving_init (BIT(0) | BIT(1) | BIT(2));
	reg_dma_rf_tx_addr = (u16)(u32) pkt;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (PKT_BUFF_SIZE>>4);   // rf rx buffer enable & size
	reg_dma_chn_irq_msk = 0;

	reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;

	// timer1 interrupt enable
	reg_tmr1_tick = 0;
	reg_tmr1_capt = CLOCK_SYS_CLOCK_1US * CHANNEL_SLOT_TIME;
	reg_tmr_ctrl |= FLD_TMR1_EN;

	reg_irq_mask |= FLD_IRQ_TMR1_EN | FLD_IRQ_ZB_RT_EN;    //enable RF & timer1 interrupt

}

_attribute_ram_code_ void irq_host_timer1 (void)
{
	//log_event (TR_T_irq_timer1);
	tick_timer1_irq++;
	if (device_packet_received == 1) {
		ll_chn_pkt[ll_chn_sel]++;
	}

	host_channel = get_next_channel_with_mask (chn_mask, host_channel);
#if(!__PROJECT_OTA_MASTER__) 	//@@@@@@ remember: when ota begin, do not hopping
	chn_mask = update_channel_mask(chn_mask, host_channel, ll_chn_pkt);
#endif

	device_packet_received = 0;
	//log_data (TR_24_rf_channel, host_channel);

	rf_set_channel (host_channel, RF_CHN_TABLE);
	rf_set_rxmode ();
}


_attribute_ram_code_ void irq_host_rx(void)
{
	//log_event (TR_T_rf_irq_rx);
	u8 * raw_pkt = (u8 *) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE);
	rf_rx_wptr = (rf_rx_wptr + 1) & 1;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE); //set next buffer

	reg_rf_irq_status = FLD_RF_IRQ_RX;

	if	(	raw_pkt[0] >= 15 &&
			RF_PACKET_LENGTH_OK(raw_pkt) &&
			RF_PACKET_CRC_OK(raw_pkt) )	{

		// scheduling TX, 150 us interval
		rf_set_tx_rx_off ();
		u32 t = *((u32 *) (raw_pkt + 8));
		t += ((raw_pkt[12] + 8) * 4 + 0) * CLOCK_SYS_CLOCK_1US;
		u32 diff = t - clock_time ();
		if (diff > (300 * CLOCK_SYS_CLOCK_1US))
		{
			t = clock_time () + 5 * CLOCK_SYS_CLOCK_1US;
		}
		rf_start_stx (p_debug_pkt, t);

		callback_rx_function p_post = NULL;
		extern u8 *  rf_rx_response(u8 * p, void *);
		u8 * p_ack = rf_rx_response (raw_pkt, &p_post);
		if (p_ack) {

			rf_set_power_level_index (RF_POWER_LEVEL_MAX);
			rf_set_ack_packet (p_ack);
#if(__PROJECT_OTA_MASTER__)  //
			if( ((rf_packet_ack_pairing_t *)p_ack)->proto == RF_PROTO_BYTE ){
#endif

			// assume running @ 32MHz, 32M/1024: 32 us resolution
			((rf_packet_ack_pairing_t *)p_ack)->rssi = raw_pkt[4];
			((rf_packet_ack_pairing_t *)p_ack)->chn = chn_mask;
			((rf_packet_ack_pairing_t *)p_ack)->per =
					(ll_chn_pkt[0] & 0xf) | ((ll_chn_pkt[1] & 0xf) << 4) ;
			//((rf_packet_ack_mouse_t *)p_ack)->info = fh_num;
		#if(CLOCK_SYS_CLOCK_HZ == 32000000)
			((rf_packet_ack_pairing_t *)p_ack)->tick = ((reg_tmr1_tick >> 10) & 0xff) |
													((tick_timer1_irq & 0xff) << 8);
		#elif(CLOCK_SYS_CLOCK_HZ == 24000000)
			((rf_packet_ack_pairing_t *)p_ack)->tick = ((reg_tmr1_tick * 170 >> 17) & 0xff) |
													((tick_timer1_irq & 0xff) << 8);
		#else				// 16 MHz
			((rf_packet_ack_pairing_t *)p_ack)->tick = ((reg_tmr1_tick >> 9) & 0xff) |
										((tick_timer1_irq & 0xff) << 8);
		#endif

#if(__PROJECT_OTA_MASTER__)
			}
#endif
		}
		else {		//cancel TX
			rf_stop_trx ();
		}

		if (p_post) {
			(*p_post) (raw_pkt);
		}
		raw_pkt[0] = 1;
		device_packet_received++;
	}
}


_attribute_ram_code_ void irq_host_tx(void)
{
	tick_last_tx = clock_time ();
	rf_set_channel (host_channel, RF_CHN_TABLE);
	rf_set_rxmode ();

	reg_rf_irq_status = FLD_RF_IRQ_TX;
}

