#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"

#include "device_info.h"
#include "mouse_rf.h"
#include "mouse_button.h"
#include "mouse_batt.h"
#include "mouse_rf.h"
#include "mouse_custom.h"

custom_cfg_t   *p_custom_cfg;

/*
 * BUTTON ----- GPIO_10, GPIO_8, GPIO_7,
 * LED ----- GPIO_2
 * SWS ----- When Button GPIO_7 pressed, SWS and GPIO_2 output high
 */


#if (MOUSE_SIM_RF_PEN)
const u32 m_hw_def_dft[] = {
	M_HW_BTN_UP,
	M_HW_BTN_RF_LED,
	M_HW_BTN_DOWN,
    M_HW_LED_CTL,
    M_HW_GPIO_LEVEL_UP | (M_HW_GPIO_LEVEL_RF_LED<<8) | (M_HW_GPIO_LEVEL_DOWN<<16) | (M_HW_GPIO_LEVEL_LED<<24),
    M_HW_VBAT_CHN,
};
#elif(MOUSE_R250_RF_PEN)
const u32 m_hw_def_dft[] = {
	M_HW_BTN_UP,
	M_HW_BTN_RF_LED,
	M_HW_BTN_DOWN,
	//M_HW_BTN_START,
	//M_HW_BTN_TAB,
	M_HW_BTN_VOL_DOWN,
	M_HW_BTN_VOL_UP,
    M_HW_LED_CTL,
    M_HW_GPIO_LEVEL_UP | (M_HW_GPIO_LEVEL_RF_LED<<8) | (M_HW_GPIO_LEVEL_DOWN<<16) |(M_HW_GPIO_LEVEL_START<<24),
    //M_HW_GPIO_LEVEL_TAB | (M_HW_GPIO_LEVEL_VOL_DOWN<<8) | (M_HW_GPIO_LEVEL_VOL_UP<<16) | (M_HW_GPIO_LEVEL_LED<<24),
    M_HW_GPIO_LEVEL_TAB | (M_HW_GPIO_LEVEL_VOL_DOWN<<8),
    M_HW_VBAT_CHN,
};
#elif(MOUSE_R150_RF_PEN)
const u32 m_hw_def_dft[] = {
	M_HW_BTN_UP,
	M_HW_BTN_RF_LED,
	M_HW_BTN_DOWN,
	M_HW_BTN_START,
	M_HW_BTN_TAB,
	//M_HW_BTN_VOL_DOWN,
	//M_HW_BTN_VOL_UP,
    M_HW_LED_CTL,
    M_HW_GPIO_LEVEL_UP | (M_HW_GPIO_LEVEL_RF_LED<<8) | (M_HW_GPIO_LEVEL_DOWN<<16) |(M_HW_GPIO_LEVEL_START<<24),
    //M_HW_GPIO_LEVEL_TAB | (M_HW_GPIO_LEVEL_VOL_DOWN<<8) | (M_HW_GPIO_LEVEL_VOL_UP<<16) | (M_HW_GPIO_LEVEL_LED<<24),
    M_HW_GPIO_LEVEL_TAB | (M_HW_GPIO_LEVEL_LED<<8),
    M_HW_VBAT_CHN,
};
#endif


custom_btn_ui_t rc_btn_ui = {
    //( FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT ),                       //pairing ui
    //( FLAG_BUTTON_LEFT | FLAG_BUTTON_RIGHT | FLAG_BUTTON_MIDDLE ),  //emi ui
	( FLAG_BUTTON_UP | FLAG_BUTTON_DOWN ),
	( FLAG_BUTTON_UP | FLAG_BUTTON_RF_LED | FLAG_BUTTON_DOWN ),
};


led_cfg_t rc_led_cfg[] = {
    4,      4,      2,      0x40,    //power-on, 1s on, 64ms * 4 = 256ms
    4,      4,      2,      0x40,    //pairing manual, 2Hz, 2 times
    3,      3,      3,      0,       //battery low  2Hz
};


#if 1//MOUSE_CUSTOM_FULL_FUNCTION    
void mouse_custom_re_get( u8 *p_dst, u8 *p_src_0, u8 *p_src_1, u32 len ){
    int i;
    for( i = 0; i < len; i++ ){
        *p_dst = (*p_src_1 == U8_MAX) ? *p_src_0 : *p_src_1;
        *p_src_0++;
        *p_src_1++;
        *p_dst++;
    }
}

void mouse_custom_re_get_4( u32 *p_dst, u32 *p_src_0, u32 *p_src_1, u32 len ){
    int i;
    for( i = 0; i < len; i++ ){
        *p_dst = (*p_src_1 == U32_MAX) ? *p_src_0 : *p_src_1;
        *p_src_0++;
        *p_src_1++;
        *p_dst++;
    }
}
#else
static inline void mouse_custom_re_get( u8 *p_dst, u8 *p_src_0, u8 *p_src_1, u32 len ) {}
static inline void mouse_custom_re_get_4( u32 *p_dst, u32 *p_src_0, u32 *p_src_1, u32 len ) {}
#endif

//cfg_init	to£ºval_c   =        0    |   1    |   2    |      3
//--------------------------------------------------------------
//	0		cust_addr_0 =        8    |   a    |   4    |      6
//	1		cust_addr_1 =        7    |   9    |   3    |      5
//	2		cust_addr_2 =        6    |   8    |   2    |      4
//	3		cust_addr_3 =        5    |   7    |   1    |      3
//--------------------------------------------------------------
u8 custom_cfg_re_define( u8 cfg, u8* p_cfg_re_def ){
	u32 val_c = ( *(p_cfg_re_def + cfg) == U8_MAX ) ? cfg : \
		( ( ( cfg + *(p_cfg_re_def+cfg) ) >> 1 ) & 3 );
	return val_c;
}

extern rf_packet_pairing_t	pkt_pairing;

void rc_custom_init ( rc_status_t *pStatus ){

	for(int i = 0; i<3; i++){
		p_custom_cfg = (custom_cfg_t *)(DEVICE_ID_ADDRESS - i*0x100);
		if(p_custom_cfg->cap != 0){
			break;
		}
	}

    if ( p_custom_cfg->cap != U8_MAX )
        cap_internal_adjust( p_custom_cfg->cap );

	mouse_custom_re_get_4( pStatus->hw_define, m_hw_def_dft, m_hw_def_dft, sizeof(rc_hw_t)>>2 );

    pkt_pairing.did = (p_custom_cfg->did == U32_MAX) ? pkt_pairing.did : p_custom_cfg->did;   //device-id init

    if(pkt_pairing.did > 2000){
    	write_reg8(0x8005, 0x22);
    	while(1);
    }

	u16 vendor_id = p_custom_cfg->vid;
    if(vendor_id != U16_MAX){
		rf_set_access_code0 (rf_access_code_16to32(vendor_id));
	}

    pStatus->high_end = U8_MAX;

}

