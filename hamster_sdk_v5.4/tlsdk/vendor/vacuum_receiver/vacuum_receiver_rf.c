/*
 * vaccum_receiver_rf.c
 *
 *  Created on: Aug 20, 2014
 *
 */

#if(__PROJECT_VACUUM_RECEIVER__)

#define  SAVE_INTERUPT_TIME      0

#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
//#include "../link_layer/rf_ll.h"
#include "vacuum_receiver_rf.h"
#include "trace.h"



#if 1
u8		chn_mask_fix = 0;
#else
u8		chn_mask_fix = 0x80;
#endif

u8		chn_mask = 0x80;

#define	CHANNEL_HOPPING_ALWAYS				1

#ifndef	LL_HOST_RX_MULTI_RECEIVING_EN
	#ifdef __PROJECT_DONGLE_8366__
		#define	LL_HOST_RX_MULTI_RECEIVING_EN		0
	#else
		#define	LL_HOST_RX_MULTI_RECEIVING_EN		0
	#endif
#endif

#ifndef			CHANNEL_SLOT_TIME
#define			CHANNEL_SLOT_TIME			8000
#endif

#ifndef		PKT_BUFF_SIZE
#if	(MCU_CORE_TYPE && MCU_CORE_TYPE == MCU_CORE_8266)
#define		PKT_BUFF_SIZE		256
#else
#define		PKT_BUFF_SIZE		80
#endif
#endif

#define		FH_CHANNEL_PKT_RCVD_MAX			8
#define		LL_CHANNEL_SYNC_TH				2
#define		LL_CHANNEL_SEARCH_TH			60
#define		LL_CHANNEL_SEARCH_FLAG			BIT(16)
#define		LL_NEXT_CHANNEL(c)				((c + 6) & 14)


u8  	rf_rx_buff[PKT_BUFF_SIZE*2] __attribute__((aligned(4)));
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

volatile int device_ack_received = 0;


/////////////////////////////////////////////////////////////////////
//	link management functions
/////////////////////////////////////////////////////////////////////
#define		reg_debug_cmd		REG_ADDR16(0x8008)

