/*
 * dongle_rf.h
 *
 *  Created on: Feb 13, 2014
 *      Author: xuzhen
 */

#ifndef DONGLE_RF_H_
#define DONGLE_RF_H_

typedef void (*callback_rx_function) (unsigned char *);

void	proc_debug (void);
u8	get_next_channel_with_mask(u32 mask, u8 chn);
u8	update_channel_mask(u32 mask, u8 chn, u8* per);

void ll_host_init (u8 * pkt);
void ll_device_init (void);

void irq_host_timer1 (void);
void irq_host_rx(void);
void irq_host_tx(void);

void irq_device_rx(void);
void irq_device_tx(void);
int	device_send_packet (u8 * p, u32 timeout, int retry, int pairing_link);
void ll_add_clock_time (u32 ms);
void ll_channel_alternate_mode (void);

extern int device_sync;

void dongle_rf_proc(void);


#endif /* DONGLE_RF_H_ */
