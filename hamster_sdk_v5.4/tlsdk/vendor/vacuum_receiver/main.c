
#include "../../proj/tl_common.h"

#if(!__PROJECT_PM_TEST__)

#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "vacuum_receiver_iouart.h"

#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"

extern void usb_init();
extern void user_init();
extern void main_loop();
extern void proc_button_ahead();

//unsigned char bug_test[8];
//unsigned char bug_cc;
//int b_test;

//extern u8 *pkt_ack;
extern u8 receiver_emi_test_en;
int main (void) {

	cpu_wakeup_init();

	//usb_dp_pullup_en (0);

	clock_init();

	dma_init();

	gpio_init();

	irq_init();

	//usb_init();

	rf_drv_init(0);

    user_init ();

    if(!receiver_emi_test_en){
    	irq_enable();
    }

	while (1) {
#if(MODULE_WATCHDOG_ENABLE)
		wd_clear();
#endif

		main_loop ();
	}
}


#endif

