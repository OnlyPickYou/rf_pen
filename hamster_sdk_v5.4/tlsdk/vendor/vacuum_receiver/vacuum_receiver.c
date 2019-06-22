#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
//#include "../link_layer/rf_ll.h"
#include "vacuum_receiver_custom.h"
#include "vacuum_receiver_emi.h"
#include "vacuum_receiver_suspend.h"
#include "vacuum_receiver_iouart.h"
#include "vacuum_receiver_rf.h"
#include "trace.h"

#ifndef	 TEST_DEBUG_EN
#define	 TEST_DEBUG_EN	0
#endif

u8       KeySave = 0;
u8		 BattSave = 0;
u32 	 KeySentCnt = 0;

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

//pairing ack head: 80805113
rf_packet_ack_pairing_t	ack_pairing = {
		sizeof (rf_packet_ack_pairing_t) - 4,	// 0x14=24-4,dma_len

		sizeof (rf_packet_ack_pairing_t) - 5,	// 0x13=24-5,rf_len
		RF_PROTO_VACUUM,							// 0x51,proto
		PKT_VACUUM_PARING,							// 0x39,flow
		FRAME_TYPE_ACK,								// 0x80,type

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

rcv_sleep_t rcv_sleep ={
	BASIC_WORKING_MODE,			//working mode
	0,
	0,

	0,
	0,

	0,
	1000,		//30s on deepsleep mode
};

rcv_release_t rcv_release = {
		0,
		0,
};

//ack empty head: c0805108
rf_ack_empty_t	ack_empty = {
		sizeof (rf_ack_empty_t) - 4,			// 0x09,dma_len

		sizeof (rf_ack_empty_t) - 5,			// 0x08,rf_len
		RF_PROTO_VACUUM,							// 0x51,proto
		PKT_FLOW_DIR,							// 0x80,flow
		FRAME_TYPE_ACK_EMPTY,					// 0xc0,type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
};

//ack debug head: 4080510f
rf_packet_debug_t	ack_debug = {
		sizeof (rf_packet_debug_t) - 4,	// dma_len,0x10

		sizeof (rf_packet_debug_t) - 5,	// rf_len,0x0f
		RF_PROTO_VACUUM,					// proto,0x51
		PKT_FLOW_DIR,					// flow,0x80
		FRAME_TYPE_DEBUG,				// type,0x40

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
		0,					// info0
		0,					// info1
		0,					// info2

		U32_MAX,			// gid1
		U32_MAX,			// did,device id
};



//ack kb head: 82805109
rf_packet_ack_keyboard_t	ack_keyboard = {
		sizeof (rf_packet_ack_keyboard_t) - 4,	// dma_len

		sizeof (rf_packet_ack_keyboard_t) - 5,	// rf_len 	0x09
		RF_PROTO_VACUUM,		// proto 						0x51
		PKT_VACUUM_DATA,		// flow
		FRAME_TYPE_ACK_VACUUM,	// type 0x85

//		PIPE1_CODE,			// gid1

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
		0,					// status
};

data_buff_t data_buff;


extern u8 receiver_emi_test_en;
_attribute_ram_code_ void irq_handler(void)
{
	u32 src = reg_irq_src;

	if(src & FLD_IRQ_TMR1_EN){
		log_task_begin(TR_T_irq_timer1);
		irq_host_timer1();
		log_task_end(TR_T_irq_timer1);
		reg_tmr_sta = FLD_TMR_STA_TMR1;//write 1 to clear
	}

	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_host_rx();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		log_event(TR_T_irq_tx);
		irq_host_tx();
	}

}

u32	get_device_id (u8 type)
{
	return U32_MAX;
}



/////////////////////////////////////////////
// dongle_custom.c  中的变量
/////////////////////////////////////////////
extern int     auto_paring_enable;



/*
 * rsv_save_buff(kb_data_t *data)
 */
#define MAX_BTN_SIZE		3
_attribute_ram_code_ void rsv_save_buff(kb_data_t *data)
{
	//int i = 0;
	if(data->cnt > 0)
	{

		data_buff.data[data_buff.wptr] = data->keycode[0];
		data_buff.batt[data_buff.wptr] = data->ctrl_key;

		data_buff.wptr++;
		data_buff.wptr &= 15;
		if((data_buff.wptr - data_buff.rptr) >= 15){
			data_buff.rptr++;
		}
	}
}



