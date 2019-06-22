#pragma once

#include "../../proj/common/types.h"
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif


#define _USER_CONFIG_DEFINED_	1	// must define this macro to make others known 

#ifndef	DONGLE_DBG_EN
#define DONGLE_DBG_EN		1
#endif

#define USB_DESCRIPTER_CONFIGURATION_FOR_KM_DONGLE    0
#define USB_ID_AND_STRING_CUSTOM                   	  0


#define	USB_PRINTER				0
//////////// product  Infomation  //////////////////////////////
#define ID_VENDOR				0x248a			// for report
// If ID_PRODUCT left undefined, it will default to be combination of ID_PRODUCT_BASE and the USB config USB_SPEAKER_ENABLE/USB_MIC_ENABLE...
#define ID_PRODUCT_BASE			0x8266
#define	ID_PRODUCT				(0x8566 + USB_PRINTER - !USB_KEYBOARD_ENABLE * 2)//dongle PID = 0x8564 support mouse only
																				//dongle PID = 0x8566 support mouse and keyboard.
#define STRING_VENDOR			L"Telink"
#define STRING_PRODUCT			L"Wireless Receiver"
#define STRING_SERIAL			L"TLSR8566"

#ifndef USE_CURRENT_VERSION_1P6
#define USE_CURRENT_VERSION_1P6			0
#endif


#if(USE_CURRENT_VERSION_1P6)
#define	KEYBOARD_PIPE1_DATA_WITH_DID	1
#endif

#if		__PROJECT_DONGLE_8366__
#define CHIP_TYPE				CHIP_TYPE_8366
#else
#define CHIP_TYPE				CHIP_TYPE_8266
#define USB_REMOTE_WAKEUP_FEATURE_ENABLE	0
#endif

#define APPLICATION_DONGLE		1			// or else APPLICATION_DEVICE
#define DONLGE_MONITOR_MODE		0			// by pass mode,  no sending pkts

#define	FLOW_NO_OS				1
#define	DID_AUTO_GEN			0

#ifndef OTP_PROGRAM
#define	OTP_PROGRAM				0
#endif

#define SRAM_SLEEP_CODE                 0

#if DONGLE_CAVY_EN
#define	PARING_SOFTWARE_MODE_ENABLE		0
#define	PARING_MANUAL_MODE_ENABLE		0
#define	PARING_AUTO_MODE_ENABLE			1
#define USB_KEYBOARD_ENABLE             0
#else
#define	PARING_SOFTWARE_MODE_ENABLE		1
#define	PARING_MANUAL_MODE_ENABLE		1
#define	PARING_AUTO_MODE_ENABLE			0
#define USB_KEYBOARD_ENABLE             1
#endif

#define	DCDC_SUPPLY_VOLTAGE				ll_ldo_set_with_bat_gt_3p3

#define DONGLE_TEST_MODE_CD_ENABLE 		0
//////////// debug  /////////////////////////////////
#define __MOUSE_SIMU__  	0
#define __KEYBOARD_SIMU__  	0

/////////////////// MODULE /////////////////////////////////

#define MODULE_PM_ENABLE		0
#define MODULE_ETH_ENABLE		0
#define	MODULE_AUDIO_ENABLE		0

///////////////////  Hardware  //////////////////////////////
#define CHIP_EOP_ERROR			0
/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_TYPE  		CLOCK_TYPE_PLL	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
//#define CLOCK_SYS_TYPE  		CLOCK_TYPE_OSC	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	24000000

/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE   0

#define	SPECIAL_EMI_MODE_EN		 0

///////////////////  interrupt  //////////////////////////////

///////////////////  GPIO  /////////////////////////////////
//  only need to define those are not default
//  all gpios are default to be output disabled, input disabled, output to 0, output strength to 1


/// Antena
///  PA  gpio
#define GPIO_RF_PA_TXEN 		GPIO_SWM
#define GPIO_RF_PA_RXEN 		GPIO_SWS

// ir
#define	GPIO_IR			        0
#define GPIO21_OUTPUT_ENABLE    0

//////////////////    RF configuration //////////////
#define RF_PROTOCOL				RF_PROTO_PROPRIETARY		//  RF_PROTO_PROPRIETARY / RF_PROTO_RF4CE / RF_PROTO_ZIGBEE

///////////////////  ADC  /////////////////////////////////


