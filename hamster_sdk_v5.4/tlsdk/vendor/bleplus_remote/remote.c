#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/tl_audio.h"
#include "../link_layer/rf_ll.h"
#include "../common/trace.h"
#include "../../proj/drivers/audio.h"
#include "../../proj/drivers/adc.h"

#include "rc_audio.h"

#if (__PROJECT_8267_REMOTE__)

#define			ADR_ACCESS_CODE0		0x3f00
#define			ADR_DEVICE_ID			0x3f04
#define			FLG_MIC_ON				BIT(7)

u32			mode_link = 0;
u32			access_code1;
u8			host_status;

rf_packet_pairing_t	pkt_pairing = {
		sizeof (rf_packet_pairing_t) - 4,	// dma_len
#if RF_FAST_MODE_1M
		RF_PROTO_BYTE,
		sizeof (rf_packet_pairing_t) - 6,	// rf_len
#else
		sizeof (rf_packet_pairing_t) - 3,	// rf_len
		RF_PROTO_BYTE,						// proto
#endif
		PKT_FLOW_DIR | FRAME_TYPE_PARING,	// flow
		FRAME_TYPE_MOUSE,					// type

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// reserved
		0x01010101,			// device id
};


rf_packet_remote_t	pkt_km = {
		sizeof (rf_packet_mouse_t) - 4,	// dma_len

		sizeof (rf_packet_mouse_t) - 3,	// rf_len
		RF_PROTO_BYTE,		// proto
		PKT_FLOW_DIR,		// flow
		FRAME_TYPE_MOUSE,					// type

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// number of frame
};

_attribute_ram_code_ void irq_handler(void)
{
	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_device_rx();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		irq_device_tx();
	}
}

