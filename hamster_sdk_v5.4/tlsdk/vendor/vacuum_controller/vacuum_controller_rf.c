/*
 * mouse_rf.c
 *
 *  Created on: Feb 12, 2014
 *      Author: xuzhen
 */
#if(__PROJECT_VACUUM__)
#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "vacuum_device_info.h"
#include "vacuum_controller.h"
#include "vacuum_controller_rf.h"
//#include "vacuum_controller_custom.h"
#include "vacuum_controller_button.h"
#include "vacuum_controller_batt.h"
#include "vacuum_device_info.h"
#include "trace.h"

#define		CHANNEL_SLOT_TIME				8000

#define		LL_CHANNEL_SEARCH_TH			60
#define		LL_CHANNEL_SEARCH_FLAG			BIT(16)
#define		LL_NEXT_CHANNEL(c)				((c + 6) & 14)

#define     PKT_BUFF_SIZE   				48


u8  	rf_rx_buff[PKT_BUFF_SIZE*2] __attribute__((aligned(4)));
int		rf_rx_wptr;

u8		device_channel;
u16		ll_chn_tick;

u32		ll_chn_mask = LL_CHANNEL_SEARCH_FLAG;
u32		ll_clock_time;
u32		tick_last_tx;
u32 	get_device_id = U32_MAX;
int		device_packet_received;
int		km_dat_sending = 0;

extern 	u8 btn_save;
extern 	u32 key_scaned;
extern 	u16 custom_binding;
extern int custom_binding_idx;



volatile int		device_ack_received = 0;


u8* kb_rf_pkt = (u8*)&pkt_pairing;

extern vc_data_t vc_event;
#if(WITH_SPELL_PACKET_EN)
extern vc_data_t	 vc_s_event;
#endif

#if(USE_CURRENT_VERSION_1P6)
//02(50/10)510b
rf_packet_pairing_t	pkt_pairing = {
		sizeof (rf_packet_pairing_t) - 4,	// 0x0c=16-4,dma_len
#if RF_FAST_MODE_1M
		RF_PROTO_BYTE,
		sizeof (rf_packet_pairing_t) - 6,	// rf_len
#else
		sizeof (rf_packet_pairing_t) - 5,	// 0x0b=16-5,rf_len
		RF_PROTO_VACUUM,						// 0x51,proto
#endif
		PKT_VACUUM_PARING,					// flow
		FRAME_TYPE_VACUUM,					// 0x05,type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// reserved
		0xdeadbeef,			// device id
};

//0280510f
rf_packet_keyboard_t	pkt_km = {
		sizeof (rf_packet_keyboard_t) - 4,	//0x10=20-4,dma_len

		sizeof (rf_packet_keyboard_t) - 5,	//0x0f=20-5,rf_len
		RF_PROTO_VACUUM,						// 0x5a, proto
		PKT_VACUUM_DATA,						// 0xc6, flow
		FRAME_TYPE_VACUUM,						// 0x05, type

//		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		1,					// pno

		0xdeedbeef,			// did
};



rf_packet_pairing_t     pkt_manual = {		// manual pair / confrim pair
		sizeof (rf_packet_pairing_t) - 4,	// 0x0c=16-4,dma_len
#if RF_FAST_MODE_1M
		RF_PROTO_BYTE,
		sizeof (rf_packet_pairing_t) - 6,	// rf_len
#else
		sizeof (rf_packet_pairing_t) - 5,	// 0x0b=16-5,rf_len
		RF_PROTO_VACUUM,						// 0x5a,proto
#endif
		PKT_VACUUM_PARING,					// flow
		FRAME_TYPE_VACUUM_MANN,				// type, CONF

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// reserved
		0xdeadbeef,			// device id
};

rf_packet_keyboard_t	pkt_conf = {
		sizeof (rf_packet_keyboard_t) - 4,	//0x10=20-4,dma_len

		sizeof (rf_packet_keyboard_t) - 5,	//0x0f=20-5,rf_len
		RF_PROTO_VACUUM,						// 0x5a, proto
		PKT_VACUUM_PARING,						// 0x39, flow
		FRAME_TYPE_VACUUM_CONF,					//0x07,  type   -> confirm pair

//		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		1,					// pno

		0xdeedbeef,			//did
};

