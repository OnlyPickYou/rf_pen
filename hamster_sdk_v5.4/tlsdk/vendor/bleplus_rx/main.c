
#include "../../proj/tl_common.h"

#if(__PROJECT_RX_8267__)

#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"

#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"

extern void user_init();

int main (void) {

	write_reg8(0x808000,0x11);
	cpu_wakeup_init();

	clock_init();

	dma_init();

	gpio_init();

	irq_init();

	rf_drv_init(0);

    user_init ();

    irq_enable();

	while (1) {
#if(MODULE_WATCHDOG_ENABLE)
		wd_clear();
#endif
		main_loop ();
	}
}

#endif