_attribute_ram_code_ u8	get_next_channel_with_mask(u32 mask, u8 chn)
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
_attribute_ram_code_ u8	update_channel_mask(u32 mask, u8 chn, u8* chn_pkt)
{
	static int ll_chn_sel_chg;

#if CHANNEL_HOPPING_ALWAYS
	if (device_packet_received == 1) {
		tick_fh = clock_time ();
	}
	else if (clock_time_exceed(tick_fh, 3000000)){ //fre_hp_always_time_us))  {
		chn_pkt[ll_chn_sel_chg] = 0;
		chn_pkt[!ll_chn_sel_chg] = 10; //frq_hp_chn_pkt_rcvd_max;
	}
#endif

	int hit_th = (chn_pkt[ll_chn_sel] - chn_pkt[!ll_chn_sel]) > 8; //frq_hp_hit_diff_num;
	if (chn_pkt[ll_chn_sel] >= 10 || hit_th) {  //10 max
		int dual_chn[2];
		dual_chn[0] = mask & 0x0f;
		dual_chn[1] = mask >> 4;
		if (hit_th) { //change channel
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
u8 *	p_debug_pkt = 0;

void vc_rx_init (u8 * pkt)
{
	p_debug_pkt = pkt;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (PKT_BUFF_SIZE>>4);   // rf rx buffer enable & size
	reg_dma_chn_irq_msk = 0;

	reg_irq_mask |= FLD_IRQ_ZB_RT_EN;    //enable RF & timer1 interrupt
	reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;
	// timer1 interrupt enable
	//reg_tmr1_tick = 0;
	//reg_tmr1_capt = CLOCK_SYS_CLOCK_1US * CHANNEL_SLOT_TIME;
	//reg_tmr_ctrl |= FLD_TMR1_EN;
}

#define		reg8_dbg_crc_ignore		REG_ADDR8(5)
void irq_vc_rx(void)
{
	static u32 irq_host_rx_no = 0;
	//log_event (TR_T_rf_irq_rx);

	u8 * raw_pkt = (u8 *) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE);
	rf_rx_wptr = (rf_rx_wptr + 1) & 1;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE); //set next buffer
	reg_rf_irq_status = FLD_RF_IRQ_RX;

	//rf_packe_header_t *p = (rf_packet_header_t*)(((u8*)raw_pkt) + 8);
	if	(	raw_pkt[0] >= 15 &&
			RF_PACKET_LENGTH_OK(raw_pkt) &&
			(reg8_dbg_crc_ignore || RF_PACKET_CRC_OK(raw_pkt)) )	{

		//void (*p_post) (u8 *)  = NULL;
		callback_rx_func p_post = NULL;
		extern u8 *  rf_rx_response(u8 * p, void *);
		u8 * p_ack = rf_rx_response (raw_pkt, &p_post);
		if (p_ack) {
			//SetTxMode (host_channel, RF_CHN_TABLE);
			//RF_TX_PA_POWER_LEVEL (1);
			// assume running @ 32MHz, 32M/1024: 32 us resolution
			//sleep_us (10);
			//TxPkt (p_ack);
		}
		if (p_post) {
			(*p_post) (raw_pkt);
		}
	}
	//raw_pkt[0] = 1;

	irq_host_rx_no++;
}

void irq_vc_tx(void)
{
	static u32 tick_last_tx;
	tick_last_tx = clock_time ();
	//SetRxMode (host_channel, RF_CHN_TABLE);
	reg_rf_irq_status = FLD_RF_IRQ_TX;
}

///////////////////////////////////////////////////////////////////////////////
static u32 tick_last_tx;
static u32 tick_timer1_irq;

///////////////////////////////////////////////////////////////////////////////
//	host
///////////////////////////////////////////////////////////////////////////////
#if		(__PROJECT_DONGLE__ || __PROJECT_VACUUM_RECEIVER__)
void ll_host_init (u8 * pkt)
{
	rf_multi_receiving_init (BIT(0) | BIT(1) | BIT(2));
	p_debug_pkt = pkt;
	reg_dma_rf_tx_addr = (u16)(u32) pkt;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (PKT_BUFF_SIZE>>4);   // rf rx buffer enable & size
	reg_dma_chn_irq_msk = 0;

	reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;

	// timer1 interrupt enable
	reg_tmr1_tick = 0;
	reg_tmr1_capt = CLOCK_SYS_CLOCK_1US * CHANNEL_SLOT_TIME;
	reg_tmr_ctrl |= FLD_TMR1_EN;

	//reg_irq_mask = FLD_IRQ_TMR1_EN;    //enable RF & timer1 interrupt
	//reg_irq_mask =  FLD_IRQ_ZB_RT_EN;
	reg_irq_mask |= FLD_IRQ_TMR1_EN | FLD_IRQ_ZB_RT_EN;    //enable RF & timer1 interrupt

}


extern rcv_sleep_t rcv_sleep;
extern u32 suspend_idx_last;
extern u32 suspend_idx_cur;

#if SAVE_INTERUPT_TIME
_attribute_ram_code_ void irq_host_timer1 (void)
{
	//log_event (TR_T_irq_timer1);
	tick_timer1_irq++;
	if (device_packet_received == 1) {
		ll_chn_pkt[ll_chn_sel]++;
	}

	host_channel = get_next_channel_with_mask (chn_mask, host_channel);
	chn_mask = update_channel_mask(chn_mask, host_channel, ll_chn_pkt);

	device_packet_received = 0;

	rf_set_channel (host_channel, RF_CHN_TABLE);
	rf_set_rxmode ();
}
#else
_attribute_ram_code_ void irq_host_timer1 (void)
{
	log_event (TR_T_irq_timer1);
	DBG1_GPIO_TOGGLE;
	tick_timer1_irq++;
	if (device_packet_received == 1) {
		ll_chn_pkt[ll_chn_sel]++;
	}

	if(rcv_sleep.mode == BASIC_SUSPEND_MODE){
		if(suspend_idx_last != suspend_idx_cur){
			suspend_idx_last = suspend_idx_cur;
			host_channel = get_next_channel_with_mask (chn_mask, host_channel);
		}
	}
	else{
		host_channel = get_next_channel_with_mask (chn_mask, host_channel);
		chn_mask = update_channel_mask(chn_mask, host_channel, ll_chn_pkt);
	}
//	if(rcv_sleep.mode != BASIC_SUSPEND_MODE){
//		chn_mask = update_channel_mask(chn_mask, host_channel, ll_chn_pkt);
//	}

//81
	device_packet_received = 0;
	//log_data (TR_24_rf_channel, host_channel);
	if (p_debug_pkt && clock_time_exceed (tick_last_tx, 500000)) {
		//rf_stop_trx ();
		rf_set_channel (15, RF_CHN_TABLE);
		//rf_set_tx_pipe (PIPE_MOUSE);
		//RF_TX_PA_POWER_LEVEL (0);
		((rf_packet_ack_pairing_t *)p_debug_pkt)->tick = reg_tmr1_tick;
		((rf_packet_ack_pairing_t *)p_debug_pkt)->chn = chn_mask;
		((rf_packet_ack_pairing_t *)p_debug_pkt)->info1++;
		//rf_multi_receiving_send_packet (p_debug_pkt);
		rf_set_power_level_index (RF_POWER_LEVEL_MIN);
		rf_send_single_packet (p_debug_pkt);
		//TxPkt (p_debug_pkt);
		tick_last_tx = clock_time ();
	}
	else {

		rf_set_channel (host_channel, RF_CHN_TABLE);
		rf_set_rxmode ();

	}

    log_data(TR_24_chn_mask,host_channel<<8 | chn_mask);

}
#endif
#if DBG_RX_RAM
#define DBG_RAM_SIZE 	32
#define DBG_RAM_NUM		8
u8 dbg_ram[DBG_RAM_SIZE*DBG_RAM_NUM];
u8 dbg_ram_index;

static u8 dbg_rx_irq_cnt;
#endif

_attribute_ram_code_ void irq_host_rx(void)
{
	static u32 irq_host_rx = 0;
#if DBG_RX_RAM
	dbg_rx_irq_cnt++;
#endif
	//log_event (TR_T_rf_irq_rx);
	DBG2_GPIO_TOGGLE;
	u8 * raw_pkt = (u8 *) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE);
	rf_rx_wptr = (rf_rx_wptr + 1) & 1;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE); //set next buffer

	reg_rf_irq_status = FLD_RF_IRQ_RX;
