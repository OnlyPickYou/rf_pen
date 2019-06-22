#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"

#if (__PROJECT_8267_REMOTE__)

////////////////////////////////////////////////////////////////////////
//	mouse data management
////////////////////////////////////////////////////////////////////////
mouse_data_t     km_data[MOUSE_FRAME_DATA_NUM];
mouse_data_t     mouse_data;

int	km_wptr = 0;
int	km_rptr = 0;

void km_data_add (u32 * s, int len)
{
	memcpy4 ((u32 *)&km_data[km_wptr&3], s, len);
	km_wptr = (km_wptr + 1) & 7;
	if ( ((km_wptr - km_rptr) & 7) > 4 ) {	//overwrite older data
		km_rptr = (km_wptr - 4) & 7;
	}
}


int km_data_get (u8 *p)
{
	p[0] = 0;
	for (int i=0; km_rptr != km_wptr && i<4; i++) {
		memcpy ((void *) p + 1 + sizeof(mouse_data_t) * p[0]++,
				(void *) &km_data[km_rptr & 3],
				sizeof(mouse_data_t));
		km_rptr = (km_rptr + 1) & 7;
	}
	return p[0];
}

int	proc_mouse (u8 button)
{
	static u32 fno;
	static u32	dtick_am, tick_button;
	static u8 button_last;
	static u8 mask = 0;

	if (button_last != button)
	{
		tick_button = clock_time ();
		mask = 1;
		button_last = button;
	}
	if (clock_time_exceed (tick_button, 200000))
	{
		mask = 0;
	}

	u32 t = clock_time ();
	if (1) {
		#if 1
				airmouse_getxy ((mouse_data_t *) &mouse_data);
				mouse_data.btn = button;
		#else
				mouse_data.x = fno & BIT(7) ? 6 : -6;
				mouse_data.y = fno & BIT(6) ? -6 : 6;
		#endif

				fno++;
	}
	if (mask)
	{
		mouse_data.x = mouse_data.y = 0;
	}

	int valid = *(u32*) &mouse_data;
	if (1 || valid)
	{
		km_data_add ((u32 *)&mouse_data, sizeof (mouse_data_t));
	}
	dtick_am = clock_time () - t;
	return 1 || valid;
}



#endif  //end of __PROJECT_8267_REMOTE__
