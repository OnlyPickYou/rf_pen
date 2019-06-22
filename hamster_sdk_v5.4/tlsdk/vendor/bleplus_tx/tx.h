#pragma once
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#include "..\..\proj\common\types.h"

#define SWS_DATA_OUT 			1   //sws pullup: output high, output disable

#define _USER_CONFIG_DEFINED_	1	// must define this macro to make others known 




typedef struct {
	u8  kb_mode;    //state machine
	u8  isConnect;  //0: not connect   1: connected
	u8  rf_sending;
	u8  kb_pipe_rssi;

	u8  tx_power;
	u8  tx_retry;
    u16 no_ack;

	u32 dongle_id;
    u32 loop_cnt;
} kb_status_t;

extern kb_status_t  kb_status;

typedef enum{
	STATE_IDLE  = 0,
	STATE_PAIRING,
	STATE_CONNECT,

	STATE_EMI,
	STATE_OTA,
}KB_MODE;

#define		CUSTOM_INFO_ADDRESS		0x3f00


#define KB_MAIN_LOOP_TIME_MS    12


#include "tx_default_config.h"
/////////////////// set default   ////////////////

#include "../common/default_config.h"
/////////////////// main loop, event loop  ////////////////
enum {
	EV_FIRED_EVENT_MAX = 8
};

typedef enum {
	EV_SUSPEND_NOTIFY,
	EV_WAKEUP_NOTIFY,
	EV_KEY_PRESS,
#if(MOUSE_USE_RAW_DATA)
	EV_MOUSE_RAW_DATA,
#endif	
	EV_RF_PKT_RECV,
	EV_PAIRING_START,
	EV_PAIRING_STOP,
	EV_MOUSE_EVENT,
	EV_KEYBOARD_EVENT,
#if(MODULE_SOMATIC_ENABLE)	
	EV_SOMATIC_EVENT,
#endif
	EV_EVENT_MAX,
} ev_event_e;

typedef enum {
	EV_POLL_MOUSE_EVENT, EV_POLL_KEYBOARD_EVENT,
#if(MODULE_SOMATIC_ENABLE)	
	EV_POLL_SOMATIC_EVENT,
#endif	
	EV_POLL_RF_RECV, EV_POLL_DEVICE_PKT, EV_POLL_RF_CHN_HOPPING, EV_POLL_IDLE, //  Must be the last item in ev_poll_e
	EV_POLL_MAX,
} ev_poll_e;

typedef enum {
    MS_LOW_END,                  //large current and short distance
	MS_HIGHEND_250_REPORTRATE,   //250 report rate, get sensor data per 4ms
	MS_HIGHEND_ULTRA_LOW_POWER,  //send pkt per 8ms * 3	
	MS_HIGHEND_DEFAULT,          //
} MOUSE_HIGHEND_RATE;


/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

