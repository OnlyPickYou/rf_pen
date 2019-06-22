#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#define _USER_CONFIG_DEFINED_	1	// must define this macro to make others known
#define	__LOG_RT_ENABLE__		0
#define LOG_IN_RAM				0
//#define	__DEBUG_PRINT__			0
//////////// product  Information  //////////////////////////////
#define ID_VENDOR				0x248a			// for report
#define ID_PRODUCT_BASE			0x880C
// If ID_PRODUCT left undefined, it will default to be combination of ID_PRODUCT_BASE and the USB config USB_SPEAKER_ENABLE/USB_MIC_ENABLE...
// #define ID_PRODUCT			0x8869

#define STRING_VENDOR			L"Telink"
#define STRING_PRODUCT			L"2.4G Remote Control"
#define STRING_SERIAL			L"TLSR8267"

#define CHIP_TYPE				CHIP_TYPE_8267		// 8866-24, 8566-32
#define APPLICATION_DONGLE		0					// or else APPLICATION_DEVICE
#define	FLOW_NO_OS				1

#define	RF_LONG_PACKET_EN		1
//#define	RF_FAST_MODE_1M			0					// BLE mode

/////////////////// MODULE /////////////////////////////////
#define KB_MAIN_LOOP_TIME_MS    16

//////////////// audio ///////////////////////////////////
#define TL_MIC_32K_FIR_16K		1

#define	ADPCM_PACKET_LEN				144
#define TL_MIC_ADPCM_UNIT_SIZE			256

#if TL_MIC_32K_FIR_16K
	#define	TL_MIC_BUFFER_SIZE				2048
#else
	#define	TL_MIC_BUFFER_SIZE				1024
#endif

///////////////////  Hardware  //////////////////////////////
#define 	AUDIO_DEBUG_EN			1


/////////////////// Clock  /////////////////////////////////

#define CLOCK_SYS_TYPE  		CLOCK_TYPE_PLL	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	16000000

/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE	0

///////////////////  interrupt  //////////////////////////////

///////////////////  GPIO  /////////////////////////////////
#define	PC4_INPUT_ENABLE				0	//amic digital input disable
#define	PC5_INPUT_ENABLE				0	//amic digital input disable



#define	GPIO_LED				GPIO_PA4

//debug
//#define PA6_OUTPUT_ENABLE	1
//#define PA7_OUTPUT_ENABLE	1
//
//#define CHN0_LOW			((*(volatile u8*) 0x800583) &= 0xbf )
//#define CHN0_HIGH			((*(volatile u8*) 0x800583) |= 0x40 )
//#define CHN0_TOGGLE			((*(volatile u8*) 0x800583) ^= 0x40 )
//#define CHN1_LOW			((*(volatile u8*) 0x800583) &= 0x7f )
//#define CHN1_HIGH			((*(volatile u8*) 0x800583) |= 0x80 )
//#define CHN1_TOGGLE			((*(volatile u8*) 0x800583) ^= 0x80 )


#if 1

#define		KB_MAP_NORMAL	{\
				{VK_NONE	,	VK_3,	  	VK_1,		VK_NONE,	}, \
				{VK_2,	 		VK_5,	  	VK_M,		VK_4,	 	}, \
				{VK_NONE,	 	VK_NONE,	VK_NONE,	VK_NONE,	}, \
				{VK_NONE,	 	VK_NONE,	VK_NONE,	VK_NONE,	}, \
				{VK_NONE,	 	VK_NONE,	VK_NONE,	VK_NONE,	}, \
				{VK_NONE,		VK_NONE,	VK_NONE,	VK_NONE,	}, }

#define		KB_MAP_NUM		KB_MAP_NORMAL
#define		KB_MAP_FN		KB_MAP_NORMAL

#define  KB_DRIVE_PINS  {GPIO_PB1, GPIO_PB2, GPIO_PB3, GPIO_PB6}
#define  KB_SCAN_PINS   {GPIO_PD4, GPIO_PD5, GPIO_PD6, GPIO_PD7, GPIO_PE0, GPIO_PE1}

#define	PULL_WAKEUP_SRC_PB1		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PB2		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PB3		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PB6		MATRIX_ROW_PULL

#define	PULL_WAKEUP_SRC_PD4		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PD5		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PD6		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PD7		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PE0		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PE1		MATRIX_COL_PULL

#define PB1_INPUT_ENABLE		1
#define PB2_INPUT_ENABLE		1
#define PB3_INPUT_ENABLE		1
#define PB6_INPUT_ENABLE		1
#define PD4_INPUT_ENABLE		1
#define PD5_INPUT_ENABLE		1
#define PD6_INPUT_ENABLE		1
#define PD7_INPUT_ENABLE		1
#define PE0_INPUT_ENABLE		1
#define PE1_INPUT_ENABLE		1