/****************************************************************************************************************
     	callback_pairing

函数调用条件：  非golden_dongle的 link包/paring包可以进入该函数
              golden_dongle发link/paring包的时候，dongle直接给PIPE1_CODE，不会进该函数
-----------------------------------------------------------------------------------------------------------------
        配对类型         |   存储配对设备ID号方式        |   是否响应paring包和link包              |        触发配对的条件
-----------------------------------------------------------------------------------------------------------------
auto_paring   | 存入ram，不存入firmware  |   响应 paring包和link包                     |  1.ram中没有存储配对过的设备ID
soft_paring   |   存入firmware和ram     	|   响应paring包，不响应link包        |  1.配对时间允许内
			  |						    |					    		|  2.必须是paring包
	          |  						|								|  3.能量满足
manual_paring |   存入firmware和ram    	|   响应paring包，不响应link包        |  1.配对时间允许内
			  |							|						        |  2.必须是paring包
			  |							|							    |  3.能量满足
			  |						    |							    |  4.ram中已经存储配对ID后，不响应上电配对包
------------------------------------------------------------------------------------------------------------------
 ***************************************************************************************************************/
_attribute_ram_code_ void	callback_pairing (u8 * p)
{

	log_task_begin(TR_T_paring);

	// if valid device, return PIPE1_CODE
	// device_id == 0: ROM device

	u8 rssi_cur;
	u8 first_rcv_rssi = 0;
	rf_packet_pairing_t *p_pkt = (rf_packet_pairing_t *) (p + 8);
    static u32 rx_rssi = 0;

        if ( p_pkt->flow == PKT_VACUUM_PARING){
            static u8 rssi_last;
            rssi_cur = RECV_PKT_RSSI(p);
            if ( abs(rssi_last - rssi_cur) < 12 ){
                rx_rssi = ( (rx_rssi<<1) + rx_rssi + rssi_cur + 3 ) >> 2;
            }
            rssi_last = rssi_cur;
        }

    log_data(TR_24_paring_rssi,(rx_rssi&0xff)<<8 | rssi_cur);

	int  rssi_paring_good       = rx_rssi > 23;  //rssi > 23(-87 dbm)   manual、soft 对能量有要求, auto没有                       //有PKT_FLOW_PARING 标志的为paring包，否则是link包                         //有PKT_FLOW_TOKEN 标志的，是上电配对包, manual在已经有配对device后，不响应上电配对


	if( auto_paring_enable && p_pkt->flow== PKT_VACUUM_PARING && rssi_paring_good ){			//any controller can pair with this receiver
		log_event(TR_T_store_id);
		set_device_id_in_ram(p_pkt->did);                     //auto paring 配对距离限制
	}

	log_task_end(TR_T_paring);
}

#define		PER32S128(a, b)			((((a)*31)>>5) + ((b)<<10))

#if DBG_RX_RSSI
u32 	dbg_rssi_value = 0;
#endif
/****************************************************************************************************************

 ***************************************************************************************************************/


