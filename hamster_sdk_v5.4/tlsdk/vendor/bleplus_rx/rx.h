#pragma once
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif


#define SWS_DATA_OUT 			1   //sws pullup: output high, output disable

#define _USER_CONFIG_DEFINED_	1	// must define this macro to make others known 



#include "rx_default_config.h"
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