#else
//02(50/10)510b
rf_packet_pairing_t	pkt_pairing = {
		sizeof (rf_packet_pairing_t) - 4,	// 0x0c=16-4,dma_len
#if RF_FAST_MODE_1M
		RF_PROTO_BYTE,
		sizeof (rf_packet_pairing_t) - 6,	// rf_len
#else
		sizeof (rf_packet_pairing_t) - 5,	// 0x0b=16-5,rf_len
		RF_PROTO_VACUUM,						// 0x51,proto
#endif
		PKT_VACUUM_PARING,					// flow
		FRAME_TYPE_VACUUM,					// 0x05,type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// reserved
		0xdeadbeef,			// device id
};

#if(WITH_SPELL_PACKET_EN)
rf_packet_vacuum_t	pkt_km = {
		sizeof (rf_packet_vacuum_t) - 4,	//0x10=20-4,dma_len

		sizeof (rf_packet_vacuum_t) - 5,	//0x0f=20-5,rf_len
		RF_PROTO_VACUUM,						// 0x5a, proto
		PKT_VACUUM_DATA,						// 0xc6, flow
		FRAME_TYPE_VACUUM,					// 0x05, type

//		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		1,					// pno
};

#else
//0280510f
rf_packet_keyboard_t	pkt_km = {
		sizeof (rf_packet_keyboard_t) - 4,	//0x10=20-4,dma_len

		sizeof (rf_packet_keyboard_t) - 5,	//0x0f=20-5,rf_len
		RF_PROTO_VACUUM,						// 0x5a, proto
		PKT_VACUUM_DATA,						// 0xc6, flow
		FRAME_TYPE_VACUUM,					// 0x05, type

//		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		1,					// pno
};
#endif

#endif

_attribute_ram_code_ u8	get_next_channel_with_mask(u32 mask, u8 chn)
{
	int chn_high = (mask >> 4) & 0x0f;

	if (mask & LL_CHANNEL_SEARCH_FLAG) {
		return LL_NEXT_CHANNEL (chn);
	}
	else if (chn_high != chn) {
		return chn_high;
	}
	else {
		return mask & 0x0f;
	}
}


/////////////////////////////////////////////////////////////////////////
// device side
/////////////////////////////////////////////////////////////////////////

void ll_device_init (void)
{
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (PKT_BUFF_SIZE>>4);   // rf rx buffer enable & size
	reg_dma_chn_irq_msk = 0;
	reg_irq_mask |= FLD_IRQ_ZB_RT_EN;    //enable RF & timer1 interrupt
	reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;
}

#if(DBG_RF_CHN)

#define DEVICE_CHN_SIZE			128
#define LL_CHN_MASK_SIZE		32

volatile u8  dbg_device_chn[DEVICE_CHN_SIZE];
volatile u8  dbg_device_cnt;

volatile u32 dbg_ll_chn_mask[LL_CHN_MASK_SIZE];
volatile u8  dbg_ll_chn_mask_cnt;

#endif
_attribute_ram_code_ void irq_device_rx(void)
{
	log_task_begin(TR_T_irq_rx);
	u8 * raw_pkt = (u8 *) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE);
	rf_rx_wptr = (rf_rx_wptr + 1) & 1;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE); //set next buffer

	reg_rf_irq_status = FLD_RF_IRQ_RX;

	if	(	raw_pkt[0] >= 15 &&
			RF_PACKET_LENGTH_OK(raw_pkt) &&
			RF_PACKET_CRC_OK(raw_pkt) )	{
		rf_packet_ack_pairing_t *p = (rf_packet_ack_pairing_t *)(raw_pkt + 8);
        rf_power_enable (0);

		log_data(TR_24_p_tick,p->tick);

		extern int  rf_rx_process(u8 *);
		if (rf_rx_process (raw_pkt) && ll_chn_tick != p->tick) {


			ll_chn_tick = p->tick;			//sync time
			device_ack_received = 1;
#if(!FACTORY_PRODUCT_TEST_EN)
			ll_chn_mask = p->chn;			//update channel
#if(DBG_RF_CHN)
			dbg_ll_chn_mask[dbg_ll_chn_mask_cnt++ & 0x1f] = ll_chn_mask;
#endif
#endif
		}
		rf_set_channel (device_channel, RF_CHN_TABLE);
		raw_pkt[0] = 1;
	}
	log_task_end(TR_T_irq_rx);
}


//extern rf_packet_pairing_t	pkt_pairing;
//task_when_rf_func p_task_when_rf = NULL;