_attribute_ram_code_ void callback_vacuum (u8 *p)
{

#if(WITH_SPELL_PACKET_EN)
	rf_packet_vacuum_t *p_pkt = (rf_packet_vacuum_t *)(p + 8);

	static u8 seq_no_keyboard = 0;
	if (p_pkt->seq_no != seq_no_keyboard && p_pkt->rf_len == (sizeof(rf_packet_vacuum_t) - 5)){

		seq_no_keyboard = p_pkt->seq_no;

		kb_data_t *rf_data = (kb_data_t *)(p_pkt->data);
		kb_data_t *rf_s_data = (kb_data_t *)(p_pkt->data + sizeof(kb_data_t));


		#if(WITH_REPEAT_AND_RELEASE_FUNC )
				if( (rf_data->ctrl_key) & 0x30){		//the key have been released
					KeySentCnt++;
					if(rf_data->ctrl_key & 0x10){
						KeySave = rf_data->keycode[0];
					}

					rsv_save_buff(rf_data);		//save kb data to cache
					BattSave = (rf_data->ctrl_key & 0x0f);
					rcv_release.not_release_flg = (rf_data->ctrl_key & 0x20) ? 0 : 1;
				}
				else{
					rcv_release.not_release_flg = 1;			// key repeat, not release
				}

				if( rf_s_data->cnt > 0){
					if( (rf_s_data->ctrl_key) & 0x30){		//the key have been released
						if(rf_s_data->ctrl_key & 0x10){
							KeySave = rf_s_data->keycode[0];
						}

						rsv_save_buff(rf_s_data);		//save kb data to cache
						BattSave = (rf_data->ctrl_key & 0x0f);
						rcv_release.not_release_flg = (rf_s_data->ctrl_key & 0x20) ? 0 : 1;
					}
					else{
						rcv_release.not_release_flg = 1;			// key repeat, not release
					}
				}

		#endif
	}
#else
	rf_packet_keyboard_t *p_pkt = (rf_packet_keyboard_t *)(p + 8);

	static u8 seq_no_keyboard = 0;
	//skip same packet,and make sure the packet from controller
	if (p_pkt->seq_no != seq_no_keyboard && p_pkt->rf_len == (sizeof(rf_packet_keyboard_t) - 5)) {

		kb_data_t *rf_data = (kb_data_t *)(p_pkt->data);
		seq_no_keyboard = p_pkt->seq_no;
#if(WITH_REPEAT_AND_RELEASE_FUNC )
		if( (rf_data->ctrl_key) & 0x30){		//the key have been released
			KeySentCnt++;
			if(rf_data->ctrl_key & 0x10){
				KeySave = rf_data->keycode[0];
			}
			rsv_save_buff(rf_data);		//save kb data to cache
			rcv_release.not_release_flg = (rf_data->ctrl_key & 0x20) ? 0 : 1;
		}
		else{
			rcv_release.not_release_flg = 1;			// key repeat, not release
		}


#if(0)

	if(!(rf_data->ctrl_key & 0x10)){		//the key have been released

		if( !(rf_data->ctrl_key >> 4) && !KeyPressCnt ){			//Press key, only once time
			KeyPressCnt ++;
			rf_data->ctrl_key |= 0x10;
			rsv_save_buff(rf_data);		//save kb data to cache
		}
		else{													//Release Key
			KeyPressCnt = 0;
			rsv_save_buff(rf_data);		//save kb data to cache
		}
		rcv_release.not_release_flg = 0;


	}
	else{
		rcv_release.not_release_flg = 1;
	}
#endif

#else
	rsv_save_buff(rf_data);		//save kb data to cache
#endif
#if DBG_RX_RSSI
		dbg_rssi_value = RECV_PKT_RSSI(p);
#endif

	}
#endif
}



#if(USE_CURRENT_VERSION_1P6)
/********************************************************************************
 *
 * vacuum_controller与vacuum_receiver配对
 * (1)auto_pair:
 * 	receiver初始化上电处于auto_pair模式，任何controller都可以与receiver配对。receiver记controller的ID到RAM，
 * 当receiver在pipe1上收到数据包，会将收到的controller的ID与RAM中的ID做比较，若相同则上传数据，否则滤掉。
 *
 * (2)confirm pair
 * controller已与receiver配对(RAM中保存receiver的ID)，并且可以控制receiver(receiver保存controller的ID)，
 * 此时按power + down可进行confirm pair(在pipe1上发type为FRAME_TYPE_VACUUM_CONF的包)。receiver收到之后，
 * 将confrim_device置1， receiver进入confrim状态，不允许其他controller再配对；此时controller的数据包的TYPE
 * 置为(FRAME_TYPE_VACUUM_CONF)，记receiver的ID到OTP/FLASH,重新上电不会再配对，直接发数据包;
 *
 * (3)manual pair
 * receiver已进入confrim状态，controller之前没有与receiver配对过(RAM中没有receiver的ID),此时按(power + down)
 * 进manual pair，在pipe0上发type为FRAME_TYPE_VACUUM_MANN的配对包。receiver收到之后，记controller的ID到RAM。
 * confrim_device置1，不允许其他controller再配对。controller记receiver的ID到OTP/FLASH,重新上电不需要配对，
 * 直接发数据包；
 *
 *(4)receiver重新上电，之前保存在RAM中信息全部丢失，ID为0xffffffff，这是只有在pipe1上收到来自controller的数据
 * 包,不需要进行比较ID,直接上传数据， 同时记controller的ID，若type为FRAME_TYPE_VACUUM_CON则将confrim_device置1，
 * receiver不允许其他controller再配对;
 *
 *(5)factory_reset(解配对)
 *当receiver已经与被controller置为confrim状态，有且只有这个controller才能与receiver解配对。controller按power键
 *上电，在3s之内按up + down键， 进入factory状态，一旦解配对成功。receiver又会回到auto pair(confrim = 0)状态，
 *controller会发auto pair包，再次与receiver配对。
 *auto pair包。
 *

 *******************************************************************************/
