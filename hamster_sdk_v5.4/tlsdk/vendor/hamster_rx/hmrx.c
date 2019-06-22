#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../link_layer/rf_ll.h"
#include "hmrx_custom.h"
#include "hmrx_usb.h"
#include "hmrx_emi.h"
#include "hmrx_suspend.h"
#include "hmrx_rf.h"
#include "trace.h"

//pairing ack head: 80805113
rf_packet_ack_pairing_t	ack_pairing = {
		sizeof (rf_packet_ack_pairing_t) - 4,	// 0x14=24-4,dma_len

		sizeof (rf_packet_ack_pairing_t) - 5,	// 0x13=24-5,rf_len
		RF_PROTO_BYTE,							// 0x51,proto
		PKT_FLOW_DIR,							// 0x80,flow
		FRAME_TYPE_ACK,							// 0x80,type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
		0,					// info0
		0,					// info1
		0,					// info2

		U32_MAX,			// gid1
		U32_MAX,			// device id
};

//ack empty head: c0805108
rf_ack_empty_t	ack_empty = {
		sizeof (rf_ack_empty_t) - 4,			// 0x09,dma_len

		sizeof (rf_ack_empty_t) - 5,			// 0x08,rf_len
		RF_PROTO_BYTE,							// 0x51,proto
		PKT_FLOW_DIR,							// 0x80,flow
		FRAME_TYPE_ACK_EMPTY,					// 0xc0,type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
};

rf_packet_ack_keyboard_t	ack_keyboard = {
		sizeof (rf_packet_ack_keyboard_t) - 4,	// dma_len

		sizeof (rf_packet_ack_keyboard_t) - 5,	// rf_len 	0x09
		RF_PROTO_BYTE,		// proto 						0x51
		PKT_FLOW_DIR,		// flow 						0x80
		FRAME_TYPE_ACK_KEYBOARD,					// type 0x82

//		PIPE1_CODE,			// gid1

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
		0,					// status
};


extern u8		host_keyboard_status;
extern u32		custom_binding_device_id;

_attribute_ram_code_ void irq_handler(void)
{
	u32 src = reg_irq_src;

	if(src & FLD_IRQ_TMR1_EN){
		irq_host_timer1();
		reg_tmr_sta = FLD_TMR_STA_TMR1;//write 1 to clear
	}

	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_host_rx();        
        //log_event (TR_T_irq_rx);
	}

	if(src_rf & FLD_RF_IRQ_TX){
		irq_host_tx();
	}
}

extern int     rf_paring_enable;

/****************************************************************************************************************
     	callback_pairing
 ***************************************************************************************************************/
void	callback_pairing (u8 * p)
{
	rf_packet_pairing_t *p_pkt = (rf_packet_pairing_t *) (p + 8);
    static u32 rx_rssi = 0;
    if ( 1 ){
        static u8 rssi_last;
        u8 rssi_cur = RECV_PKT_RSSI(p);
        if ( abs(rssi_last - rssi_cur) < 12 ){
            rx_rssi = ( ( (rx_rssi<<1) + rx_rssi + rssi_cur ) >> 2 );
        }
        rssi_last = rssi_cur;
    }
	int  type                   = p_pkt->type;  //pkt->type = : 1: mouse,  2:keyboard
	int  device_not_pair        = custom_binding_device_id == U32_MAX;  // 1.没有配过任何device时，才可以进行auto  2.manual在已经有配对device后，不响应上电配对
	int  rssi_paring_good       = rx_rssi > 23;  //rssi > 23(-87 dbm)   manual、soft 对能量有要求, auto没有
	int  device_paring_flag     = p_pkt->flow & PKT_FLOW_PARING;                            //有PKT_FLOW_PARING 标志的为paring包，否则是link包
	int  device_power_on_paring = p_pkt->flow & PKT_FLOW_TOKEN;                             //有PKT_FLOW_TOKEN 标志的，是上电配对包, manual在已经有配对device后，不响应上电配对


	if(rf_paring_enable && device_not_pair && rssi_paring_good ){
		custom_binding_device_id = p_pkt->did;
	}
}