///////////////////  Keyboard //////////////////////////////
#define KB_PWR_TIME_INTERVAL		(2000*1000)


///////////////////  Audio  /////////////////////////////////
#define MIC_RESOLUTION_BIT		16
#define MIC_SAMPLE_RATE			16000
#define MIC_CHANNLE_COUNT		1
#define	MIC_ENOCDER_ENABLE		0

///////////////////  POWER MANAGEMENT  //////////////////
#define	SUSPEND_SET_RX_4MS		0
#define PM_ACTIVE_SUSPEND_WAKEUP_TIME  	300		// in ms
#define PM_USB_WAKEUP_TIME  			15 		// in ms
#define PM_ENTER_DEEPSLEEP_TIME			600		// in MS

#define PM_SUSPEND_WAKEUP_GPIO_PIN  	0
#define PM_SUSPEND_WAKEUP_GPIO_LEVEL  	1
#define PM_SUSPEND_WAKEUP_FUNC_PIN 		0
#define PM_SUSPEND_WAKEUP_FUNC_LEVEL 	1
/*
the should be the combination of the followings:
DEEPSLEEP_WAKEUP_PIN_GPIO0 to DEEPSLEEP_WAKEUP_PIN_GPIO3
DEEPSLEEP_WAKEUP_PIN_ANA01 to DEEPSLEEP_WAKEUP_PIN_ANA12
*/
#define PM_DEEPSLEEP_WAKEUP_PIN 		0
#define PM_DEEPSLEEP_WAKEUP_LEVEL 		0

///////////////////  USB   /////////////////////////////////
#if(APPLICATION_DONGLE)

#define     DEVICE_CUSTOMIZATION_INFO_ADDR  0x3f00     //定制化信息地址
#define		RF_CRYSTAL_12M_EN	1
#define		cust_crystal_12M	( (*(unsigned char*) (DEVICE_CUSTOMIZATION_INFO_ADDR - 1)) != 0xff )    //( custom_cfg_t->crystal_12M )

#define         TP_GAIN_NOT_FIXED   1
#define         CUST_TP_GAIN_INFO_ADDR   (DEVICE_CUSTOMIZATION_INFO_ADDR - 8)
#define         CUST_TP_GAIN0      ( ((*(unsigned char*) (CUST_TP_GAIN_INFO_ADDR + 2)) != 0xff) ? (*(unsigned char*) (CUST_TP_GAIN_INFO_ADDR + 2)) : (*(unsigned char*) (CUST_TP_GAIN_INFO_ADDR + 0)) ) 
#define         CUST_TP_GAIN1      ( ((*(unsigned char*) (CUST_TP_GAIN_INFO_ADDR + 3)) != 0xff) ? (*(unsigned char*) (CUST_TP_GAIN_INFO_ADDR + 3)) : (*(unsigned char*) (CUST_TP_GAIN_INFO_ADDR + 1)) ) 


#define	USB_PRINTER_ENABLE 		USB_PRINTER	//
#define	USB_SPEAKER_ENABLE 		0
#define	USB_MIC_ENABLE 			0
#define	USB_MOUSE_ENABLE 		1

#define	USB_SOMATIC_ENABLE      0   //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
#define USB_CUSTOM_HID_REPORT	1
#endif


#ifndef	WITH_REPEAT_AND_RELEASE_FUNC
#define	WITH_REPEAT_AND_RELEASE_FUNC	1
#endif

#ifndef	WITH_SPELL_PACKET_EN
#define	WITH_SPELL_PACKET_EN			1
#endif


#define	RCV_RELEASE_THRESH				50
////////////////  ethernet /////////////////	
#define ETH_PHY_RST_GPIO			GPIO_GP0
#define GPIO0_OUTPUT_ENABLE			1

///////////////////  RF4CE   /////////////////////////////////
#define FREAKZ_ENABLE		    	0
#define TL_RF4CE					1

#ifndef DBG_RX_RAM
#define DBG_RX_RAM		0
#endif

#define DBG_TRACE				0
#define EVB_DBG_BOARD			0

#if(DBG_TRACE)
#define	__LOG_RT_ENABLE__		1
#define LOG_IN_RAM				1
#endif

//GPIO_GP7
//GPIO_GP8
#if EVB_DBG_BOARD

#define  DBG_UART_RX_GPIO     GPIO_GP9

