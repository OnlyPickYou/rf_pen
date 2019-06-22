#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"
#include "../common/device_power.h"

#include "../link_layer/rf_ll.h"

#if (__PROJECT_RX_8267__)



#define		TEST_PKT_BUFF_SIZE		48

unsigned char  rx_test_buff[TEST_PKT_BUFF_SIZE*2] __attribute__((aligned(4)));
int			rx_test_wptr;

int rxRcv_cnt = 0;
int rxRcv_correct_cnt = 0;
int rxRcv_flg;


void rx_test_init (void)
{
	reg_dma_rf_rx_addr = (u16)(u32) (rx_test_buff);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (TEST_PKT_BUFF_SIZE>>4);   // rf rx buffer enable & size
	reg_dma_chn_irq_msk = 0;
	reg_irq_mask |= FLD_IRQ_ZB_RT_EN;    //enable RF irq
	reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;  //enable TX and RX irq
}

_attribute_ram_code_ void rx_rcv_handler(void)
{
	rxRcv_cnt ++;

	u8 * raw_pkt = (u8 *) (rx_test_buff + rx_test_wptr * TEST_PKT_BUFF_SIZE);
	rx_test_wptr = (rx_test_wptr + 1) & 1;
	reg_dma_rf_rx_addr = (u16)(u32) (rx_test_buff + rx_test_wptr * TEST_PKT_BUFF_SIZE); //set next buffer

	reg_rf_irq_status = FLD_RF_IRQ_RX;

	if	(	raw_pkt[0] >= 15 &&
			RF_PACKET_LENGTH_OK(raw_pkt) &&
			RF_PACKET_CRC_OK(raw_pkt) )	{


		rxRcv_correct_cnt ++;
		rxRcv_flg = 1;


		raw_pkt[0] = 1;
	}
}


u32 adbg_irq_cnt;
_attribute_ram_code_ void irq_handler(void)
{

	adbg_irq_cnt ++;

	u32 src = reg_irq_src;

	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		rx_rcv_handler();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		//.....
		reg_rf_irq_status = FLD_RF_IRQ_TX;
	}
}

u32 adbg_cnt;

u8 	led_state = 0;
void  user_init(void)
{
	gpio_set_output_en(GPIO_PB1,1);
	gpio_set_input_en(GPIO_PB1,0);

	gpio_write(GPIO_PB1,1);
	sleep_us(1000000);   	//1s delay
	gpio_write(GPIO_PB1,0);
	led_state = 0;  //led off
}


u32 a_dbg_main_loop;
void rx_test(void)
{
	rf_power_enable(1);
	rx_test_init ();
	irq_enable();
	rf_receiving_pipe_enble(0x3f);	// channel mask


	rf_set_power_level_index (RF_POWER_8dBm);
	//rf_set_tx_pipe (PIPE_PARING);
	rf_set_channel (10, RF_CHN_TABLE);  //2450
	rf_set_rxmode();

	while(1){
		a_dbg_main_loop++;

		if(rxRcv_flg){
			rxRcv_flg = 0;
			led_state = !led_state;
			gpio_write(GPIO_PB1, led_state);  //led shine
		}

		sleep_us(10000); //10 ms
	}
}

void main_loop(void)
{
	rx_test();
}










#endif  //end of __PROJECT_TX_8267__
