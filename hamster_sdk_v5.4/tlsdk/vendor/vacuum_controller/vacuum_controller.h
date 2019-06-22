#pragma once
#include "../common/mouse_type.h"
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#include "vacuum_controller_default_config.h"
/////////////////// set default   ////////////////

#include "../common/default_config.h"

/*
 *			tx_repeat_mode
 *	(1)without key_scan and sleep;
 *	(2)power key is sent with one second interval,almost 10000 times;
 *
 */

#ifndef		TEST_TX_REPEAT_MODE
#define		TEST_TX_REPEAT_MODE		0
#endif

#if 		TEST_TX_REPEAT_MODE
#define		REPEAT_TIMES			10000			//10000 times
#define		MAIN_LOOP_TIMES			300				//3s
#endif


/*
 * 			tx_pair_mode
 * 	(1)without power on/off function;
 * 	(2)when power key is pressed, auto_pair packet will be sent;
 *
 */
#ifndef		MANUAL_PAIR_EN
#define		MANUAL_PAIR_EN				0
#endif

#ifndef		TEST_TX_PAIR_MODE
#define		TEST_TX_PAIR_MODE			0
#endif

#ifndef		FW_WITH_EMI_FUNC
#define		FW_WITH_EMI_FUNC			1
#endif

#ifndef 	CONTROLLER_ID_IN_FW
#define 	CONTROLLER_ID_IN_FW 		1
#endif

#ifndef		FACTORY_PRODUCT_TEST_EN
#define 	FACTORY_PRODUCT_TEST_EN		0
#endif

#define		STORE_2BYTE_ID				1

#define		MAX_RETURN_BTN				6
#define 	MAX_BATT_DETECT				3

#define		MAINLOOP_8MS_DURING_CNT		58
#define 	MANNUAL_PARING_CNT			300			//连续按键300个main_loop,300 * 10ms = 5s进manual paring

#define 	SLEEP_MODE_BASIC_SUSPEND 	0
#define 	SLEEP_MODE_WAIT_DEEP    	2
#define 	SLEEP_MODE_DEEPSLEEP     	3

#define		PARING_START_ADDR			0x3000		//配对起始存放地址
#define		PARING_END_ADDR				0x3f00
#define		CUSTOM_CONTROLLER_ADDR		0x2ffc		//定制golden controller的地址
#define		MAX_PARING_TIMES			((PARING_END_ADDR - PARING_START_ADDR)/2)	 		//最大配对次数

//support repeat key function
#define		KEY_NONE		0
#define		KEY_CHANGE		1
#define		KEY_SAME		2

#ifndef		WITH_REPEAT_FUNC
#define		WITH_REPEAT_FUNC			1
#endif

#define		REPEAT_TIME_THRESH			25				//(10 * 10ms = 100ms)
#define		POWER_KEY_PRESS_CNT			20




/* Batt value = KeyStatus | BattValue */
#ifndef		WITH_NEW_DETECT_BATT_FUNC
#define		WITH_NEW_DETECT_BATT_FUNC	1
#endif
//BattValue
#define		BATT_GE_2P3V_AREA				0				//>2.3V
#define		BATT_GE_2P1V_AREA				0x01			//2.1~2.3
#define		BATT_GE_2P0V_AREA				0x02			//2.0~2.1
#define		BATT_LESS_2P0V_AREA				0x03			//<2.1v

//KeyStatus
#define 	KEY_REPEAT					0
#define		KEY_PRESSED					0x10
#define		KEY_RELEASED				0x20

/*
 * #define 	KEY_REPEAT					0x10
 * #define	KEY_PRESSED					0x0
 */

/*
 *"power" + "up" send 0x70,	"up" + "down" send 0x60
 */
#ifndef		WITH_KEYCODE_SEND
#define		WITH_KEYCODE_SEND			1
#endif

#define		COMBINE_KEY1_FUNC			0x60
#define		COMBINE_KEY2_FUNC			0x70

//when release key, send the last keycode
#ifndef		WITH_RELEASE_KEY_FUNC
#define		WITH_RELEASE_KEY_FUNC		1
#endif

#ifndef 	WITH_DOUBLE_KEY_FUNC
#define		WITH_DOUBLE_KEY_FUNC		1
#endif

#ifndef		WITH_SPELL_PACKET_EN
#define		WITH_SPELL_PACKET_EN		1
#endif

#ifndef		WITH_POWER_KEY_FOR_MANNUAL_PAIR
#define		WITH_POWER_KEY_FOR_MANNUAL_PAIR		1
#endif

