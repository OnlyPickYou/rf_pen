/*
 * kb_info.c
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#include "../../proj/tl_common.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/rf_drv.h"
#include "../common/rf_frame.h"

#include "tx_emi.h"
#include "tx_rf.h"
#include "tx_custom.h"
#include "tx_info.h"
#include "tx_batt.h"
#include "tx_pm.h"
#include "tx_led.h"
#include "tx_test.h"


#if (__PROJECT_TX_8267__)


#define		PM_REG_MODE_LINK			DEEP_ANA_REG4
#define		PM_REG_COUNT		    	DEEP_ANA_REG5

#define		PM_REG_DONGLE_ID_START		DEEP_ANA_REG0
#define		PM_REG_DONGLE_ID_END		DEEP_ANA_REG3

//u8 deep_count;

void kb_info_load(void)
{
#if 1
	kb_status.isConnect = analog_read(PM_REG_MODE_LINK);
	//deep_count = analog_read(PM_REG_COUNT);

	u8 * pd = (u8 *) (&kb_status.dongle_id);
	for (u8 i = PM_REG_DONGLE_ID_START; i <= PM_REG_DONGLE_ID_END; i++) {
		*pd  = analog_read (i);
		pd++;
	}

	if(kb_status.isConnect && kb_status.dongle_id!=U32_MAX){
		kb_status.kb_mode = STATE_CONNECT;
	}
	else{
		kb_status.kb_mode = STATE_PAIRING;
		kb_status.isConnect = 0;
		kb_status.dongle_id = 0;
	}
#endif
}

void kb_info_save(void)
{
#if 1
	u8 * pd;
	kb_status.dongle_id = rf_get_access_code1();
	pd = (u8 *) &kb_status.dongle_id;
	for (u8 i = PM_REG_DONGLE_ID_START; i <= PM_REG_DONGLE_ID_END; i++) {
		analog_write (i, *pd ++);
	}

	analog_write(PM_REG_MODE_LINK, kb_status.isConnect);
	//analog_write(PM_REG_COUNT,deep_count+1);
#endif
}


#endif  //end of __PROJECT_TX_8267__