#define		PER32S128(a, b)			((((a)*31)>>5) + ((b)<<10))

/****************************************************************************************************************
     	callback_keyboard
 ***************************************************************************************************************/
void	callback_keyboard (u8 *p)
{

	rf_packet_keyboard_t *p_pkt = (rf_packet_keyboard_t *)  (p + 8);
	static u8 seq_no_keyboard = 0;
	if (p_pkt->seq_no != seq_no_keyboard) {	//skip same packet
		seq_no_keyboard = p_pkt->seq_no;
		usbkb_hid_report((kb_data_t *)p_pkt->data);
		if ((reg_irq_src & FLD_IRQ_USB_PWDN_EN))
		{
			dongle_resume_request ();		//usb_resme_host should be in main_loop, not in interrupt handler
		}
	}
}

_attribute_ram_code_ u8 *  rf_rx_response(u8 * p, callback_rx_function *p_post)
{

	rf_packet_pairing_t *p_pkt = (rf_packet_pairing_t *) (p + 8);

    static u32 binding_device_flg;
	if (p_pkt->proto == RF_PROTO_BYTE) {
		/////////////// PIPE0: Paring/Link request //////////////////////////
		if (rf_get_pipe(p) == PIPE_PARING) {	//paring/link request
			if (p_pkt->did == custom_binding_device_id) {
				ack_pairing.gid1 = rf_get_access_code1();
                ack_pairing.did = p_pkt->did;
				*p_post = NULL;
				return (u8 *) &ack_pairing;		//send ACK
			}
			else if ( !binding_device_flg ){
				*p_post = (void *) callback_pairing;
				return (u8 *) &ack_empty;
			}
		}
		///////////// PIPE1/PIPE2: data pipe /////////////////////////////
		else if ( rf_get_pipe(p) == PIPE_KEYBOARD  && p_pkt->type == FRAME_TYPE_KEYBOARD ){
			u8 data_valid = 0;

			if(custom_binding_device_id == U32_MAX){  //no did in ram
				custom_binding_device_id= p_pkt->did;
				data_valid = 1;
			}
			else{ //did in ram
				if(custom_binding_device_id == p_pkt->did){
					data_valid = 1;
				}
			}

			if(data_valid){
				binding_device_flg = 1; //working dongle cannot be re-paired
				if(p_pkt->type == FRAME_TYPE_KEYBOARD){  //keyboard
					*p_post = callback_keyboard;
					ack_keyboard.status = host_keyboard_status;
					return (u8 *) &ack_keyboard;
				}
			}
			else{
				*p_post = NULL;
			}
		}
	}

	return (u8 *) &ack_empty;
}




void  user_init(void)
{
	custom_init();
	ll_host_init ((u8 *) &ack_empty);
	usb_dp_pullup_en (1);
	reg_wakeup_en = FLD_WAKEUP_SRC_USB;
	rf_set_power_level_index (RF_POWER_8dBm);
}

//01805117
rf_packet_mouse_t	pkt_mouse = {
		sizeof (rf_packet_mouse_t) - 4,	// dma_len

		sizeof (rf_packet_mouse_t) - 5,	// 0x17,rf_len
		RF_PROTO_BYTE,					// 0x51,proto
		PKT_FLOW_DIR,					// 0x80,flow
		FRAME_TYPE_MOUSE,				// 0x01,type

//		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		1,					// number of frame
};


extern u8 host_channel;
static u8 host_channel_bak;

void main_loop(void)
{
	usb_handle_irq();

	//usb_data_report_proc();

	if (rf_paring_enable && clock_time_exceed(0, 60 * 1000000)){  //1 min
		rf_paring_enable = 0;
	}

	proc_suspend();
}

