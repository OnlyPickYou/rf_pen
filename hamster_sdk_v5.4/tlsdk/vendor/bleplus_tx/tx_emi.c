/*
 * kb_emi.c
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */


#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"

#include "tx_emi.h"
#include "tx_rf.h"
#include "tx_custom.h"
#include "tx_info.h"
#include "tx_batt.h"
#include "tx_pm.h"
#include "tx_led.h"
#include "tx_test.h"


#if  (__PROJECT_TX_8267__)

void kb_emi_process(void)
{
}

#endif  //end of __PROJECT_TX_8267__
