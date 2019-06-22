/*
 * kb_custom.c
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

kb_custom_cfg_t *p_kb_custom_cfg;


void kb_custom_init(void)
{
	//custom info fix(when burning OTP err)
	for(int i=0;i<3;i++){
		p_kb_custom_cfg = (kb_custom_cfg_t *) (CUSTOM_INFO_ADDRESS+(i*0x30));
		if(p_kb_custom_cfg->cap != 0){
			break;
		}
	}

	//set pipe
    if(p_kb_custom_cfg->pipe_pairing != U16_MAX){
		rf_set_access_code0 (rf_access_code_16to32(p_kb_custom_cfg->pipe_pairing));
	}

    //load device id
	if (p_kb_custom_cfg->did != U32_MAX) {
		pkt_pairing.did = p_kb_custom_cfg->did;
	}
	pkt_km.did = pkt_pairing.did;
	pkt_audio.did = pkt_pairing.did;


	analog_write(0x81,p_kb_custom_cfg->cap == U8_MAX ? 0xd8 : p_kb_custom_cfg->cap);
}



#endif  //end of __PROJECT_TX_8267__