_attribute_ram_code_ u8 *  rf_rx_response(u8 * p, callback_rx_func *p_post)
{
	static u32 rf_rx_pkt;
	rf_rx_pkt++;
	rf_packet_pairing_t *p_pkt = (rf_packet_pairing_t *) (p + 8);
	rcv_sleep.suspend_cnt = 0;
    static u32 confrim_device = 0;

	if (p_pkt->proto == RF_PROTO_VACUUM) {
		DBG3_GPIO_TOGGLE;
		if(rcv_sleep.mode == BASIC_SUSPEND_MODE){
			rcv_sleep.mode = BASIC_WORKING_MODE;
		}

#if DBG_RX_RAM
		memcpy(dbg_ram + DBG_RAM_SIZE*((dbg_ram_index++)&7),p,DBG_RAM_SIZE );
#endif

		///////////////  Paring/Link request //////////////////////////
		if (rf_get_pipe(p) == PIPE_PARING) {	//paring/link request
			log_event(TR_T_pipe0_data);
			if((p_pkt->flow == PKT_VACUUM_DATA) && !confrim_device){
				*p_post = callback_vacuum;
				return (u8 *) &ack_keyboard;

			}
			else if ( p_pkt->flow == PKT_VACUUM_PARING  && (((p_pkt->type == FRAME_TYPE_VACUUM) && !confrim_device ) \
					|| ( p_pkt->type == FRAME_TYPE_VACUUM_MANN)) ) {	//auto pair and manual pair
				log_event(TR_T_send_pipe1);
				if( p_pkt->did != get_device_id_from_ram() ){
					*p_post = (void *) callback_pairing;
					return (u8 *) &ack_empty;

				}
				else{
					debug_send_pipe1_code++;
					ack_pairing.did = p_pkt->did;
					ack_pairing.gid1 = rf_get_access_code1();
					confrim_device = (p_pkt->type == FRAME_TYPE_VACUUM_MANN) ? 1 : confrim_device;
					*p_post = NULL;
					return (u8 *) &ack_pairing;		//send ACK

				}
			}
//			else if ( !binding_device){
//				*p_post = (void *) callback_pairing;
//				return (u8 *) &ack_empty;
//			}
		}
		///////////////   vacuum     //////////////////////////
		else if ( rf_get_pipe(p) == PIPE_VACUUM ) {

			log_event(TR_T_pipe1_data);
			if( (p_pkt->type == FRAME_TYPE_VACUUM || p_pkt->type == FRAME_TYPE_VACUUM_CONF) && p_pkt->flow == PKT_VACUUM_DATA){

				if(get_device_id_from_ram() == U32_MAX){
					confrim_device = (p_pkt->type == FRAME_TYPE_VACUUM_CONF) ? 1 : confrim_device;
					set_device_id_in_ram(p_pkt->did);
					*p_post = callback_vacuum;
					return (u8 *) &ack_keyboard;

				}
				else if(get_device_id_from_ram() == p_pkt->did){
					*p_post = callback_vacuum;
					return (u8 *) &ack_keyboard;

				}

			}
			else if( (p_pkt->type == FRAME_TYPE_VACUUM_CONF || p_pkt->type == FRAME_TYPE_VACUUM_RESET) \
					&& p_pkt->flow == PKT_VACUUM_PARING &&  p_pkt->did == get_device_id_from_ram()){

				confrim_device = (p_pkt->type == FRAME_TYPE_VACUUM_CONF) ? 1 : 0;
				*p_post = NULL;
				ack_pairing.gid1 = rf_get_access_code1();
				ack_pairing.did = p_pkt->did;
				return (u8 *) &ack_pairing;		//send ACK

			}
		}
		////////// end of PIPE1 /////////////////////////////////////
	}

	return (u8 *) &ack_empty;
}
#else
_attribute_ram_code_ u8 *  rf_rx_response(u8 * p, callback_rx_func *p_post)
{
	rf_packet_pairing_t *p_pkt = (rf_packet_pairing_t *) (p + 8);
	rcv_sleep.suspend_cnt = 0;
    static u32 binding_device = 0;
	if (p_pkt->proto == RF_PROTO_VACUUM) {
		DBG3_GPIO_TOGGLE;
		if(rcv_sleep.mode == BASIC_SUSPEND_MODE){
			rcv_sleep.mode = BASIC_WORKING_MODE;
		}
#if(WITH_REPEAT_AND_RELEASE_FUNC)
		rcv_release.release_cnt = 0;
#endif
//#if DBG_RX_RAM
//		memcpy(dbg_ram + DBG_RAM_SIZE*((dbg_ram_index++)&7),p,DBG_RAM_SIZE );
//#endif

		///////////////  Paring/Link request(PIPE0) //////////////////////////
		if (rf_get_pipe(p) == PIPE_PARING) {	//paring/link request

			log_event(TR_T_pipe0_data);

			if(rf_get_pipe(p) == PIPE_PARING && (p_pkt->flow == PKT_VACUUM_DATA)){

				*p_post = callback_vacuum;
				//ack_keyboard.status = 0;
				return (u8 *) &ack_keyboard;
			}
			else if ((p_pkt->flow == PKT_VACUUM_PARING) && (p_pkt->type == FRAME_TYPE_VACUUM) ) {

				log_event(TR_T_send_pipe1);
				if( p_pkt->did != get_device_id_from_ram() ){
					*p_post = (void *) callback_pairing;
					return (u8 *) &ack_empty;
				}
				else{
					ack_pairing.gid1 = rf_get_access_code1();
					ack_pairing.did = p_pkt->did;
					*p_post = NULL;
					return (u8 *) &ack_pairing;		//send ACK
				}
			}
		}
		///////////////   PIPE1     //////////////////////////
		else if (rf_get_pipe(p) == PIPE_VACUUM){
			if(p_pkt->type == FRAME_TYPE_VACUUM && p_pkt->flow == PKT_VACUUM_DATA){
				log_event(TR_T_pipe1_data);
				binding_device = 1;
				*p_post = callback_vacuum;
				return (u8 *) &ack_keyboard;
			}
		}
		////////// end of PIPE1 /////////////////////////////////////
	}
	return (u8 *) &ack_empty;
}
#endif

