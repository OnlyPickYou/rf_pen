#pragma once
#include "../common/mouse_type.h"
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#define _USER_CONFIG_DEFINED_	1	// must define this macro to make others known 

#define MAX_MOUSE_BUTTON       6

typedef struct {

	u32 button[MAX_MOUSE_BUTTON];    //the sequence is left, right, middle, FF, FB, DPI
	u32 led_cntr;
	u32 sensor_direction[2];

	u32 gpio_level;   // bit 0-5, button pull up/down ; bit 6 led on level; bit 8, 9 sensor direction pull up/down

	u32 vbat_channel;

	u32 wheel[2];

	u32 sensor_data;
	u32 sensor_sclk;
	u32 sensor_int;

}mouse_hw_t;


typedef struct {

	u8  mouse_mode;                //EMI mode, power on, pairing auto/manual, normal mode etc
	u8  channel;
	u8  last_ACK;                  //indicate whether get the ACK for previous package send
	u8  tx_seq;                    //RF TX sequence number, increase every RF TX

	u16 idle_cnt_8ms;              //count increase if no mouse action within 8ms
	u16 idle_cnt_long_suspend;     //count increase if no mouse action for longer suspend,
								   // may still keep RF link, and after reach some limit may need goto deep sleep mode
	u8  rcv_rssi;
	u8  cpi;
	u8  sensor_dir;
	u8  wheel_dir;

	u32 dongle_id;
	u32 device_id;

	u8  battery_level;
	u8  mouse_battery_low_cnt;
    u8  mouse_sensor_status;

    device_led_t   *led_define;
	mouse_hw_t    *hw_define;
	mouse_data_t  *data;

} mouse_status_t;


typedef enum{
	STATE_POWERON =0,
	STATE_NORMAL,
	STATE_SUSPEND,
	STATE_DEEPSLEEP,
	STATE_PAIRING ,
	STATE_EMI,

}MOUSE_MODE;

#include "mouse_default_config.h"
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

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

