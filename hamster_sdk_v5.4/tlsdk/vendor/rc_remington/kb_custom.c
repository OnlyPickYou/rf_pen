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

#include "kb_custom.h"
#include "kb_rf.h"

kb_custom_cfg_t *p_kb_custom_cfg;



void kb_custom_init(void)
{
	p_kb_custom_cfg = (kb_custom_cfg_t *)DEVICE_ID_ADDRESS;

    if(p_kb_custom_cfg->pipe_pairing != U16_MAX){
		rf_set_access_code0 (rf_access_code_16to32(p_kb_custom_cfg->pipe_pairing));
	}

	if (p_kb_custom_cfg->did != U32_MAX) {
		pkt_pairing.did = p_kb_custom_cfg->did;
	}

	pkt_km.did = pkt_pairing.did;

	analog_write(0x81,p_kb_custom_cfg->cap == U8_MAX ? 0xd8 : p_kb_custom_cfg->cap);

	kb_status.cust_tx_power = (p_kb_custom_cfg->tx_power == 0xff) ? RF_POWER_8dBm : p_kb_custom_cfg->tx_power;
	kb_status.tx_power = RF_POWER_2dBm;
	extern u8 *	kb_p_map[4];
	if( *((volatile u8 *)0x3d00) != U8_MAX){
		kb_p_map[0] = kb_p_map[1] = kb_p_map[2] = kb_p_map[3] = 0x3d00;}

	u16 *p_cust_map_addr = 0x3f20;
    for( int j = 0; j < sizeof(kb_p_map)/sizeof(int); j++ ){
        for( int i = 0; i < 4; i++ ){
            u16 map_cust = *( p_cust_map_addr + 4 * j + i );
            if( map_cust != U16_MAX ){
            	kb_p_map[j] = map_cust;
            }
        }
    }
}

