#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"

#include "../common/device_power.h"

#include "keyboard.h"

#include "kb_emi.h"
#include "kb_rf.h"
#include "kb_custom.h"
#include "kb_info.h"
#include "kb_batt.h"
#include "kb_pm.h"
#include "kb_ota.h"
#include "kb_led.h"
#include "kb_test.h"


void kb_platform_init(void);
extern  void kb_keyScan_init(void);

kb_status_t  kb_status;

extern kb_data_t	kb_event;

void  user_init(void)
{
	kb_status.no_ack = 1;

	kb_custom_init();
	kb_batt_det_init();
	kb_info_load();
	kb_rf_init();

	int status = kb_status.host_keyboard_status | (kb_status.kb_mode == STATE_POWERON ? KB_NUMLOCK_STATUS_POWERON : 0);
//	kb_scan_key (status, 1);		//only valid in power_on pressing key pairing
	kb_status.host_keyboard_status = KB_NUMLOCK_STATUS_INVALID;

	kb_platform_init();
	kb_keyScan_init();  //decide the key mapping to use
	kb_pm_init();
}


void kb_platform_init(void)
{
//there is no light on this RC board

	//emi paring±ØÐëÔÚSTATE_POWERON?
//	if(kb_status.kb_mode == STATE_POWERON){
//		if(kb_pairing_mode_detect()){
//			kb_status.kb_mode  = STATE_PAIRING;
//		}
//	}
	//set different key map by the vol level of GP31
	gpio_set_func(GPIO_GP31,AS_GPIO);
	gpio_set_output_en(GPIO_GP31, 0);
	gpio_set_input_en(GPIO_GP31, 1);
	analog_write(0x42,analog_read(0x42)|0xc0);
}

_attribute_ram_code_ void irq_handler(void)
{
	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_device_rx();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		//irq_device_tx();
		reg_rf_irq_status = FLD_RF_IRQ_TX;
	}
}



void kb_task_when_rf ( void ){
	kb_batt_det_process();
}


volatile int dbg_loops;
u32 key_scaned;
u8  bat_low_det;
void main_loop(void)
{
	dbg_loops++;
	key_scaned = kb_scan_key (0, !km_dat_sending);
	if(kb_pairing_mode_detect()==1){
		kb_status.rsv1 = 1;			//home mode
		kb_status.kb_mode  = STATE_PAIRING;
	}
	else if(kb_pairing_mode_detect()==2)
	{
		kb_status.rsv1 = 0;			//factory mode
		kb_status.kb_mode  = STATE_PAIRING;
	}
#if ZZCZ
	if(kb_status.kb_mode == STATE_NORMAL)
	{
		if ((0x06 == kb_event.keycode[0]&&  0x07  == kb_event.keycode[1])||(0x07 == kb_event.keycode[0]&&  0x06  == kb_event.keycode[1]))
		{
			rf_set_power_level_index(RF_POWER_m24dBm);
			kb_status.kb_mode  = STATE_PAIRING;
		}
	}
#endif

	if(kb_status.kb_mode <= STATE_PAIRING){
		kb_paring_and_syncing_proc();
	}
	bat_low_det = kb_batt_det_process();
//	p_task_when_rf = kb_task_when_rf;
//	kb_rf_proc(key_scaned,bat_low_det);
	kb_rf_proc(key_scaned,0);
#if ZZCZ
	if(kb_status.kb_mode == STATE_PAIRING)
	{
		gpio_write( GPIO_GP30, 1);
		sleep_us(6000);
		gpio_write( GPIO_GP30, 0);
	}
#endif
	kb_pm_proc();
//	}

	kb_status.loop_cnt++;
}