#else
#define		KB_MAP_NORMAL	{\
				{VK_NONE	,	VK_3,	  	VK_1,		VK_NONE,	}, \
				{VK_2,	 		VK_5,	  	VK_M,		VK_4,	 	}, \
				{VK_NONE,	 	VK_NONE,	VK_NONE,	VK_NONE,	}, \
				{VK_NONE,	 	VK_NONE,	VK_NONE,	VK_NONE,	}, \
				{VK_NONE,	 	VK_NONE,	VK_NONE,	VK_NONE,	}, }//\
				//{VK_NONE,		VK_NONE,	VK_NONE,	VK_NONE,	}, }

#define		KB_MAP_NUM		KB_MAP_NORMAL
#define		KB_MAP_FN		KB_MAP_NORMAL

#define  KB_DRIVE_PINS  {GPIO_PB1, GPIO_PB2, GPIO_PB3, GPIO_PB6}
//#define  KB_SCAN_PINS   {GPIO_PD4, GPIO_PD5, GPIO_PD6, GPIO_PD7, GPIO_PE0, GPIO_PE1}
#define KB_SCAN_PINS   {GPIO_PD5, GPIO_PD6, GPIO_PD7, GPIO_PE0, GPIO_PE1}
#define	PULL_WAKEUP_SRC_PB1		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PB2		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PB3		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PB6		MATRIX_ROW_PULL

#define	PULL_WAKEUP_SRC_PD4		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PD5		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PD6		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PD7		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PE0		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PE1		MATRIX_COL_PULL

#endif

#define	MATRIX_ROW_PULL		PM_PIN_PULLDOWN_100K
#define	MATRIX_COL_PULL		PM_PIN_PULLUP_10K
#define	KB_LINE_HIGH_VALID	0


#define	PM_PIN_PULL_DEFAULT		0

///////////////////  ADC  /////////////////////////////////

///////////////////  battery  /////////////////////////////////

#define BATT_ADC_CHANNEL		0
#define BATT_FULL_VOLT			(4100)	//  mV
#define BATT_LOW_VOLT			(3700)	//  mV
#define BATT_NO_PWR_VOLT		(3400)	//  mV
#define	ADC_CHN0_ANA_INPUT		ADC_CHN_INP_ANA_7
#define ADC_CHN0_REF_SRC		ADC_REF_SRC_INTERNAL

///////////////////  Mouse  Keyboard //////////////////////////////

///////////////////  Audio  /////////////////////////////////

///////////////////  gsensor //////////////////////////////


///////////////////  POWER MANAGEMENT  //////////////////

#define PM_SUSPEND_WAKEUP_TIME  		10000	// in ms
#define PM_ENTER_SUSPEND_TIME  			4000	// in ms
#define PM_ENTER_DEEPSLEEP_TIME			600000	// in ms: 10 minute, motion,L M R wakeup
#define PM_SUSPEND_WAKEUP_FUNC_PIN 		0
#define PM_SUSPEND_WAKEUP_FUNC_LEVEL 	0

/*
 the should be the combination of the followings:
 DEEPSLEEP_WAKEUP_PIN_GPIO0 to DEEPSLEEP_WAKEUP_PIN_GPIO3
 DEEPSLEEP_WAKEUP_PIN_ANA01 to DEEPSLEEP_WAKEUP_PIN_ANA12
 */
#define PM_SUSPEND_WAKEUP_GPIO_PIN  	(GPIO_SWS)
#define PM_SUSPEND_WAKEUP_GPIO_LEVEL  	0
#define PM_DEEPSLEEP_WAKEUP_PIN 		(WAKEUP_SRC_GPIO1 | WAKEUP_SRC_GPIO2 | WAKEUP_SRC_GPIO3 | WAKEUP_SRC_PWM2 | WAKEUP_SRC_DO | WAKEUP_SRC_CN)
#define PM_DEEPSLEEP_WAKEUP_LEVEL 		0xf0
#define PM_SEARCH_TIMEOUT				3800

/////////////////// Audio /////////////////////////////////////
#define MIC_RESOLUTION_BIT		16
#define MIC_SAMPLE_RATE			16000
#define MIC_CHANNLE_COUNT		1
#define	MIC_ENOCDER_ENABLE		0

#define	ADPCM_SAMPLE_PER_FRAME		256

/////////////////// set default   ////////////////

#include "../common/default_config.h"

/////////////////// main loop, event loop  ////////////////
enum{
	EV_FIRED_EVENT_MAX = 8
};

typedef enum{
	EV_SUSPEND_NOTIFY,
	EV_WAKEUP_NOTIFY,
	EV_KEY_PRESS,
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

	EV_POLL_IDLE, //  Must be the last item in ev_poll_e
	EV_POLL_MAX,
}ev_poll_e;

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
