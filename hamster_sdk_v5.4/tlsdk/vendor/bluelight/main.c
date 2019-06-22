
#include "../../proj/tl_common.h"

#if(1)

#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"

extern void user_init();

_attribute_ram_code_ void irq_handler(void)
{
#if 0
	u32 src = reg_irq_src;
	if(src & FLD_IRQ_TMR1_EN){
		irq_host_timer1();
		reg_tmr_sta = FLD_TMR_STA_TMR1;
	}
#endif

}


int main (void) {

	cpu_wakeup_init();

	clock_init();

//	dma_init();

//	gpio_init();
//	while (1);
	//irq_init();
	//reg_irq_mask = ((IRQ_TIMER1_ENABLE?FLD_IRQ_TMR1_EN:0) | (IRQ_GPIO_ENABLE?FLD_IRQ_GPIO_RISC0_EN:0)
		//	| (IRQ_RF_RTX_ENABLE ? FLD_IRQ_ZB_RT_EN : 0) | (MODULE_ETH_ENABLE ? FLD_IRQ_SBC_MAC_EN : 0));

	reg_irq_mask = FLD_IRQ_ZB_RT_EN;
	rf_drv_init(0);

    user_init ();

    irq_enable();

	while (1) {
		main_loop ();
	}
}

#endif


