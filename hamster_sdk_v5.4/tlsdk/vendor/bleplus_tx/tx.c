#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"
#include "../common/device_power.h"

#include "../link_layer/rf_ll.h"

#include "tx_emi.h"
#include "tx_rf.h"
#include "tx_custom.h"
#include "tx_info.h"
#include "tx_batt.h"
#include "tx_pm.h"
#include "tx_led.h"
#include "tx_test.h"
#include "tx_keyscan.h"

#if (__PROJECT_TX_8267__)

kb_status_t  kb_status;


_attribute_ram_code_ void irq_handler(void)
{

	u32 src = reg_irq_src;

	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_tx_device_rx();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		//irq_tx_device_tx();
		reg_rf_irq_status = FLD_RF_IRQ_TX;
	}
}

void  user_init(void)
{
#if 0
	tx_test();
#else

	kb_status.no_ack = 1;
	kb_custom_init();
	kb_info_load();  //load info from
	kb_rf_init();

	tx_device_led_init(GPIO_LED,1);
	tx_device_led_setup(tx_led_cfg[LED_POWER_ON]);

	kb_pm_init();

#endif
}


u32 key_scaned;
u32 mainLoop_update_tick;
void main_loop(void)
{
	mainLoop_update_tick = clock_time();

	key_scaned = keyboard_scan_key (0, !km_dat_sending);

	if(kb_status.kb_mode == STATE_EMI){
		kb_emi_process();
	}
	else{
		if(kb_status.kb_mode <= STATE_PAIRING){
			kb_paring_and_syncing_proc();
		}

		kb_rf_proc(key_scaned);

		tx_device_led_process();
		//kb_pm_proc();
		cpu_sleep_wakeup(0, PM_WAKEUP_TIMER ,mainLoop_update_tick + 12*CLOCK_SYS_CLOCK_1MS);
	}
	kb_status.loop_cnt++;

}










#endif  //end of __PROJECT_TX_8267__
