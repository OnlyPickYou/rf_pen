
#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/blt_ll/blt_ll.h"

////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32		tick_loop;
void main_loop ()
{
	static	u32	dbg_conn, dbg_brx;

	tick_loop = clock_time ();

	////////////////////////////////////// BLE entry ////////////////////////////
	if (blt_state == BLT_LINK_STATE_ADV)
	{
		u8 *p_conn = blt_send_adv (BLT_ENABLE_ADV_ALL);
		if (p_conn)						//go to connection state
		{
			dbg_conn++;
		}
	}
	else		// connection state
	{
		blt_brx ();
		dbg_brx++;
	}

	////////////////////////////////////// UI entry /////////////////////////////////


	////////////////////////////////////// Suspend entry /////////////////////////////
#if 0
	while ((u32)(blt_next_event_tick - clock_time ()) < BIT(30));
#else
	blt_sleep_wakeup (0, PM_WAKEUP_TIMER, blt_next_event_tick);
	//blt_sleep_wakeup (0, 0, blt_next_event_tick);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//	Initialization: MAC address, Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////
u8  tbl_mac [] = {0xef, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5};

u8	tbl_adv [] =
		{0x0, 0x13,
		 0xef, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,		//mac address
		 0x02, 0x01, 0x05, 0x03, 0x19, 0xC1, 0x03, 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,
		 0, 0, 0			//reserve 3 bytes for CRC
		};


u8	tbl_rsp [] =
		{0x0, 0x15,									//type len
		 0xef, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,		//mac address
		 0x0e, 0x09, 'T', 'e', 'l', 'i', 'n', 'k', ' ', 'b', 'L', 'i', 'g' ,'h', 't',
		 0, 0, 0								//reserve 3 bytes for CRC
		};

void user_init()
{
	extern void rf_drv_1m();
	rf_drv_1m ();
	analog_write (0x81, 0x56);		//adjust frequency offset

	blt_init (tbl_mac, tbl_adv, tbl_rsp);

	extern void my_att_init ();
	my_att_init ();

}


