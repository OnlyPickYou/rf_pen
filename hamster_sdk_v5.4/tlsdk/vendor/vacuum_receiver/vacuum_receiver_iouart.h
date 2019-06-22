/*
Created on: 2015-8-21
 *      Author: zjs
 */
#ifndef VACUUM_RECEIVER_IOUART_H
#define VACUUM_RECEIVER_IOUART_H

#include "../../proj/common/types.h"



#ifndef		MAINBOARD_UART_EN
#define		MAINBOARD_UART_EN		0
#endif


#ifndef		RECEIVER_TX_DELAY_TIME
#define		RECEIVER_TX_DELAY_TIME		100000		//tx delay need 100ms
#endif

#define		IO_TRANS_EN_DELAY_TIME		200000		//IO transmit packet time:200ms
#define		IO_TX_AND_RX_TIME			10000		//IO tx or rx delay 10ms


#define		DEVICE_TYPE_RECEIVER	0x10
#define		DEVICE_TYPE_PRODUCT		0x20

#define		FRAME_TYPE_RECEIVER		0x10
#define		FRAME_TYPE_PRODUCT		0x20

#define		START_BYTE				0xaa
#define		END_BYTE				0xbb


#ifndef 	UART_RX_DEBUG
#define		UART_RX_DEBUG	0
#endif

#define	 DBG_RX_RSSI		0

#if UART_RX_DEBUG
#define GPIO_TOGGLE(pin)			( (*((volatile u32*)0x800584)) ^= pin )
#endif


//#define IOUART_TX    GPIO_GP6
//#define IOUART_RX    GPIO_GP9


#define 	IO_UART_TX					GPIO_GP2
#define		IO_UART_RX					GPIO_GP4
#define 	IO_UART_TRANS_EN			GPIO_GP10		//set gpio_gp10 for uart enable

#define		RECEIVER_UART_EN		1

#define 	RETRY_TIMES				5

#ifndef		EMI_UART_EN
#define		EMI_UART_EN				1
#endif

#define		EMI_UART_RX_EN			1

#define		EMI_TX_DELAY_TIME			10000		//tx delay need 100ms
#define		EMI_TRANS_EN_DELAY_TIME		100000		//IO transmit packet time:150ms
#define		EMI_TX_AND_RX_TIME			10000		//IO tx or rx delay 10ms

#if EMI_UART_RX_EN

#define 	EMI_UART_TX					GPIO_GP2	//emi uart tx
#define		EMI_UART_RX					GPIO_GP4	//emi uart rx
#define		EMI_UART_RECV_EN			GPIO_GP10

#endif

//#define IOUART_BAUD_RATE 19200

#define		UART_BAUD_RATE			9600
#define		UART_GAP				104


typedef struct{
	u8 start_byte;
	u8 len;
	u8 type;		//rf module
	u8 flow;		//flow, 0x10:data transmit, 0x20:ack

	u8 data;		//btn data
	u8 batt;		//battery data
#if DBG_RX_RSSI
	u8 rssi;		//rssi value
#endif
	u8 retry;		//retry times
	u8 crc;			//CRC check

	u8 end_byte;
}uart_pkt_data_t;


typedef struct{
	u8 start_byte;
	u8 len;
	u8 type;
	u8 cmd;

	u8 crc;
	u8 end_byte;

}uart_pkt_data_emi_t;


typedef struct{
	u8 start_byte;
	u8 len;
	u8 type;		// product
	u8 flow;

	u8 crc;
	u8 end_byte;
}uart_pkt_ack_t;

typedef struct{
	u8 start_byte;
	u8 len;
	u8 type;		// product

	u8 crc;
	u8 end_byte;
}uart_pkt_ack_emi_t;



extern void uart_delay(void);
extern void uart_init(void);
extern void uart_tx(u32 pin, u8 data);
extern u8 uart_rx(u32 pin);
extern unsigned char IOUART_IsStart(void);

extern void uart_send_pkt(u32 pin, u8 *p, u8 len);
extern u8 uart_rx_and_detect(u32 pin, u8 *p, u8 len);
//extern u8 uart_send_packet(u8 *p, u8 retry);
//extern void uart_proc(u8 btn_changed);
extern void uart_tx_init(void);


#endif
