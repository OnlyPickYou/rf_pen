/*
 * mouse_rf.h
 *
 *  Created on: Feb 12, 2014
 *      Author: xuzhen
 */

#ifndef VACUUM_CONTROLLER_RF_H_
#define VACUUM_CONTROLLER_RF_H_

#ifndef TEST_VACUUM
#define	TEST_VACUUM		0
#endif

#define LINK_PIPE_CODE_OK  		0x01
#define LINK_RCV_DONGLE_DATA  	0x02
#define LINK_WITH_DONGLE_OK	    (LINK_PIPE_CODE_OK | LINK_RCV_DONGLE_DATA)


//vc_status.loop_cnt relative
#define KB_MANUAL_PARING_MOST	   (8000/KB_MAIN_LOOP_TIME_MS)  //手动配对最大时间
#define KB_NO_QUICK_SLEEP_CNT	   (20000/KB_MAIN_LOOP_TIME_MS)  //LINK最大时间
#define KB_PARING_POWER_ON_CNT      44    						 //上电自动配对包

#define HOST_NO_LINK        (vc_status.no_ack >= 200)
#define HOST_LINK_LOST		(vc_status.no_ack >= 100)

#define device_never_linked (rf_get_access_code1() == U32_MAX)
#define	MANNUAL_PARING_BACK_LINK		1

#define KEYBOARD_FRAME_DATA_NUM			2

extern u8* kb_rf_pkt;
extern rf_packet_pairing_t	pkt_pairing;
extern rf_packet_pairing_t	pkt_manual;

#if(WITH_SPELL_PACKET_EN)
extern rf_packet_vacuum_t	pkt_km;
#else
extern rf_packet_keyboard_t	pkt_km;
#endif
extern rf_packet_keyboard_t pkt_conf;

//extern rf_packet_vacuum_t 	pkt_vc;

extern int	km_dat_sending;

extern void vc_paring_and_syncing_proc(void);

extern void vc_rf_proc(u8 btn_new);
extern void vc_rf_init(void);

//void irq_device_rx(void);
void irq_device_tx(void);


#define DO_TASK_WHEN_RF_EN      1
//typedef void (*task_when_rf_func) (void);

//extern task_when_rf_func p_task_when_rf;

#endif /* MOUSE_RF_H_ */
