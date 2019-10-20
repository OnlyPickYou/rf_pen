
#pragma once
#include "../../proj/config/user_config.h"
#include "mouse.h"

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif


#ifndef MOUSE_CUSTOM_FULL_FUNCTION
#define MOUSE_CUSTOM_FULL_FUNCTION      1
#endif

#ifndef MOUSE_HW_CUS
#define MOUSE_HW_CUS      1
#endif

#ifndef MOUSE_SW_CUS
#define MOUSE_SW_CUS      1
#endif

#ifndef MOUSE_WKUP_SENSOR_SIM
#define MOUSE_WKUP_SENSOR_SIM      1
#endif

#ifndef MOUSE_BTNUI_CUS
#define MOUSE_BTNUI_CUS      1
#endif

#ifndef MOUSE_SENSOR_CPI_CUS
#define MOUSE_SENSOR_CPI_CUS	1
#endif

#if (MCU_CORE_TYPE == MCU_CORE_5330)
#define		DEVICE_ID_ADDRESS		0x3f00
#else
#define		DEVICE_ID_ADDRESS		0x3f00
#endif

#if( MCU_CORE_TYPE == MCU_CORE_8366)

#ifndef M_HW_BTN_UP
#define M_HW_BTN_UP			GPIO_GP7
#endif

#ifndef M_HW_BTN_RF_LED
#define M_HW_BTN_RF_LED		GPIO_GP8
#endif

#ifndef M_HW_BTN_DOWN
#define M_HW_BTN_DOWN		GPIO_GP10
#endif

#ifndef M_HW_BTN_START
#define M_HW_BTN_START		GPIO_GP9
#endif

#ifndef M_HW_BTN_TAB
#define M_HW_BTN_TAB		GPIO_GP3
#endif

#ifndef M_HW_BTN_VOL_DOWN
#define M_HW_BTN_VOL_DOWN	GPIO_GP1
#endif

#ifndef M_HW_BTN_VOL_UP
#define M_HW_BTN_VOL_UP		GPIO_GP3
#endif

#ifndef M_HW_LED2_CTL
#define M_HW_LED2_CTL   	GPIO_SWS
#endif

#ifndef M_HW_LED_CTL
#define M_HW_LED_CTL    	GPIO_GP2    //GPIO_GP2
#endif

#ifndef M_HW_GPIO_LEVEL_UP
#define M_HW_GPIO_LEVEL_UP   	U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_RF_LED
#define M_HW_GPIO_LEVEL_RF_LED   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_DOWN
#define M_HW_GPIO_LEVEL_DOWN   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_START
#define M_HW_GPIO_LEVEL_START   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_TAB
#define M_HW_GPIO_LEVEL_TAB   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_VOL_DOWN
#define M_HW_GPIO_LEVEL_VOL_DOWN   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_VOL_UP
#define M_HW_GPIO_LEVEL_VOL_UP   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_LED
#define M_HW_GPIO_LEVEL_LED   U8_MAX
#endif

#ifndef M_HW_VBAT_CHN
#define M_HW_VBAT_CHN    COMP_GP6
#endif

#if(0)
#ifndef M_HW_BTN_LEFT
#define M_HW_BTN_LEFT   GPIO_GP10	//GP10
#endif

#ifndef M_HW_BTN_RIGHT
#define M_HW_BTN_RIGHT  GPIO_GP8
#endif

#ifndef M_HW_BTN_MIDL
#define M_HW_BTN_MIDL   GPIO_GP7
#endif

#ifndef M_HW_LED_CTL
#define M_HW_LED_CTL    GPIO_GP2    //GPIO_GP2
#endif

#ifndef M_HW_LED2_CTL
#define M_HW_LED2_CTL   GPIO_SWS
#endif

#ifndef M_HW_GPIO_LEVEL_LEFT
#define M_HW_GPIO_LEVEL_LEFT   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_RIGHT
#define M_HW_GPIO_LEVEL_RIGHT   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_MIDL
#define M_HW_GPIO_LEVEL_MIDL   U8_MAX
#endif

#ifndef M_HW_GPIO_LEVEL_LED
#define M_HW_GPIO_LEVEL_LED   U8_MAX
#endif

#ifndef M_HW_VBAT_CHN
#define M_HW_VBAT_CHN    COMP_GP6
#endif

#endif


#endif


enum{
	E_LED_POWER_ON = 0,
    E_LED_PAIRING,
    E_LED_BAT_LOW,
};

typedef struct{
    u8  paring_ui;          //0x20:0xff for auto; else for paring/emi key define
    u8  emi_ui;             //0xff for auto; else for paring/emi key define

} custom_btn_ui_t;


typedef struct{
	u16	vid;		//vendor id
	u16 gid;
    
	u32 did;		//0x04~0x07 device id
	
	u8	cap;		//0x08 crystal CAP setting
    u8  tx_power;   	 //0x09
    u8  tx_power_paring; //0x0a
    u8  tx_power_sync;

    u8  tx_power_emi;    //0x0c
    u8  paring_only_pwon;//0x0d
    u8  slp_mode;		 //0x0e
    u8  slp_no_dongle;   //0x0f

    u16 slp_tick;        //time to enter deep sleep mode, 1 seonds base
    


} custom_cfg_t;

extern custom_cfg_t    *p_custom_cfg;
extern custom_btn_ui_t  rc_btn_ui;
extern led_cfg_t rc_led_cfg[];

#define rc_cust_tx_power         (p_custom_cfg->tx_power)
#if	MOUSE_RF_CUS

#define rc_cust_tx_power_paring  ( (p_custom_cfg->tx_power_paring == 0xff) ? RF_POWER_m16dBm : p_custom_cfg->tx_power_paring )
#define rc_cust_tx_power_sync  ( (p_custom_cfg->tx_power_sync == 0xff) ? rc_status->tx_power : p_custom_cfg->tx_power_sync )
#define rc_cust_tx_power_emi         ( (p_custom_cfg->tx_power_emi == 0xff) ? RF_POWER_8dBm : p_custom_cfg->tx_power_emi )
#else
#define rc_cust_tx_power_paring		RF_POWER_m24dBm
#define rc_cust_tx_power_sync		rc_status->tx_power
#define rc_cust_tx_power_emi			RF_POWER_8dBm
#endif

#define rc_cust_paring_only_pwon (p_custom_cfg->paring_only_pwon)
#define CUST_MCU_SLEEP_EN 	        (p_custom_cfg->slp_mode & 1)
#define QUICK_SLEEP_EN  	        (p_custom_cfg->slp_no_dongle)
#define GET_HOST_ACCESS_CODE_FLASH_OTP  (p_custom_cfg->gid)



void rc_custom_init ( rc_status_t *pStatus );

#if 0
static inline led_cfg_t mouse_led_pairing_end_cfg_cust( u32 pairing_end ){

     rc_led_cfg[E_LED_PAIRING_END].on_time = rc_led_cfg[E_LED_PAIRING_END].off_time;
     return rc_led_cfg[E_LED_PAIRING_END];
}
#endif



