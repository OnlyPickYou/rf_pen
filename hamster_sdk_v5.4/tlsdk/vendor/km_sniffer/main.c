
#include "../../proj/tl_common.h"

#if(!__PROJECT_PM_TEST__)

#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"

#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"

extern void user_init();
extern void proc_button_ahead();
extern void irq_km_timer1 (void);

_attribute_ram_code_ void irq_handler(void)
{
	u32 src = reg_irq_src;
	if(src & FLD_IRQ_TMR1_EN){
		irq_km_timer1();
		reg_tmr_sta = FLD_TMR_STA_TMR1;
	}

	u8  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_ll_rx();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		irq_ll_tx();
	}
}

int main (void) {

	cpu_wakeup_init();

	//while (1){write_reg8 (0x800300, 0);};
	clock_init();

//	dma_init();

//	gpio_init();

	//irq_init();
	//reg_irq_mask = ((IRQ_TIMER1_ENABLE?FLD_IRQ_TMR1_EN:0) | (IRQ_GPIO_ENABLE?FLD_IRQ_GPIO_RISC0_EN:0)
		//	| (IRQ_RF_RTX_ENABLE ? FLD_IRQ_ZB_RT_EN : 0) | (MODULE_ETH_ENABLE ? FLD_IRQ_SBC_MAC_EN : 0));

	reg_irq_mask = FLD_IRQ_ZB_RT_EN;

	rf_drv_init(XTAL_12M_RF_2m_MODE);

    user_init ();

    irq_enable();

	while (1) {
		main_loop ();
	}
}

#endif


