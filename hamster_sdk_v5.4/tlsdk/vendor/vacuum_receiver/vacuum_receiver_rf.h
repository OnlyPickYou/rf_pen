/*
 * vacuum_receiver_rf.h
 *
 *  Created on: Aug 20, 2015
 *
 */

#ifndef _VACUUUM_RECEIVER_RF_H_
#define _VACUUUM_RECEIVER_RF_H_

typedef void (*callback_rx_func) (u8 *);
typedef void (*task_when_rf_func) (void);

#if(CLOCK_SYS_CLOCK_HZ == 32000000 || CLOCK_SYS_CLOCK_HZ == 24000000)
	#define			SHIFT_US		5
#elif(CLOCK_SYS_CLOCK_HZ == 16000000 || CLOCK_SYS_CLOCK_HZ == 12000000)
	#define			SHIFT_US		4
#elif(CLOCK_SYS_CLOCK_HZ == 8000000)
	#define			SHIFT_US		3
#else
	#error clock not set properly
#endif

u8	get_next_channel_with_mask(u32 mask, u8 chn);
//u8	update_channel_mask(u32 mask, u8 chn, u8* per);

void ll_host_init (u8 * pkt);
//void ll_device_init (void);

void irq_host_timer1 (void);
void irq_host_rx(void);
void irq_host_tx(void);

//void irq_device_rx(void);
//void irq_device_tx(void);
//int	device_send_packet (u8 * p, u32 timeout, int retry, int pairing_link);
//void ll_add_clock_time (u32 ms);
//void ll_channel_alternate_mode (void);

extern int device_sync;
extern task_when_rf_func p_task_when_rf;

#define DO_TASK_WHEN_RF_EN      1



#endif /* _RF_LINK_LAYER_H_ */
