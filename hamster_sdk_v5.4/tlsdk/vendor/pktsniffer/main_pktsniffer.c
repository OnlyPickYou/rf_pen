
#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../link_layer/rf_ll.h"

#if(__PROJECT_PKT_SNIFFER__)

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

void timer_irq1_handler (void) {
}

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

//unsigned char buff_pkt[256];
int	tpkt;
unsigned char sreg, swm, sws, dev, devc;
#define		SWIRE_S		0x5d
#define		SWIRE_M		0x5b

unsigned int	chnscan, sc_t, sc_chn, sc_id;

u32 tick_scan;
u32 tick_idle;
signed char chn;

u8 *	pbuff[4];
u8		wptr, rptr;

void rf_proc_post (u8 * raw_pkt) {
	u8* p = raw_pkt;
	u32 t = (p[8] + (p[9]<<8) + (p[10]<<16) + (p[11]<<24));
	u8* pe = p + p[0];
	p[10] = pe[0];
	p[11] = pe[1];
	//p[4] = pe[2];
	pe[0] = t >> 6;
	pe[1] = t >> 14;
	pe[2] = t >> 22;
	if (chnscan) {
		p[8] = sc_chn;
	}

	pbuff[wptr] = p;
	wptr = (wptr + 1) & 3;
	//SendPkt (p+4, p[0]);
	tick_idle = clock_time ();
}

u8 * rf_rx_response (u8 * raw_pkt, callback_rx_func *p_post) {
	u32 static dbg_rx_no;
	dbg_rx_no++;
	*p_post = rf_proc_post;
	return NULL;
}

short rcmd = 0;
short rcmdn;

void user_init() {

	//for PHY test
	if( *(u8 *)0x7f000 != 0xff ){
		write_reg8(0x401, 0);					//disable PN
		write_reg32 (0x800408, 0x29417671);	//accesscode: 1001-0100 1000-0010 0110-1110 1000-1110   29 41 76 71
	}



	chnscan = 0;
	sc_id = 0;
	reg32_dbg_scan = chnscan;

	ll_rx_init ();

	rf_multi_receiving_init (0x3f);

	////////////////////////////////////////////////////
	// RX mode setup: packet sniffer
	////////////////////////////////////////////////////
	dev = reg8_dbg_dev;
	chn = 0x5c;

	reg8_dbg_chn = chn;

	LoadTblCmdSet (tbl_sys_ini, sizeof (tbl_sys_ini)/sizeof (TBLCMDSET));

	usb_dp_pullup_en (1);

	while (!reg_usb_host_conn);
}

void main_loop () {

		rcmdn = reg16_dbg_cmd;
		devc = reg8_dbg_dev;
		dev = devc;

		chnscan = reg32_dbg_scan;
		if (chnscan == 0)
		{
			sc_id = 0;
		}
		else if (clock_time_exceed(tick_scan, 1000 * (chnscan & 0x7f)))
		{
			tick_scan = clock_time();
			sc_chn = ((chnscan>>8)&0xff) + sc_id;
			SetRxMode (sc_chn, 0);
			sc_id++;
			if (sc_id >= (chnscan>>16))
				sc_id = 0;
		}


		if (rcmd!=rcmdn) {		// RX/TX mode
			rcmd = rcmdn;
			////////////////////////////////////////////////////////
			//	USB capture
			////////////////////////////////////////////////////////
			{
				chn = rcmd;
				u8 rtx = rcmd >> 8;
				if (rtx & 0x7f) {
					if (rtx & 0x80) {
						SetRxMode (chn, RF_CHN_TABLE);
					}
					else {
						SetRxMode (chn, 0);
					}
				}
				else {
					if (rtx & 0x80) {
						SetTxMode (chn, RF_CHN_TABLE);
					}
					else {
						SetTxMode (chn, 0);
					}
				}
			}
		}
		if (1 && rptr != wptr) {
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