void  user_init(void)
{

#if(DBG_TRACE)
    usb_log_init ();
#endif

	custom_init();

	uart_init();

#if EMI_UART_RX_EN
	uart_rx_emi_init();
#endif
	//ack_debug.gid1 = ack_pairing.gid1 = rf_get_access_code1 ();

	ll_host_init ((u8 *) &ack_debug);

	//usb_dp_pullup_en (1);

	//shut down the on-chip capacitor
	analog_write(0x80, (analog_read(0x80)&0xbf));
	analog_write(0x81, (analog_read(0x81)&0xe0));

	rf_set_power_level_index (RF_POWER_8dBm);
}


extern uart_pkt_data_t uart_pkt_data;
extern uart_pkt_ack_t uart_pkt_ack;


extern u8 *pkt_data;
extern u8 *pkt_ack;


volatile u32 uart_tx_time;				//10ms
volatile u32 uart_tx_delay_time;		//100ms
volatile u32 uart_trans_en_time;		//150ms

volatile u8 uart_retry;
#if RECEIVER_UART_EN
_attribute_ram_code_ void uart_tx_proc(void)
{
    u8 uart_en_flag = 0;
	u32 crc_cnt = 0;
	u8 *p_data = (u8 *)&uart_pkt_data;
	u8 *p_ack  = (u8 *)&uart_pkt_ack;
#if(WITH_REPEAT_AND_RELEASE_FUNC)
	if((data_buff.wptr != data_buff.rptr) || rcv_release.send_p_flg )
	{
		if(rcv_release.send_p_flg)
		{
			//KeyPressCnt = 0;
			uart_pkt_data.data = KeySave;//data_buff.data[data_buff.wptr];
			uart_pkt_data.batt = 0x20 | BattSave;
			rcv_release.send_p_flg = 0;

		}
		else
#else
		if( data_buff.wptr != data_buff.rptr )
#endif
		{
			uart_pkt_data.data = data_buff.data[data_buff.rptr];		//valid data information
			uart_pkt_data.batt = data_buff.batt[data_buff.rptr];		//low battery information

			data_buff.rptr++;
			data_buff.rptr &= 15;
		}
#if DBG_RX_RSSI
		uart_pkt_data.rssi = dbg_rssi_value;
#endif
		uart_en_flag = 1;
	}

	if(uart_en_flag){
		gpio_write(IO_UART_TRANS_EN,1);		//high level enable uart
		uart_tx_delay_time = clock_time();

		while(!clock_time_exceed(uart_tx_delay_time,RECEIVER_TX_DELAY_TIME));	//sleep 100ms

		u8 r = irq_disable();
		for(uart_retry=0; uart_retry<RETRY_TIMES; uart_retry++){
			uart_pkt_data.retry =  uart_retry;
			crc_cnt = 0;
			for(int i=0; i<uart_pkt_data.len-2; i++){		//len-1:crc, len:end_byte
					crc_cnt += *p_data;
					p_data++;
			}

			crc_cnt = (u8)(crc_cnt & 0xff) ^ 0x55;   //crc rule
			uart_pkt_data.crc = (u8)crc_cnt;
			p_data = (u8 *)&uart_pkt_data;
			uart_tx_time = clock_time();
			uart_send_pkt(IO_UART_TX, pkt_data, sizeof(uart_pkt_data_t));//uart_pkt_data.len);
			while(!clock_time_exceed(uart_tx_time,IO_TX_AND_RX_TIME));	//sleep 10ms
			if(uart_rx_and_detect(IO_UART_RX, p_ack, sizeof(uart_pkt_ack_t)) && (uart_pkt_ack.flow == 0x20)){
				memset(p_ack,0,sizeof(uart_pkt_ack_t));
				break;
			}
		}
		irq_restore(r);
		gpio_write(IO_UART_TRANS_EN,0);		//low level disable uart
	}

}
#elif MAINBOARD_UART_EN

