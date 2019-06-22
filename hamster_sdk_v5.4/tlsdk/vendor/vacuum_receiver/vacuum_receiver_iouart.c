/*
 * 	vacuum_receiver_iouart.h
 * 	Create on: Aug 20, 2014
 * 	Author: js
 */
#include "../../proj/tl_common.h"
#include "vacuum_receiver_iouart.h"
#include "vacuum_receiver.h"
//#include "../../proj/mcu_spec/gpio_8366.h"


/*************************************************************
********	use hamster gpio to simulate UART	 *********
*************************************************************/
#if RECEIVER_UART_EN

uart_pkt_data_t uart_pkt_data = {

		START_BYTE,					//start_byte
		sizeof(uart_pkt_data_t),					//uart_len
		DEVICE_TYPE_RECEIVER,	//receiver type
		FRAME_TYPE_RECEIVER,	//frame, receiver -> product

		0,						//data
		0x64,					//init battery data
#if DBG_RX_RSSI
		0,						//rssi value
#endif
		0,						//retry	2 times
		0,						//crc init
		END_BYTE,				//end_byte
	
};


uart_pkt_ack_t uart_pkt_ack = {
		0,					//start_byte
		0,	//uart_len
		0,	//receiver type
		0,	//frame, receiver -> product

		0,						//crc init
		0,				//end_byte
};

u8 *pkt_data = (u8 *)&uart_pkt_data;
u8 *pkt_ack = (u8 *)&uart_pkt_ack;

#endif

#if EMI_UART_EN

uart_pkt_data_emi_t uart_pkt_data_emi = {

		0,							//start byte
		0,							//len
		0,							//type
		0,							//cmd

		0,							//crc
		0,							//end byte

};



uart_pkt_ack_emi_t uart_pkt_ack_emi = {

		START_BYTE,						//start byte
		sizeof(uart_pkt_ack_emi_t),		//len
		DEVICE_TYPE_RECEIVER,			//type

		0,								//crc
		END_BYTE,

};

#endif



void uart_delay(void){
	u32 tick = 0;
	tick = clock_time();
	while(!clock_time_exceed(tick, UART_GAP));				// delay 104us for uart gap
}



void uart_tx(u32 pin, u8 data)
{
	
	u8 i,bit;
	//u8 data = 0x31;
	gpio_write(pin, 0);
	uart_delay();
	for(i=0; i<8; i++)
	{
		bit = (data >> i) & 0x01;
		gpio_write(pin, bit);
		uart_delay();

	}
	gpio_write(pin, 1);
	uart_delay();
}

/*********************************************************
 * name: uart_puts()
 * function: transmit string by uart
 *
********************************************************/
void uart_send_pkt(u32 pin, u8 *p, u8 len)
{
	int i;
	for(i=0; i<len; i++)
	{
		//t_tick = clock_time();
		uart_tx(pin, *p++);
		//while(!clock_time_exceed(t_tick,UART_GAP));
		uart_delay();
	}
	
}


u8 uart_rx(u32 pin)
{
    u8 rcv_data = 0;
    u8 i = 0;
    //u32 debug_tick;
    //debug_tick = clock_time();
    uart_delay();							//skip start bit
    for(i=0; i<8; i++){
    	rcv_data >>= 1;
    	if(gpio_read(pin)) {
    		rcv_data |= 0x80;
            }
#if	UART_RX_DEBUG
			GPIO_TOGGLE(DBG_UART_RX_GPIO);
#endif
        uart_delay();
    }
	//uart_delay();
    return rcv_data;
}

u8 *pkt_buf;
const u32 receive_timeout = 10000;			//uart rx timeout is 10ms
const u32 wait_timeout = 3000;				//wait 3.5ms

//u32 rx_history = U32_MAX;

const u16 wait_start_bit = 412;			//4*UART_GAP

u8 uart_rx_and_detect(u32 pin, u8 *p, u8 len)
{
	int i=0;
	u8 crc_right = 0;
	u32 crc_cnt = 0;
	u8 detect_crc = 0;
	pkt_buf = p;

	u32 tick,wait_tick;
	tick = clock_time();

	while(!clock_time_exceed(tick, receive_timeout)){
		if(gpio_read(pin)== 0){

			for(i=0; i<len; i++){
				sleep_us(51);				//delay 0.5 UART_GAP,start to rx
				*p =  uart_rx(pin);
				p++;
				wait_tick = clock_time();
				while((gpio_read(pin)!=0) && !clock_time_exceed(wait_tick, wait_start_bit));
			}

			for(i=0; i<len-2; i++){
				crc_cnt += *(pkt_buf + i);
			}
			detect_crc = *(pkt_buf + len - 2);			//crc
			crc_right = (((u8)(crc_cnt & 0xff) ^ 0x55) == detect_crc);		//crc rule
			//uart_send_pkt(IO_UART_TX, pkt_ack, sizeof(uart_pkt_ack_t));
			while(!clock_time_exceed(tick, receive_timeout));		//wait to the end
			return crc_right;		//right: return 1, wrong: 0
		}
	}

//	while(!clock_time_exceed(tick, receive_timeout));
//	return 0;

}


void uart_tx_init(void){
	//u32 uart_tx_delay_time = clock_time();
#if EMI_UART_EN

	uart_tx(EMI_UART_TX,0xaa);

#else
	uart_tx(IO_UART_TX,0xaa);
	//while(!clock_time_exceed(uart_tx_delay_time,IO_TX_AND_RX_TIME));	//sleep 10ms
#endif
}

void uart_init(void){

#if RECEIVER_UART_EN
	/****set GPIO****/
	//set TxPin
	gpio_set_input_en(IO_UART_TX, 0);
	gpio_set_output_en(IO_UART_TX, 1);

	//set RxPin
	gpio_set_input_en(IO_UART_RX, 1);
	gpio_set_output_en(IO_UART_RX,0);

	//set GPIO_GP10 for IO, output mode
	gpio_set_input_en(IO_UART_TRANS_EN, 0);
	gpio_set_output_en(IO_UART_TRANS_EN,1);
	gpio_write(IO_UART_TRANS_EN,0);			//IO_UART_TRANS_EN high level

#if UART_RX_DEBUG
	gpio_set_func(DBG_UART_RX_GPIO,AS_GPIO);
    gpio_set_input_en(DBG_UART_RX_GPIO,0);
    gpio_set_output_en(DBG_UART_RX_GPIO,1);
    gpio_write(DBG_UART_RX_GPIO,1);
#endif

#endif

	uart_tx_init();

}

