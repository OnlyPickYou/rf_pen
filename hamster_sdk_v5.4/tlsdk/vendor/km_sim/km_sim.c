#if(1)

#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../link_layer/rf_ll.h"
#include "../common/trace.h"

#define			ADR_ACCESS_CODE0		0x3f00
#define			ADR_DEVICE_ID			0x3f02

u32			mode_link = 0;
u32			access_code1;

rf_packet_pairing_t	pkt_pairing = {
		sizeof (rf_packet_pairing_t) - 4,	// dma_len
#if RF_FAST_MODE_1M
		RF_PROTO_BYTE,
		sizeof (rf_packet_pairing_t) - 6,	// rf_len
#else
		sizeof (rf_packet_pairing_t) - 5,	// rf_len
		RF_PROTO_BYTE,						// proto
#endif
		PKT_FLOW_DIR | FRAME_TYPE_PARING,	// flow
		FRAME_TYPE_MOUSE,					// type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// reserved
		0x01010101,			// device id
};


rf_packet_mouse_t	pkt_km = {
		sizeof (rf_packet_mouse_t) - 4,	// dma_len

		sizeof (rf_packet_mouse_t) - 5,	// rf_len
		RF_PROTO_BYTE,		// proto
		PKT_FLOW_DIR,		// flow
		FRAME_TYPE_MOUSE,					// type

//		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// number of frame
};

_attribute_ram_code_ void irq_handler(void)
{

#if 0
	u32 src = reg_irq_src;
	if(src & FLD_IRQ_GPIO_RISC2_EN){
		reg_irq_src = FLD_IRQ_GPIO_RISC2_EN;
		gpio_user_irq_handler();
	}
#endif

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
			//access_code1 = p_pkt->gid1;
			rf_set_access_code1 (p_pkt->gid1);
			mode_link = 1;
			return 1;
		}
		////////// end of PIPE1 /////////////////////////////////////
		///////////// PIPE1: ACK /////////////////////////////
		else if (p_pkt->type == FRAME_TYPE_ACK_MOUSE) {
			rf_rx_mouse++;
			return 1;
		}
		////////// end of PIPE1 /////////////////////////////////////
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////
//	keyboard/mouse data management
////////////////////////////////////////////////////////////////////////
mouse_data_t     km_data[MOUSE_FRAME_DATA_NUM];
mouse_data_t     mouse_data;

int	km_wptr = 0;
int	km_rptr = 0;
int	km_dat_sending = 0;

void km_data_add (u32 * s, int len)
{
	memcpy4 ((u32 *)&km_data[km_wptr&3], s, len);
	km_wptr = (km_wptr + 1) & 7;
	if ( ((km_wptr - km_rptr) & 7) > 4 ) {	//overwrite older data
		km_rptr = (km_wptr - 4) & 7;
	}
}


int km_data_get ()
{
	if (km_dat_sending) {
		return 1;
	}
	pkt_km.pno = 0;
	for (int i=0; km_rptr != km_wptr && i<4; i++) {
	//while (km_rptr != km_wptr) {
		memcpy ((void *) &pkt_km.data[sizeof(mouse_data_t) * pkt_km.pno++],
				(void *) &km_data[km_rptr & 3],
				sizeof(mouse_data_t));
		km_rptr = (km_rptr + 1) & 7;
	}
	if (pkt_km.pno) {		//new frame
		pkt_km.seq_no++;
		km_dat_sending = 1;
	}
	return pkt_km.pno;
}


void km_data_send_ok ()
{
	km_dat_sending = 0;
}