void uart_rx_proc(void)
{
	u32 uart_en_flag = gpio_read(MAIN_UART_RECV_EN);
	u32 crc_cnt = 0;
	u8 *p_data = (u8 *)&uart_pkt_data;
	u8 *p_ack = (u8 *)&uart_pkt_ack;
	if(uart_en_flag){
		uart_trans_en_time = clock_time();
		uart_tx_delay_time = clock_time();
		for(int i=0; i<uart_pkt_ack.len-2; i++){		//len-1:crc, len:end_byte
			crc_cnt += *p_ack;
			p_ack++;
		}
		crc_cnt = (u8)(crc_cnt & 0xff) ^ 0x55;   //crc rule
		uart_pkt_ack.crc = (u8)crc_cnt;
		p_ack = &(uart_pkt_ack);

		while(!clock_time_exceed(uart_tx_delay_time,RECEIVER_TX_DELAY_TIME));	//sleep 100ms
	}
	while(uart_en_flag){
		uart_tx_time = clock_time();
		if(uart_rx_and_detect(MAIN_UART_RX, pkt_data, sizeof(uart_pkt_data_t)) && (uart_pkt_data.flow == 0x10)){
			while(!clock_time_exceed(uart_tx_time,IO_TX_AND_RX_TIME));	//sleep 10ms to wait tx
			uart_tx_time = clock_time();
			uart_send_pkt(MAIN_UART_TX, pkt_ack, sizeof(uart_pkt_ack_t));
			while(!clock_time_exceed(uart_tx_time,IO_TX_AND_RX_TIME));	//sleep 10ms
			uart_en_flag = 0;
			//return;
		}
		else
			uart_en_flag = gpio_read(MAIN_UART_RECV_EN);

		if(!uart_en_flag){
			break;
		}
	}

}
#endif

