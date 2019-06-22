#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/keyboard.h"


#if (__PROJECT_8267_REMOTE__)

#define				ACTIVE_INTERVAL					24000			//24ms

#define				rega_sno						0x34

/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
extern kb_data_t	kb_event;

extern int		sys_mic;

u32		tick_key_pressed;

int proc_keyboard (int read)
{


	u32 t = clock_time ();

	kb_event.keycode[0] = 0;
	kb_event.keycode[1] = 0;

	int det_key = kb_scan_key (0, read);
	///////////////////////////////////////////////////////////////////////////////////////
	//			key pressed or released
	///////////////////////////////////////////////////////////////////////////////////////
	if (det_key) 	{

		tick_key_pressed = clock_time ();

		/////////////////////////// key pressed  /////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////////////

		if (kb_event.cnt == 1 && kb_event.keycode[0] == VK_M) {
			if(!sys_mic){

				config_adc (FLD_ADC_PGA_C45, FLD_ADC_CHN_D0, SYS_16M_AMIC_16K);
				sys_enable_mic (1);
			}
			det_key = 0;
		}
		else {  //key released
			if(sys_mic){
				sys_enable_mic (0);
			}
		}

	}

	return det_key;
}




#endif  //end of __PROJECT_8267_REMOTE__