int	get_mouse_event ()
{
	static u32 fno;
	mouse_data.x = fno & BIT(7) ? 6 : -6;
	mouse_data.y = fno & BIT(6) ? -6 : 6;
	fno++;
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////
#if 0
u32		tick_1mhz,		tick_8ms;
void proc_clock_1mhz ()
{
	cpu_rc_tracking_en (0);
	rf_power_enable (0);
	tick_1mhz = clock_time ();
	reg_clk_sel = 0x3f;
}

void proc_clock_normal (int ns)
{
	u32 tc, t;
	do {
		tc = clock_time ();
		t = (tc - tick_1mhz) * 969 + (tick_1mhz - tick_8ms) * 125;
	} while (t < ns);

	tick_8ms = tc;
	analog_write (0x05, 0x80);			//turn on crystal
	sleep_us (500 >> 3);				//wait crystal clock settle
	reg_clk_sel = 0x24;
	cpu_rc_tracking_en (RC_TRACKING_32M_ENABLE | RC_TRACKING_32K_ENABLE);
}


u8 get_32k_tick ()
{
	int t = -1;
	for (int i=0; i<4; i++) {
		u8 t1 = analog_read (0x20);
		if (t1 == t)
			return t;
		t = t1;
	}
	return t;
}

#endif

void proc_suspend (void)
{
#if 0
	cpu_rc_tracking_en (0);
	cpu_sleep_wakeup (0, PM_WAKEUP_TIMER, 16);	// 8ms wakeup

	rf_drv_init(1);
	clock_init();
	sleep_us (500);
	cpu_rc_tracking_en (RC_TRACKING_32K_ENABLE);
	sleep_us (500);

#elif 1

	static u32 tick_8ms;
	while (!clock_time_exceed (tick_8ms, 8000));
	tick_8ms = clock_time ();

#elif 1

	cpu_sleep_wakeup_rc (0, PM_WAKEUP_TIMER | PM_WAKEUP_PAD, 8);	// 8ms wakeup

	//cpu_sleep_wakeup_rc (0, PM_WAKEUP_PAD, 8000 * CLOCK_SYS_CLOCK_1US);	// 8ms wakeup

	rf_drv_init(1);

	sleep_us (500);

#else

	static u32 tick_8ms;
	while (!clock_time_exceed (tick_8ms, 8000));
	tick_8ms = clock_time ();

#endif

	static u32	dbg_8ms;
	dbg_8ms ++;
	if (dbg_8ms > 10) {
		//while (1);
	}
}

///////////////////////////////////////////////////////////////////////////////////
void  user_init(void)
{
	//device_info_load ();

	u16 *pac = (u16 *) ADR_ACCESS_CODE0;
	if (*pac != U16_MAX) {
		rf_set_access_code0 ( rf_access_code_16to32(*pac) );
	}
	pac = (u16 *)ADR_DEVICE_ID;
	if (*pac != U16_MAX) {
		pkt_pairing.did = *pac;
	}
	ll_device_init ();

	//cpu_set_system_tick (0);
    
	rf_receiving_pipe_enble( 0x3f);	// channel mask


	usb_dp_pullup_en (1);

	rf_set_power_level_index (RF_POWER_8dBm);
	//rf_set_power_level_index (RF_POWER_0dBm);
	//rf_set_power_level_index (RF_POWER_m12dBm);

	cpu_set_gpio_wakeup (GPIO_GP10, 0, 1);

	gpio_write (GPIO_GP10, 1);

	swire2usb_init();
}

u32		dbg_loop;

#if 1
void main_loop(void)
{
	static u32 dbg_t0, dbg_t1;

	dbg_loop ++;
	if (mode_link) {
		if (get_mouse_event ()) {		// add data to buffer
			km_data_add ((u32 *)&mouse_data, sizeof (mouse_data_t));
		}

		if (km_data_get ()) {			// get data from buffer
			rf_set_tx_pipe (PIPE_MOUSE);
			if (device_send_packet ((u8 *)&pkt_km, 650, 3, 0)) {
				km_data_send_ok ();
			}
			else {
				pkt_km.per ++;
			}
		}
	}
	else {
		rf_set_tx_pipe (PIPE_PARING);
		device_send_packet ((u8 *)&pkt_pairing, 550, 0, 0);
		//rf_power_enable (1);
		sleep_us (700);
		rf_power_enable (0);
	}

	dbg_t1 = clock_time () - dbg_t0;
	if (dbg_loop & 0) {
		while (1) {write_reg8 (0x808008, read_reg8(0x808008) + 1);}
	}
    
    log_event( TR_T_MS_SLEEP );
	proc_suspend ();
    
    log_event( TR_T_MS_WK );
	dbg_t0 = clock_time ();

	ll_add_clock_time (8000);

	//pkt_km.seq_no++;
	pkt_pairing.seq_no++;

	proc_debug ();
}
#else

void main_loop(void)
{
	static u32 dbg_t0, dbg_t1;

	dbg_loop ++;

	device_send_packet ((u8 *)&pkt_pairing, 550, 0, 0);
	//rf_power_enable (1);
	sleep_us (10000);
	//rf_power_enable (0);


	if (analog_read (0x1a)) {
		sleep_us (90000);
	}
	else {
		analog_write (0x1a, 0x01);
		cpu_sleep_wakeup_rc (1, PM_WAKEUP_TIMER | PM_WAKEUP_PAD, 90);	// 8ms wakeup
	}



    pkt_pairing.seq_no++;

	//proc_debug ();
}

#endif



#else

#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"

//#include "debug_paring.h"  //sihui_debug
int test_mode_paring;

volatile int link_flag = 0;

rf_packet_pairing_t	pkt_pairing = {
		sizeof (rf_packet_pairing_t) - 4,	// dma_len

		sizeof (rf_packet_pairing_t) - 5,	// rf_len
		RF_PROTO_BYTE,		// proto
		PKT_FLOW_DIR,		// flow
		0,					// type

		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// reserved
		0x01010101,			// device id   //sihui_debug  0x01010101
};


rf_packet_mouse_t	pkt_km = {
		sizeof (rf_packet_mouse_t) - 4,	// dma_len

		sizeof (rf_packet_mouse_t) - 5,	// rf_len
		RF_PROTO_BYTE,		// proto
		PKT_FLOW_DIR,		// flow
		0,					// type

		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		0,					// number of frame
};

_attribute_ram_code_ void irq_handler(void)
{

#if 0
	u32 src = reg_irq_src;
	if(src & FLD_IRQ_GPIO_RISC2_EN){
		reg_irq_src = FLD_IRQ_GPIO_RISC2_EN;
		gpio_user_irq_handler();
	}
#endif

	u8  src_rf = reg_rf_rx_status;
	if(src_rf & FLD_RF_RX_INTR){
		irq_device_rx();
	}

	if(src_rf & FLD_RF_TX_INTR){
		irq_device_tx();
	}
}

int  rf_rx_process(u8 * p)
{
	rf_packet_ack_pairing_t *p_pkt = (rf_packet_ack_pairing_t *) (p + 8);
	static u32 rf_rx_mouse;
	if (p_pkt->proto == RF_PROTO_BYTE) {
		///////////////  Paring/Link ACK //////////////////////////
		if (p_pkt->type == FRAME_TYPE_ACK && p_pkt->did == pkt_pairing.did) {	//paring/link request
			test_mode_paring = 0;    //sihui_debug
			link_flag = 1;           //sihui_debug
			rf_set_access_code0 (p_pkt->gid1);
			return 1;
		}
		///////////// PIPE1: ACK /////////////////////////////
		else if (p_pkt->type == FRAME_TYPE_ACK_MOUSE) {
			///////////////  mouse packet //////////////////////////
			test_mode_paring = 0;    //sihui_debug
			rf_rx_mouse++;
			return 1;
		}
		////////// end of PIPE1 /////////////////////////////////////
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////
//	keyboard/mouse data management
////////////////////////////////////////////////////////////////////////
mouse_data_t     km_data[MOUSE_FRAME_DATA_NUM];
mouse_data_t     mouse_data;

int	km_wptr = 0;
int	km_rptr = 0;
int	km_dat_sending = 0;

void km_data_add (u32 * s, int len)
{
	memcpy4 ((u32 *)&km_data[km_wptr], s, len);
	km_wptr = (km_wptr + 1) & 3;
	if (km_wptr == km_rptr) {	//overwrite older data
		km_rptr = (km_rptr + 1) & 3;
	}
}


int km_data_get ()
{
	if (km_dat_sending) {
		return 1;          //sihui_debug   pay attention
	}
	pkt_km.pno = 0;
	while (km_rptr != km_wptr) {
		memcpy ((void *) &pkt_km.data[sizeof(mouse_data_t) * pkt_km.pno++],
				(void *) &km_data[km_rptr],
				sizeof(mouse_data_t));
		km_rptr = (km_rptr + 1) & 3;
	}
	if (pkt_km.pno) {		//new frame
		pkt_km.seq_no++;
		km_dat_sending = 1;
	}
	return pkt_km.pno;
}


void km_data_send_ok ()
{
	km_dat_sending = 0;
}

int	get_mouse_event ()
{
	static u32 fno;
	mouse_data.x = fno & BIT(7) ? 6 : -6;
	mouse_data.y = fno & BIT(6) ? -6 : 6;
	fno++;
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////
void proc_suspend (void)
{
	while (!cpu_get_32k_tick ());
	while (cpu_get_32k_tick ());
	sleep_us (500);
}

///////////////////////////////////////////////////////////////////////////////////



void  user_init(void)
{
	while (1);
	//device_info_load ();
	host_ll_device_init ();

	cpu_set_32k_tick_max (8, 1);
	//usb_dp_pullup_en (1);
	//reg_wakeup_en = FLD_WAKEUP_SRC_USB;

	//pkt_km.gid = PIPE1_CODE;    //debug_paring


#if(SIMULATE_DEVICE1)
	SET_GPIO_INPUT;             //debug_paring
	pkt_pairing.did = 0x13579bdf;
#else
	pkt_pairing.did = 0x02468ace;
#endif

}



void main_loop(void)
{



#if(0)    //silumate_device1:5330_鼠标(有按键)，可以获得配对请求，发送按键值

	int valid_data = proc_button();
	proc_paring();

	if(test_mode_paring)  //有配对请求，发配对包
	{
			//pkt_km.gid = PIPE0_CODE;
			pkt_pairing.flow = test_mode_paring;
			device_send_packet ((u8 *)&pkt_pairing, 650, 3, 0);

	}
	else //无配对请求，发link包  或者数据包
	{
			pkt_pairing.flow = PKT_FLOW_DIR;
			if(link_flag == 0)   //一直没有link上过，无条件发link包，得到相应后不再发
			{
					device_send_packet ((u8 *)&pkt_pairing, 650, 3, 0);
			}
			else  //已经link上，如果有按键事件,发数据包
			{
					km_data_add ((u32 *)&mouse_data, sizeof (mouse_data_t));

					if (km_data_get ())
					{
							//device_send_packet ((u8 *)&pkt_km, 650, 3, 0);
							if (device_send_packet ((u8 *)&pkt_km, 650, 3, 0))
							{
									km_data_send_ok ();
							}
					}
			}
	}



#else    //silumate_device1: 5330_dongle，V pattern
	pkt_pairing.flow = PKT_FLOW_DIR;
	if(link_flag == 0)   //一直没有link上过，无条件发link包，得到相应后不再发
	{
			rf_set_tx_pipe (PIPE_PARING);
			device_send_packet ((u8 *)&pkt_pairing, 650, 3, 0);
	}
	else  //已经link上，如果有按键事件,发数据包
	{
			if (get_mouse_event ()) {		// add data to buffer
				rf_set_tx_pipe (PIPE_MOUSE);
				km_data_add ((u32 *)&mouse_data, sizeof (mouse_data_t));
			}

			if (km_data_get ()) {			// get data from buffer
				if (device_send_packet ((u8 *)&pkt_km, 650, 3, 0)) {
					km_data_send_ok ();
				}
			}
	}


#endif

	proc_suspend ();
	ll_add_clock_time (8000);


}


#endif
