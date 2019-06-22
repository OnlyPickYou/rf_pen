#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/tl_audio.h"
#include "../link_layer/rf_ll.h"
#include "../common/trace.h"

#define			ADR_ACCESS_CODE0		0x7f00
#define			ADR_DEVICE_ID			0x7f02
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
#define DEBUG_NO_SUSPEND		0
void proc_suspend (void)
{
	static u32 tick_8ms;
	#if DEBUG_NO_SUSPEND
		while (!clock_time_exceed (tick_8ms, 8000));
		tick_8ms = clock_time ();
		ll_add_clock_time (8000);
		return;
	#endif
	if (sys_mic || sys_am)		// mic or airmouse on
	{
		while (!clock_time_exceed (tick_8ms, 8000));
		tick_8ms = clock_time ();
		ll_add_clock_time (8000);
	}
	else if (clock_time_exceed (tick_kb, 2000000))	// idle for 2 second
	{
		analog_write (0x34, access_code1);
		analog_write (0x35, access_code1>>8);
		analog_write (0x36, access_code1>>16);
		analog_write (0x37, access_code1>>24);
		analog_write (0x38, mode_link);

		cpu_sleep_wakeup (1, PM_WAKEUP_PAD , 0) ;
	}
	else		// key only
	{
		//while (!clock_time_exceed (tick_8ms, 20000));
		//tick_8ms = clock_time ();
		cpu_sleep_wakeup (0, PM_WAKEUP_TIMER, clock_time () + 20000 * CLOCK_SYS_CLOCK_1US) ;
		pwm_start (PWMID_LED);
		ll_channel_alternate_mode ();
	}

}

/////////////////////////////
//	mic enable
/////////////////////////////

void sys_enable_led ()
{
	if (sys_mic && sys_am)
	{
		rc_led_en (1, 2);
	}
	else if (!sys_mic && !sys_am)
	{
		rc_led_en (0, 2);
	}
	else
	{
		rc_led_en (1, 4);
	}
}

void sys_enable_mic (int en)
{
	sys_mic = en;
	gpio_write (GPIO_PC5, en);
	sys_enable_led ();
}

void sys_enable_airmouse (int en)
{
	sys_am = en;
	airmouse_enable (en);
	sys_enable_led ();
}


///////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////
int 	packet_sending = 0;
extern kb_data_t	kb_event;
int offset = 0;
u8		mouse_button = 0;

void main_loop(void)
{
	static u32		dbg_loop;
	int det_kb;
	int det_mic = 0;

	dbg_loop ++;

	proc_debug ();

	det_kb = proc_keyboard (!packet_sending);
	if (det_kb)
	{
		mouse_button = kb_event.cnt == 1 && kb_event.keycode[0] == VK_LEFTB;
		if (kb_event.cnt || kb_event.ctrl_key)		//non empty or release key
		{
			tick_kb = clock_time ();
			rc_led_en (1, 1);
		}
	}else{
		rc_led_en (0, 1);
	}

	if (sys_am || mouse_button)
	{
		proc_mouse (mouse_button);
	}

	if (sys_mic)
	{
		proc_mic_encoder ();
	}

	if (mode_link) {
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
	else {
		rf_set_tx_pipe (PIPE_PARING);
		pkt_pairing.flow = PKT_FLOW_TOKEN | PKT_FLOW_PARING | PKT_FLOW_ACK_REQ;
		device_send_packet ((u8 *)&pkt_pairing, 550, 0, 0);
		sleep_us (700);
		pkt_pairing.seq_no++;
	}

 	proc_suspend ();
}


///////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////
//s32		buffer_mic[TL_MIC_BUFFER_SIZE>>2];

void  user_init(void)
{
	//device_info_load ();

	gpio_set_output_en (GPIO_PC5, 1);		//AMIC Bias output
	config_adc (FLD_ADC_PGA_C01, FLD_ADC_CHN_D0, SYS_32M_AMIC_16K);
	config_mic_buffer ((u32)buffer_mic, TL_MIC_BUFFER_SIZE);

	//enable I2C function, enable internal 10K pullup
	reg_gpio_pe_gpio &= ~ BIT(7);
	reg_gpio_pf_gpio &= ~ BIT(1);
	analog_write(20, analog_read(20) | (GPIO_PULL_UP_10K<<2) | (GPIO_PULL_UP_10K<<6));	//  CK, DI, pullup 10K
	// enable gyro power
	gpio_write(GPIO_PD7, 0);
	gpio_set_output_en(GPIO_PD7, 1);
	//airmouse_enable (1);

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

	pac = (u16 *)ADR_DEVICE_ID;
	if (*pac != U16_MAX) {
		pkt_pairing.did = *pac;
	}
	ll_device_init ();

	//cpu_set_system_tick (0);

	rf_receiving_pipe_enble( 0x3f);	// channel mask

	swire2usb_init();

	/////////// enable USB device /////////////////////////////////////
	usb_dp_pullup_en (1);

	/////////////// setup LED /////////////////////////////////////////
	gpio_set_func (GPIO_LED, AS_GPIO);
	//reg_pwm_pol =  BIT(PWMID_LED);
	reg_pwm_clk = 255;			//clock by 256
	pwm_set (PWMID_LED, 0x2000, 0x1000);
	pwm_start (PWMID_LED);

	////////// set up wakeup source: driver pin of keyboard  //////////
	u8 pin[] = KB_DRIVE_PINS;
	for (int i=0; i<sizeof (pin); i++)
	{
		cpu_set_gpio_wakeup (pin[i], 1, 1);
	}

	rf_set_power_level_index (RF_POWER_8dBm);

#if	0
	sys_enable_mic (1);
	gpio_set_output_en (GPIO_PB7, 1);		//AMIC Bias output
	gpio_write (GPIO_PB7, 1);
#endif
}
