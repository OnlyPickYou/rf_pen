/*
 * mouse_type.h
 *
 *  Created on: Feb 10, 2014
 *      Author: xuzhen
 */

#ifndef MOUSE_TYPE_H_
#define MOUSE_TYPE_H_
#include "..\..\proj\common\types.h"
#include "device_led.h"

#define MOUSE_FRAME_DATA_NUM   4

typedef struct {
	u8 btn;
	s8 x;
	s8 y;
	s8 wheel;
	//s8 tl_wheel;
	//u8 hotkey;
}mouse_data_t;				//remote controller




#endif /* MOUSE_TYPE_H_ */
