
#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../link_layer/rf_ll.h"
#include "../common/rf_frame.h"

#if(__PROJECT_KM_SNIFFER__)

#define		reg16_dbg_cmd		REG_ADDR16(0)
#define		reg8_dbg_chn		REG_ADDR8(0)
#define		reg8_dbg_dev		REG_ADDR8(4)
#define		reg32_dbg_scan		REG_ADDR32(0x708)


const TBLCMDSET tbl_sys_ini[] = {
	0x013b,		0x20,		TCMD_UNDER_BOTH | TCMD_WRITE,	//endpoint 8 buffer size: 32*8=256
	0x013c,		0x40,		TCMD_UNDER_BOTH | TCMD_WRITE,	//threshold for uart mode
	0x013d,		0x01,		TCMD_UNDER_BOTH | TCMD_WRITE,	//FIFO mode, no DMA
	0x0128,		0x00,		TCMD_UNDER_BOTH | TCMD_WRITE,	//endpoint 8 buffer address

	0x074f,		0x01,		TCMD_UNDER_BOTH | TCMD_WRITE,	//enable system timer
	0x042c,		0xe8,		TCMD_UNDER_BOTH | TCMD_WRITE,	//max RF length
	0x043b,		0xfc,		TCMD_UNDER_BOTH | TCMD_WRITE,	//BBRX: enable tick/length output
//	0x0447,		0xf5,		TCMD_UNDER_BOTH | TCMD_WRITE,	//BBRX: tick clock divider: by 16, 4us step
//	0x044c,		0xff,		TCMD_UNDER_BOTH | TCMD_WRITE,	//BBRX: TICK frame ffff, extend to 3-byte
//	0x044d,		0xff,		TCMD_UNDER_BOTH | TCMD_WRITE,	//BBRX:

	0x0074,		0x53,		TCMD_UNDER_BOTH | TCMD_WRITE,	//id enable
	0x007e,		0xca,		TCMD_UNDER_BOTH | TCMD_WRITE,	//id enable
	0x007f,		0x08,		TCMD_UNDER_BOTH | TCMD_WRITE,	//id enable
	0x0074,		0x00,		TCMD_UNDER_BOTH | TCMD_WRITE,	//id enable

	0x0620,		0x01,		TCMD_UNDER_BOTH | TCMD_WRITE	//enable timer 0

};

void SendIdlePkt ()
{
	unsigned char k;
	for (k=0; k<64; k++) {
		while (reg_usb_ep8_fifo_mode & FLD_USB_ENP8_FULL_FLAG);
		reg_usb_ep8_dat = 0;
	}
}

void SendPkt (unsigned char * lpS, unsigned char len)
{
	unsigned char k;
	short n = len;
	if (n>252) return;
	while (n > 0) {
		if (1) {
		//if (len & 0xc0) {
			while (reg_usb_ep8_fifo_mode & FLD_USB_ENP8_FULL_FLAG);
			reg_usb_ep8_dat = n;
		}
		for (k=0; k<63; k++) {
			while (reg_usb_ep8_fifo_mode & FLD_USB_ENP8_FULL_FLAG);
			reg_usb_ep8_dat = *lpS++;
		}
		n -= 63;
	}
}

#define		LL_CHANNEL_SEARCH_FLAG			BIT(16)
u32			km_chn_mask = LL_CHANNEL_SEARCH_FLAG;
u8			km_chn = 0;
int			km_ack_miss = 0;


u32 	tick_idle;
u32 	timer1_error;

u8 *	pbuff[4];
u8		wptr, rptr;

void rf_proc_post (u8 * raw_pkt) {
	u32 static dbg_rx_no;
	dbg_rx_no++;
	u8* p = raw_pkt;
	u32 t = (p[8] + (p[9]<<8) + (p[10]<<16) + (p[11]<<24));
	u8* pe = p + p[0];
	p[10] = pe[0];
	p[11] = pe[1];
	//p[4] = pe[2];
	pe[0] = t >> 6;
	pe[1] = t >> 14;
	pe[2] = t >> 22;

	rf_packet_ack_pairing_t *p_ack = (rf_packet_ack_pairing_t *) (raw_pkt + 8);
	if (	RF_PACKET_CRC_OK(raw_pkt) &&
			(p_ack->type & 0xf8) == FRAME_TYPE_ACK &&
			p_ack->proto == RF_PROTO_BYTE )
	{

		int t = ((p_ack->tick & 0xff) << 10) + CLOCK_SYS_CLOCK_1US * 500;

		km_chn_mask = p_ack->chn;

		km_ack_miss = 0;

		if (t < CLOCK_SYS_CLOCK_1US * 8000) {
			reg_tmr1_tick = t;	//	timer sync
		}
		else {
			timer1_error++;
		}
	}

	p[8] = km_chn;
	p[9] = timer1_error;

	pbuff[wptr] = p;
	wptr = (wptr + 1) & 3;
	//SendPkt (p+4, p[0]);
	tick_idle = clock_time ();
}

u8 * rf_rx_response (u8 * raw_pkt, callback_rx_func *p_post) {
	*p_post = rf_proc_post;
	return NULL;
}

void irq_km_timer1 (void)
{
	//log_event (TR_T_irq_timer1);
	static u32 timer1_no;
	timer1_no++;

	if (km_ack_miss < 64) {
		km_ack_miss++;
	}
	else {
		km_chn_mask = LL_CHANNEL_SEARCH_FLAG;
	}

	if (km_ack_miss < 64 || (timer1_no & 3) == 0 ) {
		km_chn = get_next_channel_with_mask (km_chn_mask, km_chn);
		SetRxMode (km_chn, RF_CHN_TABLE);
	}
}


void user_init() {

	LoadTblCmdSet (tbl_sys_ini, sizeof (tbl_sys_ini)/sizeof (TBLCMDSET));

	ll_rx_init (NULL);

	reg_tmr1_tick = 0;
	reg_tmr1_capt = CLOCK_SYS_CLOCK_1US * 8000;
	reg_tmr_ctrl |= FLD_TMR1_EN;
	reg_irq_mask |= FLD_IRQ_TMR1_EN;

	////////////////////////////////////////////////////
	// RX mode setup: packet sniffer
	////////////////////////////////////////////////////
	rf_multi_receiving_init (0x3f);

	usb_dp_pullup_en (1);

	while (!reg_usb_host_conn);
}

void main_loop () {
	if (rptr != wptr) {
		u8 *p = pbuff[rptr];
		SendPkt (p + 4, p[0]);
		rptr = (rptr + 1) & 3;
		p[0] = 1;
 	}
	else if(clock_time_exceed(tick_idle, 100)) {
		tick_idle = clock_time ();
		SendIdlePkt ();
	}
}

#endif