_attribute_ram_code_ int	device_send_packet (u8 * p, u32 timeout, int retry, int pairing_link)
{
    while ( !clock_time_exceed (vc_sleep.wakeup_tick, 2000) );    //delay to get stable pll clock
	rf_power_enable (1);

	static	u32 ack_miss_no;
	device_ack_received = 0;
	int i;

	for (i=0; i<=retry; i += 1) {
		rf_set_channel (device_channel, RF_CHN_TABLE);
#if	DBG_GPIO_EN
		if( !(device_channel & 0x0f) ){
			GPIO_TOGGLE(GPIO_RF_SWITCH_T);
		}

		GPIO_HIGH(GPIO_SENDPACKET_T);
		GPIO_LOW(GPIO_SENDPACKET_T);
#endif
#if(DBG_RF_CHN)
		dbg_device_chn[ (dbg_device_cnt++) & 0x7f] = device_channel;
#endif
		u32 t = clock_time ();
		rf_send_packet (p, 300, 0);
		reg_rf_irq_status = 0xffff;
		//rf_rx_process()
//        if ( DO_TASK_WHEN_RF_EN && p_task_when_rf != NULL) {
//           (*p_task_when_rf) ();
//           p_task_when_rf = NULL;
//        }
		while (	!device_ack_received &&
				!clock_time_exceed (t, timeout) &&
				!(reg_rf_irq_status & (FLD_RF_IRX_RETRY_HIT | FLD_RF_IRX_CMD_DONE)) );




		if (device_ack_received) {

			ack_miss_no = 0;
			break;
		}
		ack_miss_no ++;
		if (ack_miss_no >= LL_CHANNEL_SEARCH_TH) {
			ll_chn_mask = LL_CHANNEL_SEARCH_FLAG;
#if(DBG_RF_CHN)
			dbg_ll_chn_mask[dbg_ll_chn_mask_cnt++ & 0x1f] = ll_chn_mask;
#endif
		}

		device_channel = get_next_channel_with_mask (ll_chn_mask, device_channel);

	}

	rf_power_enable (0);
#if(FACTORY_PRODUCT_TEST_EN)
	return 1;
#else

	if ( i <= retry ){// && !vc_status.golden_tx_en) {
		return 1;
	}
	else{
		return 0;
	}
#endif
}

void vc_rf_init(void)
{
	ll_device_init ();
	rf_receiving_pipe_enble(0x3f);	// channel mask
	vc_status.tx_retry = 3;

	if(vc_status.vc_mode == STATE_NORMAL){  	//link OK deep back or flash load pipe1 code OK
    	rf_set_access_code1 (vc_status.dongle_id );
    	kb_rf_pkt = (u8*)&pkt_km;
    	rf_set_tx_pipe (PIPE_VACUUM);
    	rf_set_power_level_index (vc_cust_tx_power);
    }
    else if(vc_status.vc_mode <= STATE_PAIRING ){ //link ERR deepback or poweron flash load pipe1 code fail
    	rf_set_access_code1 ( U32_MAX );
    	kb_rf_pkt = (u8*)&pkt_pairing;
    	rf_set_power_level_index (vc_cust_tx_power_paring);
    	rf_set_tx_pipe (PIPE_PARING);

    }
    else if(vc_status.vc_mode == STATE_TEST){
		vc_status.tx_retry = 5;
		rf_set_access_code1 ( U32_MAX );
		kb_rf_pkt = (u8*)&pkt_km;			//set as golden controller, send data packet on PIPE0
		rf_set_tx_pipe (PIPE_PARING);
		//rf_set_power_level_index (vc_cust_tx_power);
		rf_set_power_level_index (RF_POWER_m24dBm);
	}

//	rf_power_enable(1);
//	rf_set_channel (0, RF_CHN_TABLE);
}