int  rf_rx_process(u8 * p)
{
	rf_packet_ack_pairing_t *p_pkt = (rf_packet_ack_pairing_t *) (p + 8);
	static u32 rf_rx_mouse;
	if (p_pkt->proto == RF_PROTO_BYTE) {
		pkt_pairing.rssi = p[4];
		pkt_km.rssi = p[4];
		///////////////  Paring/Link ACK //////////////////////////
		if ( p_pkt->type == FRAME_TYPE_ACK && (p_pkt->did == pkt_pairing.did) ) {	//paring/link request
			access_code1 = p_pkt->gid1;
			rf_set_access_code1 (p_pkt->gid1);
			mode_link = 1;
			return 1;
		}
		////////// end of PIPE1 /////////////////////////////////////
		///////////// PIPE1: ACK /////////////////////////////
		else if (p_pkt->type == FRAME_TYPE_ACK_MOUSE) {
			rf_rx_mouse++;
			host_status = ((rf_packet_ack_keyboard_t *)p_pkt)->status;
			return 1;
		}
		////////// end of PIPE1 /////////////////////////////////////
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
u32			tick_kb;
int		sys_mic = 0;
int		sys_am = 0;

extern u32 tick_key_pressed;

#define	DEBUG_NO_SUSPEND	0

void proc_suspend (void)
{
	static u32 tick_8ms;
#if DEBUG_NO_SUSPEND
	while (!clock_time_exceed (tick_8ms, 8000));
	tick_8ms = clock_time ();
	ll_add_clock_time (8000);
	return;
#endif
	if (sys_mic)		// mic
	{
		while (!clock_time_exceed (tick_8ms, 8000));
		tick_8ms = clock_time ();
		ll_add_clock_time (8000);
	}
#if 1
	else if (clock_time_exceed (tick_key_pressed, 10 * 1000000))	// idle for 10 second
	{
		analog_write (0x34, access_code1);
		analog_write (0x35, access_code1>>8);
		analog_write (0x36, access_code1>>16);
		analog_write (0x37, access_code1>>24);
		analog_write (0x38, mode_link);

		cpu_sleep_wakeup (1, PM_WAKEUP_PAD , 0) ;  //deep
	}
#endif
	else // key only
	{
		cpu_sleep_wakeup (0, PM_WAKEUP_TIMER, clock_time () + 16000 * CLOCK_SYS_CLOCK_1US) ;
	}

}

/////////////////////////////
//	mic enable
/////////////////////////////

void sys_enable_mic (int en)
{
	sys_mic = en;

	gpio_set_output_en (GPIO_PC3, en);		//AMIC Bias output
	gpio_write (GPIO_PC3, en);

	gpio_write(GPIO_LED, en);
}

void sys_enable_airmouse (int en)
{
	sys_am = en;
	airmouse_enable (en);
	gpio_write(GPIO_LED, en);
}

void adc_clock_powerdown()
{
	analog_write(0x06, 0x01 | analog_read(0x06));
}

void adc_clock_powerup()
{
	analog_write(0x06, 0x00 | analog_read(0x06));
}

///////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////
int 	packet_sending = 0;
extern kb_data_t	kb_event;
int offset = 0;
u8		mouse_button = 0;
u16 	BattValue[10] = {0};
void Battery_get_and_filter(u8 Chn_sel, u8 len)
{
	adc_AnaChSet(Chn_sel);
	analog_write(0x06, analog_read(0x06) & 0xfe);				//power on adc clock
	adc_BatteryCheckInit(0);
	int j;
	u16 temp;
	u8 m_num = len >> 1;
	for(int i=0; i<len; i++){
		sleep_us(3);
		BattValue[i] = adc_BatteryValueGet();
		if(i){
			if(BattValue[i] < BattValue[i-1]){
				temp = BattValue[i];
				//BattValue[i] = BattValue[i-1];
				for(j = i-1; j>=0 && BattValue[j] > temp; j--){
					BattValue[j+1] = BattValue[j];
				}
				BattValue[j+1] = BattValue[j];
			}
		}
	}
	BattValue[0] = (BattValue[m_num] +  BattValue[m_num -1] +  BattValue[m_num + 1]) / 3;
}

void main_loop(void)
{
	static u32		dbg_loop;
	int det_kb;


	dbg_loop ++;
	det_kb = proc_keyboard (!packet_sending);

	if (det_kb){
		tick_kb = clock_time ();
	}

	if (sys_mic){
		//adc_clock_powerup();
		proc_mic_encoder ();
	}
//	else{
//		Battery_get_and_filter(C6, 10);
//	}

	if (mode_link) {  //connected
		if (!packet_sending)
		{
			////////////// get mouse data ////////////////////////////////
			packet_sending = km_data_get ((u8 *)&pkt_km.pno);

			offset = (pkt_km.pno & 15) * sizeof (mouse_data_t);

			///////////// get keyboard data ///////////////////////////////
			if (det_kb)
			{
				packet_sending = 1;
				pkt_km.pno |= FRAME_TYPE_REMOTE_KEYBOARD;
				memcpy4 ((void *)&pkt_km.data[offset], (void *)&kb_event, sizeof(kb_data_t));
				offset += sizeof(kb_data_t);
			}

			////////////// get mic data ///////////////////////////////////
			if (sys_mic && (host_status & FLG_MIC_ON))
			{
				if (mic_encoder_data_ready ((int *)&pkt_km.data[offset]))
				{
					pkt_km.pno |= FRAME_TYPE_REMOTE_MIC;
					offset += ADPCM_PACKET_LEN;
					packet_sending = 1;
				}
			}
		}

		if (sys_mic)
		{
			packet_sending = 1;
		}

		if (packet_sending) {
			// set packet len
			pkt_km.dma_len = offset + 8;
			pkt_km.rf_len = offset + 9;
			rf_set_tx_pipe_long_packet (PIPE_MOUSE);
			if (device_send_packet ((u8 *)&pkt_km, 600+offset*4, 3, 0)) {
				pkt_km.seq_no++;
				packet_sending = 0;
			}
			else {
				pkt_km.per ++;
			}
		}
	}
	else {  //not conencted, paring
		rf_set_tx_pipe (PIPE_PARING);
		pkt_pairing.flow = PKT_FLOW_TOKEN | PKT_FLOW_PARING | PKT_FLOW_ACK_REQ;
		device_send_packet ((u8 *)&pkt_pairing, 550, 1, 0);
		sleep_us (500);
		pkt_pairing.seq_no++;
	}

 	proc_suspend ();
}


///////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////
//s32		buffer_mic[TL_MIC_BUFFER_SIZE>>2];
//adc_clk_powerdown


#define GPIO_DBG		GPIO_PA6

#define CUST_RC32K_INFO_ADDR   0x3f80

void  user_init(void)
{
	 u8 rc32k_cap = *(unsigned char*) CUST_RC32K_INFO_ADDR;
	 if( rc32k_cap != 0xff ){
		 analog_write(0x32,  rc32k_cap );
	 }

#if 0  //debug
	gpio_set_func(GPIO_DBG, AS_GPIO);
	gpio_set_input_en(GPIO_DBG, 0);
	gpio_set_output_en(GPIO_DBG, 1);


	while(1){

		sleep_us(10000);
		gpio_write(GPIO_DBG, 1);
		cpu_sleep_wakeup(0, PM_WAKEUP_TIMER, clock_time() + 24800 *CLOCK_SYS_CLOCK_1US);
		gpio_write(GPIO_DBG, 0);
	}
#endif

	//pc3 pc4 pc5
#if(0)
	config_adc (FLD_ADC_PGA_C45, FLD_ADC_CHN_D0, SYS_16M_AMIC_16K);
#else
	Audio_Init( 1,      //diff mode, set 1 for enable audio
	                0,    //battery checkM, 0: battery direct to AVDD, 1:boost dcdc to VDD
	                AMIC,   //enum AUDIOINPUTCH audio_channel,
	                16,     //unsigned short adc_max_m, reg0x30, follow telink audio guide line
	                6,      //unsigned char adc_max_l,    reg0x32, follow telink audio guide line
	                R2);    //enum AUDIODSR d_samp by 2
	    Audio_FineTuneSampleRate(3);//reg0x30[1:0] 2 bits for fine tuning, divider for slow down sample rate
	    Audio_InputSet(1);//audio input set, ignore the input parameter


	gpio_set_output_en (GPIO_PC3, 1);		//AMIC Bias output
	gpio_write (GPIO_PC3, 1);
	//adc_Init();
#endif
	config_mic_buffer ((u32)buffer_mic, TL_MIC_BUFFER_SIZE);


	u16 *pac = (u16 *) ADR_ACCESS_CODE0;
	if (*pac != U16_MAX) {
		rf_set_access_code0 ( rf_access_code_16to32(*pac) );
	}

	mode_link = analog_read(0x38);
	if (mode_link)		//device paired
	{
		access_code1 =  (analog_read (0x34)<<0 ) | (analog_read (0x35)<<8) |
						(analog_read (0x36)<<16) | (analog_read (0x37)<<24);
		rf_set_access_code1 (access_code1);
	}

	u32 * pacid = (u32 *)ADR_DEVICE_ID;
	if (*pacid != U32_MAX) {
		pkt_pairing.did = *pac;
	}

	ll_device_init ();


	rf_receiving_pipe_enble( 0x3f);	// channel mask


	/////////// enable USB device /////////////////////////////////////
	//usb_log_init();
	//usb_dp_pullup_en (1);

	/////////////// setup LED /////////////////////////////////////////
	gpio_set_func (GPIO_LED, AS_GPIO);
	gpio_set_output_en (GPIO_LED, 1);
	gpio_set_input_en (GPIO_LED, 0);

	////////// set up wakeup source: driver pin of keyboard  //////////
	u32 pin[] = KB_DRIVE_PINS;
	for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		gpio_set_wakeup(pin[i],1,1);  	   //drive pin core(gpio) high wakeup suspend
		cpu_set_gpio_wakeup (pin[i],1,1);  //drive pin pad high wakeup deepsleep
	}
	gpio_core_wakeup_enable_all(1);

	rf_set_power_level_index (RF_POWER_8dBm);
}




#endif  //end of __PROJECT_8267_REMOTE__