void uart_gpio_disable()
{
	gpio_set_output_en(IO_UART_TX, 0);
	gpio_set_input_en(IO_UART_RX, 0);
	gpio_set_output_en(IO_UART_TRANS_EN,0);
}

void uart_gpio_enable()
{
	gpio_set_output_en(IO_UART_TX, 1);
	gpio_set_input_en(IO_UART_RX, 1);
	gpio_set_output_en(IO_UART_TRANS_EN,1);
	gpio_write(IO_UART_TRANS_EN, 0);
}
extern u8 chn_mask;
extern u8 host_channel;
extern u8 ll_chn_pkt[16];

static u8 host_channel_bak;
static inline void trace_dongle_loop ( void ){
    if ( host_channel_bak != host_channel ){
        host_channel_bak = host_channel;
    }
}

u8 new_emi_cmd = 0;
u8 enter_emi_mode = 0;			//for emi mode

u32 main_loop_tick;
u32 suspend_loop_tick;			//for work state and suspend state

u8 suspend_idx_cur = 0;
u8 suspend_idx_last = 0;

extern vc_emi_data_t vc_emi_data;

void emi_proc_loop(void)
{
	static u32 emi_loop_cnt;
	emi_loop_cnt++;
	vc_emi_process();
#if(SPECIAL_EMI_MODE_EN)
	emi_process( 0x81, vc_emi_data.test_chn_idx, 1, (u8 *)&ack_empty, RF_POWER_8dBm );
#else
	emi_process( 0x81, vc_emi_data.test_chn_idx, vc_emi_data.test_mode_sel, (u8 *)&ack_empty, RF_POWER_8dBm );
#endif
	sleep_us(1000000);			//sleep 1s

	uart_rx_emi_proc();
}



void main_loop(void)
{


	if(!receiver_emi_test_en){


		if(!rcv_sleep.mode){	//working mode
			main_loop_tick = clock_time();

#if(WITH_REPEAT_AND_RELEASE_FUNC)
	if(rcv_release.not_release_flg){
		if(rcv_release.release_cnt++ > RCV_RELEASE_THRESH ){
			rcv_release.send_p_flg = 1;
			rcv_release.not_release_flg = 0;
		}
	}
#endif



#if RECEIVER_UART_EN
			uart_tx_proc();

#elif	MAINBOARD_UART_EN

			uart_rx_proc();

#endif
			rcv_sleep.suspend_cnt++;
			if(rcv_sleep.suspend_cnt > rcv_sleep.thresh_suspend){
				rcv_sleep.mode = BASIC_SUSPEND_MODE;
			}

			//cpu_sleep_wakeup_rc(0, PM_WAKEUP_TIMER, 10);		//main_loop 10ms


			while(!clock_time_exceed(main_loop_tick, RCV_MAIN_LOOP_TIME_MS * 1000));	//10ms main_loop
		}
		else{										//suspend mode

			u8 irq_info = irq_disable();
			cpu_rc_tracking_en (RC_TRACKING_32K_ENABLE);
			uart_gpio_disable();

			cpu_sleep_wakeup_rc(0, PM_WAKEUP_TIMER, 200);

#if	UART_RX_DEBUG
		GPIO_TOGGLE(DBG_UART_RX_GPIO);
#endif
			clock_init();

			cpu_rc_tracking_en (RC_TRACKING_32K_ENABLE);
			uart_gpio_enable();
			rf_power_enable(1);
			suspend_loop_tick = clock_time();
			suspend_idx_cur ++;

			reg_tmr1_tick = (CLOCK_SYS_CLOCK_1US * 7800);
			reg_tmr_ctrl |= FLD_TMR1_EN;
			reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;

			irq_restore(irq_info);

			while(!clock_time_exceed(suspend_loop_tick,17000));

			}
		}
	else{							//emi_mode, only send pair packet
		emi_proc_loop();
	}

}

