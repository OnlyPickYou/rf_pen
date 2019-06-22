/*
 * kb_rf.h
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#ifndef KB_RF_H_
#define KB_RF_H_

#include "../common/rf_frame.h"

#define AUDIO_DATA_LEN    100
typedef struct {
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

	u8	rssi;
	u8	per;
	u8	seq_no;
	u8	pno;

	u32 did;

	u8 data[AUDIO_DATA_LEN]; //
}rf_packet_audio_t;

extern rf_packet_audio_t pkt_audio;

//kb_status.loop_cnt relative
#define KB_MANUAL_PARING_MOST	   (10000/KB_MAIN_LOOP_TIME_MS)  //手动配对最大时间
#define KB_NO_QUICK_SLEEP_CNT	   (20000/KB_MAIN_LOOP_TIME_MS)  //LINK最大时间
#define KB_PARING_POWER_ON_CNT      44    						 //上电自动配对包

#define HOST_NO_LINK        (kb_status.no_ack >= 400)
#define HOST_LINK_LOST		(kb_status.no_ack >= 300)

#define device_never_linked (rf_get_access_code1() == U32_MAX)

extern unsigned char* kb_rf_pkt;
extern rf_packet_pairing_t	pkt_pairing;
extern rf_packet_keyboard_t	pkt_km;

extern int	km_dat_sending;

extern int kb_pairing_mode_detect(void);
extern void kb_paring_and_syncing_proc(void);

extern void kb_rf_proc(u32 key_scaned);
extern void kb_rf_init(void);

void irq_tx_device_rx(void);
void irq_tx_device_tx(void);


#endif /* KB_RF_H_ */
