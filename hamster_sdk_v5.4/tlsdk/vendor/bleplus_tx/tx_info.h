/*
 * kb_info.h
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#ifndef KB_INFO_H_
#define KB_INFO_H_


typedef struct{
	u32 dongle_id;
	u8	rsv;
    u8	channel;
	u8	mode;
	u8	poweron;
} kb_info_t;

extern void kb_info_load(void);
extern void kb_info_save(void);

#endif /* KB_INFO_H_ */