#if	(FACTORY_PRODUCT_TEST_EN)
#define 	GPIO_LED					GPIO_GP0
#define 	TEST_POWERON				0x40
#define 	TEST_POWERDOWN				0x50

#endif

//0x3f00
typedef struct{
	u16	pipe_pairing;			// pipe 0,pairig
	u16 pipe_kb_data;			// pipe 2,kb data
	u32 did;					// device id

	u8  tx_power_paring;		//08
    u8  tx_power;
	u8  tx_power_emi;
	u8 	memory_type;			//0b

	u8  k_delay;

}vc_cfg_t;
extern 	vc_cfg_t *p_vc_cfg;

typedef	struct{
	u8 	cnt;								//count button num
	u8 	btn_ctrl;
	u8 	keycode[MAX_RETURN_BTN];			//6 btn
}vc_data_t;
extern 	vc_data_t vc_event;

#if(WITH_SPELL_PACKET_EN)
extern 	vc_data_t vc_s_event;				//used for spell packet
#endif



typedef struct{

	u8 	batt_status;
	u8 	batt_value[MAX_BATT_DETECT];

}batt_info_t;

extern batt_info_t batt_info;

typedef struct {
	u8  vc_mode;
	u8  mode_link;
	u8  rf_sending;
	u8  kb_pipe_rssi;

	u8  tx_power;
	u8  cust_tx_power;
	u8  tx_retry;
    u8  golden_tx_en;

    u16 no_ack;
    u8 pre_host_status;
    u8 flag;

	u32 dongle_id;
    u32 loop_cnt;
} vc_status_t;
extern vc_status_t  vc_status;

typedef struct{
    u8   mode;
    u8   device_busy;
    u8   quick_sleep;
    u8   rsvd;

    u16   cnt_basic_suspend;
    u16   thresh_basic_suspend;

    u16   wakeup_src;
    u16   wakeup_ms;

    u32  wakeup_tick;
} vc_slp_cfg_t;

extern vc_slp_cfg_t vc_sleep;

typedef	struct{

	u8 	 key_change_flg;
	u8 	 key_repeat_flg;
	u16  key_repeat_cnt;

}key_repeat_t;

extern key_repeat_t key_repeat;


typedef enum{
	STATE_POWERON = 0,
	STATE_SYNCING,
	STATE_PAIRING ,
	STATE_NORMAL,
	STATE_EMI,
	STATE_WAIT_DEEP,
	STATE_TEST,
}KB_MODE;


typedef enum{
	STATE_HIGH_BATT = 1,
	STATE_LOW_BATT = 2,
}BATT_MODE;


#if (FACTORY_PRODUCT_TEST_EN)

typedef struct{

	u8 keystatus_cnt;			// 0: stop sending keycode, 1: sending keycode
	u8 isOn;
	u8 polar;
	u8 repeatCount;				// led on/off count

	u16 OnTimes_ms;
	u16 OffTimes_ms;

	u32 StartCnt;

} test_led_t;

extern test_led_t test_led;
#endif

#define FOR_VACCUM_DEMO		0
#if (!FOR_VACCUM_DEMO)
#define vc_cust_tx_power        ((p_vc_cfg->tx_power 	   == 0xff) ? RF_POWER_8dBm   : p_vc_cfg->tx_power)
#else
#define vc_cust_tx_power        ((p_vc_cfg->tx_power 	   == 0xff) ? RF_POWER_m20dBm   : p_vc_cfg->tx_power)
#endif
#define vc_cust_tx_power_emi	((p_vc_cfg->tx_power_emi	   == 0xff) ? RF_POWER_8dBm   : p_vc_cfg->tx_power_emi)
#if 0
#define vc_cust_tx_power_paring ((p_vc_cfg->tx_power_paring == 0xff) ? RF_POWER_m20dBm : p_vc_cfg->tx_power_paring )
#else
#define vc_cust_tx_power_paring ((p_vc_cfg->tx_power_paring == 0xff) ? RF_POWER_m8dBm : p_vc_cfg->tx_power_paring )
#endif

#define	shutdown_internal_cap1	analog_write(0x80,0x21);		//shut down the on-chip capacitor
#define shutdown_internal_cap2	analog_write(0x81,0x40);


void vc_config_init(void);
int set_device_id_in_firmware (u32 id);

void otp_device_program(int , unsigned char);


//flash
static inline void flash_device_program_on (void){}
static inline void flash_device_program_off(void){}
static inline int  flash_power_on_check(void){return 1;}
void flash_device_program(int , unsigned char);
//flash program to emulate OTP






/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

