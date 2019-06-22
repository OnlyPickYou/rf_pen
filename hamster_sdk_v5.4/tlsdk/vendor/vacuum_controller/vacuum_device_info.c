#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "vacuum_device_info.h"
#include "vacuum_controller_button.h"
#include "vacuum_controller.h"
#include "vacuum_controller_rf.h"

extern 	repair_and_reset_t repair_and_reset;
void device_info_load(void)
{

	//vacuum_deepsleep_cnt = analog_read(PM_REG_START+5);
	vc_status.vc_mode = STATE_NORMAL;

	u8 * pd = (u8 *) (&vc_status.dongle_id);
	for (u8 i = PM_REG_START; i <= PM_REG_START+3; i++) {
		*pd  = analog_read (i);
		pd++;
	}
}

#if(WITH_SPELL_PACKET_EN)
extern rf_packet_vacuum_t	pkt_km;
#else
extern rf_packet_keyboard_t	pkt_km;
#endif
void device_info_save(void)
{
    u8 * pd;

    vc_status.dongle_id = rf_get_access_code1();
	pd = (u8 *) &vc_status.dongle_id;
	u32 *ptr = (u32 *)(&vc_status.dongle_id);

	for (u8 i = PM_REG_START; i <= PM_REG_START+3; i++) {
		analog_write (i, *pd ++);

	}

	analog_write(PM_REG_START+4,(vc_status.mode_link<<7) | repair_and_reset.repair_cnt);
	//analog_write(PM_REG_START+5,vacuum_deepsleep_cnt);
	analog_write(PM_REG_START+5, pkt_km.seq_no);
}

