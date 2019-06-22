/*
 * mouse_batt.h
 *
 *  Created on: Feb 14, 2014
 *      Author: xuzhen
 */

#ifndef MOUSE_BATT_H_
#define MOUSE_BATT_H_


#ifndef MOSUE_BATTERY_LOW_DETECT
#define MOSUE_BATTERY_LOW_DETECT  1
#endif

#define MOUSE_BATT_CHN_REUSE_FLAG    0x80
#define MOUSE_BATT_CHN_REUSE_BTN     0x60
#define MOUSE_BATT_CHN_REUSE_BTN_R     0x20
#define MOUSE_BATT_CHN_REUSE_BTN_M     0x40

#define MOUSE_BATT_CHN_REAL_MASK     0x1f

#define mouse_batt_low_alarm_cnt ( (rc_led_cfg[E_LED_BAT_LOW].repeat_count * (rc_led_cfg[E_LED_BAT_LOW].on_time + rc_led_cfg[E_LED_BAT_LOW].on_time)) << 3 )
#define mouse_batt_detect_time  ( 1 + mouse_batt_low_alarm_cnt )

#if MOSUE_BATTERY_LOW_DETECT
void mouse_batt_det_process( rc_status_t *rc_status );
#else
static inline void mouse_batt_det_process( rc_status_t *rc_status ){}
#endif

#endif /* MOUSE_BATT_H_ */
