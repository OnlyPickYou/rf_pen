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

#define	BATT_DETECT_HIGH	0x64			//when battery detected, vin > Vhigh(2.2v),return 2
#define	BATT_DETECT_LOW		1			//when battery detected, vin < Vlow(1.9v)
#define	BATT_DETECT_1P9		0			//when battery detected, (1.9v) < vin < 2.0v
#define BATT_DETECT_2P0		0x0a		//when battery detected, (2.0v) < vin < 2.1v
#define BATT_DETECT_2P1		0x14		//when battery detected, (2.1v) < vin < 2.2v

#ifndef INVALID_BATT_VALUE
#define	INVALID_BATT_VALUE	1
#endif

u8 vacuum_battery_low_detect(u8 vbat_chn, u8 high_ref, u8 high_scal);
void batt_detect_and_fliter(void);

#endif /* MOUSE_BATT_H_ */
