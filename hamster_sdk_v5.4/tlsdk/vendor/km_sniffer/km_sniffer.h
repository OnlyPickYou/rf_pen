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
#define ID_PRODUCT_BASE			0x8266
// If ID_PRODUCT left undefined, it will default to be combination of ID_PRODUCT_BASE and the USB config USB_SPEAKER_ENABLE/USB_MIC_ENABLE...
// #define ID_PRODUCT			0x8869

#define STRING_VENDOR			L"Telink"
#define STRING_PRODUCT			L"2.4G Wireless Demo"
#define STRING_SERIAL			L"TLSR8266"

#define CHIP_TYPE				CHIP_TYPE_8266		// 8866-24, 8566-32
#define APPLICATION_DONGLE		0			// or else APPLICATION_DEVICE
#define	FLOW_NO_OS				1

//#define	RF_LONG_PACKET_EN		1

#if (NEW_DPI_CONFIG || LR_SW_CPI)
#define	CS_OPT_TEST_MODE_LEVEL		3	//code size small
#define PARING_TIME_ON_POWER		48
#else
#define	CS_OPT_TEST_MODE_LEVEL		1
#endif


#define	BATTERY_DETECTION_WITH_LDO_SET	1
//#define	DCDC_SUPPLY_VOLTAGE				ll_ldo_set_with_bat_2p0_to_2p25

/////////////////// MODULE /////////////////////////////////
#define MODULE_PM_ENABLE		1
#define MODULE_LED_ENABLE       0

#define MODULE_MOUSE_ENABLE		1
#define	MOUSE_OPTICAL_EN		1
#define MOUSE_HAS_BUTTON		1

#define	MOUSE_MOTION			GPIO_PWM2
#define	MOUSE_WHEEL0			GPIO_PWM0
#define	MOUSE_WHEEL1			GPIO_PWM1
#define	MOUSE_BUTTON_LEFT		GPIO_GP1
#define	MOUSE_BUTTON_RIGHT		GPIO_GP2
#define	MOUSE_BUTTON_MIDDLE		GPIO_GP3
#define	MOUSE_BUTTON_FORWARD	GPIO_DO
#define	MOUSE_BUTTON_BACKWARD	GPIO_CN
#define	MOUSE_BUTTON_CPI		GPIO_SWS

#define	PULL_WAKEUP_SRC_PWM0	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_PWM1	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_PWM2	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_GPIO1	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_GPIO2	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_GPIO3	PM_PIN_PULLUP_1M

//#define	PULL_WAKEUP_SRC_GPIO0	PM_PIN_PULLUP_1M	//battery detection pin, no pullup
#define	PULL_WAKEUP_SRC_GPIO4	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_MSDO	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_MSDI	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_MCSN	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_MCLK	PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_CK		PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_DI		PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_DO		PM_PIN_PULLUP_1M
#define	PULL_WAKEUP_SRC_CN		PM_PIN_PULLUP_1M


#define	IRQ_GPIO0_ENABLE		1

#define	TX_RETRY_MAX			3
#define	TX_ACK_REQUEST_MAX		1
#define	TX_FAST_MODE_ENABLE		0
#define	RX_REQ_INTERVAL			8
#define	WAKEUP_SEARCH_MODE		0

///////////////////  Hardware  //////////////////////////////
#define GPIO_IR_LEARN_IN	GPIO_GP0
/////////////////// Clock  /////////////////////////////////
#if 1
#define CLOCK_SYS_TYPE  		CLOCK_TYPE_PLL	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	32000000
#else
#define CLOCK_SYS_TYPE  		CLOCK_TYPE_OSC	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	32000000
#endif
/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE	0

///////////////////  interrupt  //////////////////////////////
#define FREQ_ABOVE_2600_FOR_CAVY		0
///////////////////  GPIO  /////////////////////////////////
//  only need to define those are not default
//  all gpios are default to be output disabled, input disabled, output to 0, output strength to 1

//  I2C  GPIO
#define PIN_I2C_CN				GPIO_CN
#define PIN_I2C_SCL				GPIO_CK
#define PIN_I2C_SDA				GPIO_DI

#define PIN_SIF_SCL				GPIO_CK
#define PIN_SIF_SDA				GPIO_DI

