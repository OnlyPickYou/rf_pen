/*
 * kb_pm.h
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#ifndef KB_PM_H_
#define KB_PM_H_


#define SLEEP_MODE_BASIC_SUSPEND 		0
#define SLEEP_MODE_LONG_SUSPEND 		1
#define SLEEP_MODE_WAIT_DEEP    		2
#define SLEEP_MODE_DEEPSLEEP     		3

typedef struct{
    u8   mode;
    u8   device_busy;
    u8   quick_sleep;
    u8   wakeup_src;

    u8   cnt_basic_suspend;
    u8   thresh_basic_suspend;
    u8   cnt_long_suspend;
    u8   thresh_long_suspend;

    u32  wakeup_tick;
    u32  next_wakeup_tick;
} kb_slp_cfg_t;



extern void kb_pm_init(void);
extern void kb_pm_proc(void);

extern kb_slp_cfg_t kb_sleep;

#endif /* KB_PM_H_ */
