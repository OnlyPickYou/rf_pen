/*
 * kb_test.c
 *
 *  Created on: 2015-1-23
 *      Author: Administrator
 */

#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"

#include "tx_emi.h"
#include "tx_rf.h"
#include "tx_custom.h"
#include "tx_info.h"
#include "tx_batt.h"
#include "tx_pm.h"
#include "tx_led.h"
#include "tx_test.h"

#if  (__PROJECT_TX_8267__)

#define GPIO_FOR_DEBUG			GPIO_PA4

void dbg_gpio_init(void)
{
	gpio_set_output_en(GPIO_FOR_DEBUG, 1);
	gpio_set_input_en(GPIO_FOR_DEBUG, 0);
	gpio_write(GPIO_FOR_DEBUG, 0);
}


void debug_sys_tick(void)
{
	static u32 tick = 0;
	if(clock_time_exceed(tick,1000000)){
		tick = clock_time();
		pkt_km.per++;
	}
}


void adc_test(void)
{

}

void pm_test(void)
{
	rf_power_enable(0);
	sleep_us(5000000);

	//gpio_shutdown();
	while(1){
		kb_cpu_sleep_wakeup(0,PM_WAKEUP_CORE,0);
		sleep_us(2000000);
	}
}

void clk_32k_rc_test(void)
{
}


extern kb_data_t	kb_event;
int simu_key_data(void)
{
	return 0;
}

/***********************************************************************************
 *
 * 						          RF TEST
 *
 ***********************************************************************************/
#define		TEST_PKT_BUFF_SIZE		48
unsigned char  		test_rf_rx_buff[TEST_PKT_BUFF_SIZE*2] __attribute__((aligned(4)));

void rf_test_init(void)
{
	reg_dma_rf_rx_addr = (u16)(u32) (test_rf_rx_buff);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (TEST_PKT_BUFF_SIZE>>4);   // rf rx buffer enable & size
	reg_dma_chn_irq_msk = 0;
	reg_irq_mask |= FLD_IRQ_ZB_RT_EN;    //enable RF & timer1 interrupt
	reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;
}

u8 a_dbg_main_loop;
void tx_test(void)
{
	rf_test_init();
	dbg_gpio_init();

	rf_receiving_pipe_enble(0x3f);	// open all receive pipe
	rf_set_tx_pipe (PIPE_PARING);   //use pipe0 for TX send, you can can use other pipe
	rf_set_power_level_index (RF_POWER_8dBm); //set TX power max, you can change anywhere in your code

	gpio_write(GPIO_FOR_DEBUG, 1);
	sleep_us(3000000);
	gpio_write(GPIO_FOR_DEBUG, 0);


	rf_power_enable(1);
	rf_set_channel (10, RF_CHN_TABLE);  //2450
	pkt_km.did = 0x88888888; //for recognize

	while(1){

		pkt_km.type++;
		rf_send_packet ((u8*)&pkt_km, 300, 0);

		//led shine
		a_dbg_main_loop++;
		if( (a_dbg_main_loop&3)==0){  //50*8= 400 ms
			gpio_write(GPIO_FOR_DEBUG, 0);
		}
		else if( (a_dbg_main_loop&3)==2){
			gpio_write(GPIO_FOR_DEBUG, 1);
		}
		sleep_us(200000);
	}
}


/***********************************************************************************
 *
 * 						          PM TEST
 *
 ***********************************************************************************/
void gpio_shutdown(void)
{
#if 0
	//disable ie
	write_reg8(0x800581,0x00);
	write_reg8(0x800589,0x00);
	write_reg8(0x800591,0x00);
	write_reg8(0x800599,0x00);
	write_reg8(0x8005a1,0x3e); //SWS MSPI


	//disable oen
	write_reg8(0x800582,0xff);
	write_reg8(0x80058a,0xff);
	write_reg8(0x800592,0xff);
	write_reg8(0x80059a,0xff);
	write_reg8(0x8005a2,0x3f);

	//disable dataO
	write_reg8(0x800583,0x00);
	write_reg8(0x80058b,0x00);
	write_reg8(0x800593,0x00);
	write_reg8(0x80059b,0x00);
	write_reg8(0x8005a3,0x00);
#endif
}

#endif  //end of __PROJECT_TX_8267__