#if DBG_RX_RAM
	memcpy(dbg_ram + DBG_RAM_SIZE*((dbg_ram_index++)&3),raw_pkt,DBG_RAM_SIZE );
#endif
	//rf_packe_header_t *p = (rf_packet_header_t*)(((u8*)raw_pkt) + 8);
	if	(	raw_pkt[0] >= 15 &&
			RF_PACKET_LENGTH_OK(raw_pkt) &&
			RF_PACKET_CRC_OK(raw_pkt) )	{

			log_task_begin(TR_T_irq_rx);

#if (!LL_HOST_RX_MULTI_RECEIVING_EN)
		// scheduling TX, 150 us interval
		rf_set_tx_rx_off ();
		u32 t = *((u32 *) (raw_pkt + 8));
		#ifdef __PROJECT_DONGLE_8366__
				t += ((raw_pkt[12] + 8) * 4 + 0) * CLOCK_SYS_CLOCK_1US;
		#endif
		u32 diff = t - clock_time ();
		if (diff > (300 * CLOCK_SYS_CLOCK_1US))
		{
			t = clock_time () + 5 * CLOCK_SYS_CLOCK_1US;
		}
		rf_start_stx (p_debug_pkt, t);
#endif
		callback_rx_func p_post = NULL;
		extern u8 *  rf_rx_response(u8 * p, void *);
		u8 * p_ack = rf_rx_response (raw_pkt, &p_post);
		if (p_ack) {

			rf_set_power_level_index (RF_POWER_LEVEL_MAX);
			rf_set_ack_packet (p_ack);

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
		#endif
		log_data(TR_24_rx_tick, ((rf_packet_ack_pairing_t *)p_ack)->tick);
		}
		else {		//cancel TX
			rf_stop_trx ();
		}

		if (p_post) {
			(*p_post) (raw_pkt);
		}
		raw_pkt[0] = 1;
		device_packet_received++;


		log_task_end(TR_T_irq_rx);

	}

	irq_host_rx++;
}


_attribute_ram_code_ void irq_host_tx(void)
{
#if(!SAVE_INTERUPT_TIME)
	tick_last_tx = clock_time ();
#endif
	rf_set_channel (host_channel, RF_CHN_TABLE);
	rf_set_rxmode ();

	reg_rf_irq_status = FLD_RF_IRQ_TX;
}

#endif

#endif	/* __PROJECT_VACUUM_RECEIVER__ */

