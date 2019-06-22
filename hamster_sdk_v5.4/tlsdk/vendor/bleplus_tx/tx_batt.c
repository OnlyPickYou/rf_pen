/*
 * kb_batt.c
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#include "../../proj/tl_common.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/rf_drv.h"
#include "../common/rf_frame.h"

#include "tx_emi.h"
#include "tx_rf.h"
#include "tx_custom.h"
#include "tx_info.h"
#include "tx_batt.h"
#include "tx_pm.h"
#include "tx_led.h"
#include "tx_test.h"

#if (__PROJECT_TX_8267__)

u32 batt_det_count = 0;


#define ADC_SAMPLE_NUM   			11
u16 adc_sample[ADC_SAMPLE_NUM+1];
u16 adc_result;

void kb_batt_det_init(void)
{

}

extern u32	scan_pin_need;

//_attribute_ram_code_ void
kb_batt_det_process(void)
{
}



#endif  //end of __PROJECT_TX_8267__