#define	DP_FUNC					AS_GPIO
#define	DM_FUNC					AS_GPIO
#define	DP_OUTPUT_ENABLE		0
#define	DP_DATA_OUT				1
#define	DP_INPUT_ENABLE			1
#define	DM_INPUT_ENABLE			1

#define	DM_OUTPUT_ENABLE		0
#define	DM_INPUT_ENABLE			1
#define	DM_DATA_OUT				1

////////////////////////////////////////////////////////////////////////////
#define	CN_FUNC					AS_GPIO
#define	CK_FUNC					AS_GPIO
#define	DO_FUNC					AS_GPIO
#define	DI_FUNC					AS_GPIO

#define	CN_DATA_OUT				0
#define	CK_DATA_OUT				1

#define CN_OUTPUT_ENABLE		0
#define CK_OUTPUT_ENABLE		1
#define DO_OUTPUT_ENABLE		0
#define DI_OUTPUT_ENABLE		0

#define CN_INPUT_ENABLE			1
#define CK_INPUT_ENABLE			1
#define DO_INPUT_ENABLE			1
#define DI_INPUT_ENABLE			1

#define	PWM0_FUNC               AS_GPIO
#define	PWM1_FUNC               AS_GPIO
#define	PWM2_FUNC               AS_GPIO
#define	SWM_FUNC                AS_GPIO

#define	GPIO1_INPUT_ENABLE		1
#define	GPIO2_INPUT_ENABLE		1
#define	GPIO3_INPUT_ENABLE		1

#define	GPIO4_INPUT_ENABLE		1
#define	GPIO5_INPUT_ENABLE		1
#define	GPIO7_INPUT_ENABLE		1
//#define	GPIO0_INPUT_ENABLE		1	// use as battery detection input, can not enable

//////////////////    RF configuration //////////////
#define RF_PROTOCOL				RF_PROTO_PROPRIETARY		//  RF_PROTO_PROPRIETARY / RF_PROTO_RF4CE / RF_PROTO_ZIGBEE
///////////////////  ADC  /////////////////////////////////

///////////////////  battery  /////////////////////////////////

#define BATT_ADC_CHANNEL		0
#define BATT_FULL_VOLT			(4100)	//  mV
#define BATT_LOW_VOLT			(3700)	//  mV
#define BATT_NO_PWR_VOLT		(3400)	//  mV
#define	ADC_CHN0_ANA_INPUT		ADC_CHN_INP_ANA_7
#define ADC_CHN0_REF_SRC		ADC_REF_SRC_INTERNAL

///////////////////  Mouse  Keyboard //////////////////////////////
#define MOUSE_HAS_WHEEL			0
#define MOUSE_HAS_BUTTON		1

///////////////////  Audio  /////////////////////////////////
#define MIC_RESOLUTION_BIT		16
#define MIC_SAMPLE_RATE			16000
#define MIC_CHANNLE_COUNT		1
#define	MIC_ENOCDER_ENABLE		0

///////////////////  gsensor //////////////////////////////
#define GSENSOR_START_MOVING_CHECK      0
#define GSENSOR_CRANKING_ENABLE         0
#define AIRMOUSE_ENABLE_CHECK           0

///////////////////  POWER MANAGEMENT  //////////////////
#define	PM_IDLE_TIME					64
#define PM_ACTIVE_SUSPEND_WAKEUP_TIME  	8 		// in ms
#if	(MOV_PIN_DSCNNCT || NO_DETECT_MOTION )
#define ULTRA_LOW_POWER					0
#define PM_SUSPEND_WAKEUP_TIME  		100		// in ms
#else
#define ULTRA_LOW_POWER					0
#define PM_SUSPEND_WAKEUP_TIME  		10000	// in ms
#endif
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

///////////GPIO CONFIG FOR XIWANG 3IN1/////////////////////////////////////
#define KEY_GP3_10K_PULLUP_EN		analog_write(0x0c, (analog_read(0x0c) & 0x3f) | PM_PIN_PULLUP_10K << 6)
#define KEY_DO_1M_PULLUP_EN 		analog_write(0x0d, (analog_read(0x0d) & 0xcf) | PM_PIN_PULLUP_1M << 4)
#define KEY_DO_100K_PULLDOWN_EN 	analog_write(0x0d, (analog_read(0x0d) & 0xcf) | PM_PIN_PULLDOWN_100K << 4)

///////////////////  USB   /////////////////////////////////

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