#if(USE_CURRENT_VERSION_1P6)
void vc_paring_and_syncing_proc(void)
{
	if( (vc_status.mode_link&LINK_PIPE_CODE_OK) && kb_rf_pkt != (u8*)&pkt_km ){ //if link on,change to KB data pipe   vc_mode to STATE_NORMAL

		kb_rf_pkt = (u8*)&pkt_km;

		vc_status.tx_retry = 2;
    	rf_set_tx_pipe (PIPE_VACUUM);
    	vc_status.vc_mode = STATE_NORMAL;

       	device_info_save();
    	rf_set_power_level_index (vc_cust_tx_power);
    	if(analog_read(PM_REG_END) > 1 && btn_save){			//when deep back and pair successfully, send the last button value
    		vc_event.cnt = 1;
    	    btn_status.btn_new = 1;
    	    vc_event.keycode[0] = btn_save;
    	}

#if TEST_TX_PAIR_MODE
    	if(repair_and_reset.auto_pair == 1){

    		vc_event.cnt = 1;
    		btn_status.btn_new = 1;
    		vc_event.keycode[0] = VC_POWER;

    		repair_and_reset.auto_pair = 0;

    	}
#endif


    	if(repair_and_reset.repair){
    		if(get_device_id != custom_binding){

    			vc_event.cnt = 1;
    			btn_status.btn_new = 1;
    			vc_event.keycode[0] = VC_POWER;

    			set_device_id_in_firmware(vc_status.dongle_id);
    			pkt_km.type = FRAME_TYPE_VACUUM_CONF;
    		}
    		repair_and_reset.repair = 0;
    		repair_and_reset.auto_pair = 0;
    	}
    	else if(repair_and_reset.factory_reset){
    		repair_and_reset.factory_reset = 0;
    		pkt_conf.type = FRAME_TYPE_VACUUM_CONF;

    		callback_auto_paring();
    	}

    }

	if(vc_status.vc_mode == STATE_PAIRING){

		if( (repair_and_reset.repair || repair_and_reset.factory_reset) && (vc_status.loop_cnt >= KB_MANUAL_PARING_MOST)){

			if(repair_and_reset.factory_reset){
				repair_and_reset.factory_reset = 0;
				set_device_id_in_firmware(0);
			}
			else if(repair_and_reset.repair){
				repair_and_reset.repair = 0;
				vc_status.mode_link = MANNUAL_PARING_BACK_LINK;	//back to previous pairing information
				rf_set_tx_pipe (PIPE_VACUUM);

				kb_rf_pkt = (u8*)&pkt_km;
				device_info_load();
				rf_set_access_code1 ( vc_status.dongle_id );
				rf_set_power_level_index (vc_cust_tx_power);
			}
		}
	}
}
#else
void vc_paring_and_syncing_proc(void)
{
	if( (vc_status.mode_link&LINK_PIPE_CODE_OK) && kb_rf_pkt != (u8*)&pkt_km ){ //if link on,change to KB data pipe   vc_mode to STATE_NORMAL

		kb_rf_pkt = (u8*)&pkt_km;

		vc_status.tx_retry = 2;
    	rf_set_tx_pipe (PIPE_VACUUM);
    	vc_status.vc_mode = STATE_NORMAL;

       	device_info_save();
    	rf_set_power_level_index (vc_cust_tx_power);
    	if(analog_read(PM_REG_END) > 1 && btn_save){			//when deep back and pair successfully, send the last button value
    		vc_event.cnt = 1;
    	    btn_status.btn_new = 1;
    	    vc_event.keycode[0] = btn_save;
    	}

#if TEST_TX_PAIR_MODE
    	if(repair_and_reset.auto_pair == 1){

    		vc_event.cnt = 1;
    		btn_status.btn_new = 1;
    		vc_event.keycode[0] = VC_POWER;

    		repair_and_reset.auto_pair = 0;

    	}
#endif

    }

}
#endif


