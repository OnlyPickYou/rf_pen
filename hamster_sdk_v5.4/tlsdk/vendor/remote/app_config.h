#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#define _USER_CONFIG_DEFINED_	1	// must define this macro to make others known
#define	__LOG_RT_ENABLE__		0
//#define	__DEBUG_PRINT__			0
//////////// product  Information  //////////////////////////////
#define ID_VENDOR				0x248a			// for report
#define ID_PRODUCT_BASE			0x880C
// If ID_PRODUCT left undefined, it will default to be combination of ID_PRODUCT_BASE and the USB config USB_SPEAKER_ENABLE/USB_MIC_ENABLE...
// #define ID_PRODUCT			0x8869

#define STRING_VENDOR			L"Telink"
#define STRING_PRODUCT			L"2.4G Remote Control"
#define STRING_SERIAL			L"TLSR8266"

#define CHIP_TYPE				CHIP_TYPE_8266		// 8866-24, 8566-32
#define APPLICATION_DONGLE		0					// or else APPLICATION_DEVICE
#define	FLOW_NO_OS				1

#define	RF_LONG_PACKET_EN		1
//#define	RF_FAST_MODE_1M			0					// BLE mode

/////////////////// MODULE /////////////////////////////////



///////////////////  Hardware  //////////////////////////////

/////////////////// Clock  /////////////////////////////////

#define CLOCK_SYS_TYPE  		CLOCK_TYPE_PLL	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
//#define CLOCK_SYS_TYPE  		CLOCK_TYPE_OSC	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	32000000

/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE	0

///////////////////  interrupt  //////////////////////////////

///////////////////  GPIO  /////////////////////////////////
//  only need to define those are not default
//  all gpios are default to be output disabled, input disabled, output to 0, output strength to 1
////////////////////////////////////////////////////////////////////////////
#define		VK_MIC				0xf5
#define		VK_LEFTB			0xf6

//main_loop time
#define KB_MAIN_LOOP_TIME_MS    12


#define	GPIO_LED				GPIO_PC2			//PWM1
#define	PWM_LED					GPIO_PC2			//PWM1
#define	PWMID_LED				1

#define		KB_MAP_NORMAL	{\
				{VK_W_MUTE,		VK_3,	  	VK_1,		VK_MEDIA,	}, \
				{VK_2,	 		VK_LEFTB,	VK_MIC,		VK_4,	 	}, \
				{VK_RIGHT,	 	VK_NONE,	VK_ENTER,	VK_LEFT,	}, \
				{VK_W_BACK,	 	VK_NONE,	VK_DOWN,	VK_HOME,	}, \
				{VK_VOL_UP,	 	VK_NONE,	VK_MMODE,	VK_VOL_DN,	}, \
				{VK_WEB,		VK_NONE,	VK_UP,		VK_POWER,	}, }

#define		KB_MAP_NUM		KB_MAP_NORMAL
#define		KB_MAP_FN		KB_MAP_NORMAL

#if 1

#define  KB_DRIVE_PINS  {GPIO_PA1, GPIO_PA5, GPIO_PA6, GPIO_PA7}
#define  KB_SCAN_PINS   {GPIO_PB7, GPIO_PC6, GPIO_PE5, GPIO_PE6, GPIO_PF0, GPIO_PE4}

#else			//C43 version QFN56

#define  KB_DRIVE_PINS  {GPIO_PA1, GPIO_PA4, GPIO_PA5, GPIO_PA6}
#define  KB_SCAN_PINS   {GPIO_PB7, GPIO_PC6, GPIO_PE3, GPIO_PE6, GPIO_PF0, GPIO_PE0}

#endif

#define	MATRIX_ROW_PULL		PM_PIN_PULLDOWN_100K
#define	MATRIX_COL_PULL		PM_PIN_PULLUP_10K
#define	KB_LINE_HIGH_VALID	0

#define	PULL_WAKEUP_SRC_PA1		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PA4		MATRIX_ROW_PULL				//C43 verison QFN56
#define	PULL_WAKEUP_SRC_PA5		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PA6		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PA7		MATRIX_ROW_PULL

#define	PULL_WAKEUP_SRC_PB7		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PC6		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PE5		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PE6		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PF0		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PE4		MATRIX_COL_PULL

#define	PULL_WAKEUP_SRC_PE0		MATRIX_COL_PULL			//C43 verison QFN56
#define	PULL_WAKEUP_SRC_PE3		MATRIX_COL_PULL			//C43 verison QFN56

#define	PA7_FUNC				AS_GPIO

#define	PM_PIN_PULL_DEFAULT		0

#define PULL_WAKEUP_SRC_PA0           PM_PIN_PULL_DEFAULT  //SWS
//#define PULL_WAKEUP_SRC_PA1           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PA2           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PA3           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PA4           PM_PIN_PULL_DEFAULT			//C43 verison QFN56
//#define PULL_WAKEUP_SRC_PA5           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PA6           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PA7           PM_PIN_PULL_DEFAULT  //SWM
#define PULL_WAKEUP_SRC_PB0           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PB1           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PB2           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PB3           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PB4           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PB5           PM_PIN_PULL_DEFAULT  //DM
//#define PULL_WAKEUP_SRC_PB6           PM_PIN_PULL_DEFAULT  //DP
//#define PULL_WAKEUP_SRC_PB7           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PC0           PM_PIN_PULL_DEFAULT			//mic+
#define PULL_WAKEUP_SRC_PC1           PM_PIN_PULL_DEFAULT			//mic-
#define PULL_WAKEUP_SRC_PC2           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PC3           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PC4           PM_PIN_PULL_DEFAULT			// bat detection
#define PULL_WAKEUP_SRC_PC5           PM_PIN_PULL_DEFAULT			// mic bias
//#define PULL_WAKEUP_SRC_PC6           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PC7           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD0           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD1           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD2           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD3           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD4           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD5           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD6           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PD7           PM_PIN_PULLUP_1M				// Gyro power enable, active low
//#define PULL_WAKEUP_SRC_PE0           PM_PIN_PULL_DEFAULT				//C43 verison QFN56
#define PULL_WAKEUP_SRC_PE1           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PE2           PM_PIN_PULL_DEFAULT				//C43 verison QFN56
//#define PULL_WAKEUP_SRC_PE3           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PE4           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PE5           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PE6           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PE7           PM_PIN_PULL_DEFAULT
//#define PULL_WAKEUP_SRC_PF0           PM_PIN_PULL_DEFAULT
#define PULL_WAKEUP_SRC_PF1           PM_PIN_PULL_DEFAULT

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