#define  GPIO6_INPUT_ENABLE		0
#define  GPIO6_OUTPUT_ENABLE	1
#define  GPIO6_DATA_OUT			0

#define  GPIO1_INPUT_ENABLE		0
#define  GPIO1_OUTPUT_ENABLE	1
#define  GPIO1_DATA_OUT			0

#define  GPIO3_INPUT_ENABLE		0
#define  GPIO3_OUTPUT_ENABLE	1
#define  GPIO3_DATA_OUT			0


#define DBG1_GPIO_HIGH			( *(volatile u8 *)0x800585 |= 0x40 )		//GPIO_GP6
#define DBG1_GPIO_LOW			( *(volatile u8 *)0x800585 &= 0xbf )
#define DBG1_GPIO_TOGGLE		( *(volatile u8 *)0x800585 ^= 0x40 )
#define DBG2_GPIO_HIGH			( *(volatile u8 *)0x800585 |= 0x02 )		//GPIO_GP1
#define DBG2_GPIO_LOW			( *(volatile u8 *)0x800585 &= 0xfd )
#define DBG2_GPIO_TOGGLE		( *(volatile u8 *)0x800585 ^= 0x02 )
#define DBG3_GPIO_HIGH			( *(volatile u8 *)0x800585 |= 0x08 )		//GPIO_GP3
#define DBG3_GPIO_LOW			( *(volatile u8 *)0x800585 &= 0xf7 )
#define DBG3_GPIO_TOGGLE		( *(volatile u8 *)0x800585 ^= 0x08 )

#else  //oshung board
#define DBG1_GPIO_HIGH
#define DBG1_GPIO_LOW
#define DBG1_GPIO_TOGGLE
#define DBG2_GPIO_HIGH
#define DBG2_GPIO_LOW
#define DBG2_GPIO_TOGGLE
#define DBG3_GPIO_HIGH
#define DBG3_GPIO_LOW
#define DBG3_GPIO_TOGGLE

#define DBG_UART_RX_GPIO     GPIO_DP
#endif

/////////////////// set default   ////////////////

#include "../common/default_config.h"

/////////////////// main loop, event loop  ////////////////
enum{
	EV_FIRED_EVENT_MAX = 8
};

enum{

	SUSPEND_WAKEUP_TIME_MS	= 180,
	RCV_MAIN_LOOP_TIME_MS	= 10,

};

enum{

	BASIC_WORKING_MODE = 0,
	BASIC_SUSPEND_MODE = 1,
};


typedef enum{
	EV_SUSPEND_NOTIFY,
	EV_WAKEUP_NOTIFY,
	EV_KEY_PRESS,
#if(MOUSE_USE_RAW_DATA)
	EV_MOUSE_RAW_DATA,
#endif
#if(USB_SOMATIC_ENABLE)
	EV_USB_OUT_DATA,
#endif

	EV_RF_PKT_RECV,
	EV_EVENT_MAX,
}ev_event_e;

typedef enum{
	EV_POLL_RF_RECV,
	EV_POLL_USB_IRQ,
	EV_POLL_DEVICE_PKT,
	EV_POLL_AUDIO_DEC,
	EV_POLL_MOUSE_REPORT,
	EV_POLL_MOUSE_RELEASE_CHECK,
	EV_POLL_KEYBOARD_RELEASE_CHECK,
	EV_POLL_RF_CHN_HOPPING,
#if(MODULE_ETH_ENABLE)
	EV_POLL_ETH_RECV,
#endif	
	EV_POLL_IDLE, //  Must be the last item in ev_poll_e
	EV_POLL_MAX,
}ev_poll_e;


#define MAX_BUFF_SIZE		16			//three btn data

typedef struct{
	u8 	wptr;						//buff write ptr
	u8 	rptr;						//buff read ptr
	u16 rsv;						//resive

	u8 	data[MAX_BUFF_SIZE];		//buff size
	u8 	batt[MAX_BUFF_SIZE];		//low battery information
}data_buff_t;


typedef struct{
	u8 	mode;		//0:work, 1:suspend;
	u8 	wakeup_src;
	u16 rsvd;

	u32 wakeup_ms;

	u32 wakeup_tick;

	u16 suspend_cnt;
	u16 thresh_suspend;

}rcv_sleep_t;

typedef struct{
	u16 not_release_flg;
	u16 release_cnt;

	u16 send_p_flg;

}rcv_release_t;

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