_attribute_ram_code_ int  rf_rx_process(u8 * p)
{
	rf_packet_ack_pairing_t *p_pkt = (rf_packet_ack_pairing_t *) (p + 8);
	if (p_pkt->proto == RF_PROTO_VACUUM) {
		pkt_pairing.rssi = p[4];
		///////////////  Paring/Link ACK //////////////////////////
		if ( p_pkt->type == FRAME_TYPE_ACK && (p_pkt->did == pkt_pairing.did) ) {	//paring/link request
			//change to pip2 STATE_NORMAL
			rf_set_access_code1 (p_pkt->gid1);//need change to pipe2 that is for kb's data
			vc_status.mode_link = LINK_WITH_DONGLE_OK;
			get_device_id = p_pkt->gid1;
			return 1;

		}
		////////// end of PIPE1 /////////////////////////////////////
		///////////// PIPE1: ACK /////////////////////////////
		else if (p_pkt->type == FRAME_TYPE_ACK_VACUUM) {
			vc_status.kb_pipe_rssi = p[4];
			//vc_status.host_keyboard_status =((rf_packet_ack_keyboard_t*)p_pkt)->status;

			log_event(TR_T_ACK_RECV);
			return 1;
		}
		////////// end of PIPE1 /////////////////////////////////////
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////
//	keyboard/mouse data management
////////////////////////////////////////////////////////////////////////



u32 rx_rssi = 0;
u8 	rssi_last;
inline void cal_pkt_rssi(void)
{
	if(vc_status.kb_pipe_rssi){
		if(!rx_rssi){
			rx_rssi = vc_status.kb_pipe_rssi;
		}
		else{
			if ( abs(rssi_last - vc_status.kb_pipe_rssi) < 8 ){
				rx_rssi = (rx_rssi*3 + vc_status.kb_pipe_rssi ) >> 2;
			}
		}
		rssi_last = vc_status.kb_pipe_rssi;
		pkt_km.rssi = rx_rssi;

		vc_status.kb_pipe_rssi = 0;  //clear


		//0x32: -60  0x28: -70    0x23: -75   0x1d: -80
		if(pkt_km.rssi < 0x1d){   // < -80
			vc_status.tx_retry = 7;
		}
		else if(pkt_km.rssi < 0x28){  //-70
			vc_status.tx_retry = 5;
		}
		else if(pkt_km.rssi < 0x32){  //-60
			vc_status.tx_retry = 3;
		}
		else{
			vc_status.tx_retry = 2;
		}

#if(LOW_TX_POWER_WHEN_SHORT_DISTANCE)
		if(pkt_km.rssi < 0x35){  //-57   power low
			if(vc_status.tx_power == RF_POWER_2dBm){
				rf_set_power_level_index (vc_status.cust_tx_power);
				vc_status.tx_power = kb_status.cust_tx_power;
			}
		}
		else if(vc_status.tx_power == kb_status.cust_tx_power){  //power high
			rf_set_power_level_index (RF_POWER_2dBm);
			vc_status.tx_power = RF_POWER_2dBm;
		}
#endif
	}
}

//extern vcEvt_buf_t 	vcEvt_buf;
extern u8 valid_batt_value;
extern u8 PowerOn;
extern u8 key_status;
_attribute_ram_code_ void vc_rf_proc( u8 btn_new )
{
	static u32 kb_tx_retry_thresh = 0;



	if (vc_status.mode_link) {
        if ( btn_new ){
    		if(!PowerOn){
    			PowerOn = 1;
    			sleep_us(2000);				//delay time for get stable voltage
    			batt_detect_and_fliter();
    		}
        	//pkt_km.data[0] = vcEvt_buf.value[vcEvt_buf.rptr].cnt;

#if(WITH_SPELL_PACKET_EN)

    		pkt_km.data[sizeof(vc_data_t)] = 0;		//clear cnt

    		vc_event.btn_ctrl |= valid_batt_value ;
    		vc_s_event.btn_ctrl |= valid_batt_value ;

    		memcpy(&pkt_km.data, &vc_event, sizeof(vc_data_t));
    		if(vc_s_event.cnt > 0){
    			memcpy(&pkt_km.data[sizeof(vc_data_t)], &vc_s_event, sizeof(vc_data_t));
    		}
#else
        	vc_event.btn_ctrl = (key_status | valid_batt_value) ;
			memcpy(&pkt_km.data, &vc_event, sizeof(vc_data_t));
#endif
#if (FACTORY_PRODUCT_TEST_EN)
			if(vc_event.keycode[0] == TEST_POWERON)
				//pkt_km.seq_no = 3;
				pkt_km.seq_no++;
			else
				pkt_km.seq_no++;
				// pkt_km.seq_no = 5;

#else
			pkt_km.seq_no++;
			if(analog_read(PM_REG_END) == 1){
				pkt_km.seq_no = (u8)(clock_time());
			}

#endif
			km_dat_sending = 1;
			kb_tx_retry_thresh = 0x400;
		}

		if (km_dat_sending) {
            if ( kb_tx_retry_thresh-- == 0 ){
				km_dat_sending = 0;
            }
		}
	}
	else{
		pkt_pairing.seq_no++;
	}

	vc_status.rf_sending = ((km_dat_sending || !vc_status.mode_link) && (vc_status.vc_mode != STATE_WAIT_DEEP));
	if(vc_status.rf_sending){
		if(device_send_packet ( kb_rf_pkt, 550, vc_status.tx_retry, 0) ){
			km_dat_sending = 0;
			vc_status.no_ack = 0;
		}
		else{
			vc_status.no_ack ++;
			pkt_km.per ++;
		}

		//cal_pkt_rssi();
	}
}

#endif


