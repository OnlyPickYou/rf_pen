/*
 * dongle_custom.h
 *
 *  Created on: 2014-3-10
 *      Author: hp
 */

#ifndef DONGLE_CUSTOM_H_
#define DONGLE_CUSTOM_H_

#ifndef DONGLE_CUS_EN
#define DONGLE_CUS_EN		1
#endif

#ifndef	DONGLE_ID_IN_FW
#define DONGLE_ID_IN_FW		1
#endif

#include "../common/user_config.h"

void      custom_init (void);
#define 	RECV_PKT_RSSI(p)                (((u8 *)p)[4])


/********************************************************
paring_type customization
addr :  0x3f05
default  0xff:  auto			   1111 1111	->0xaf
         0xcf:  soft        	   1100 1111
         0xa7:  auto_m2			   1010 0111
		 0xfb:	manual             1111 1011
         0x6f:  golden dongle	   0110 1111

 *******************************************************/
#define		CUSTOM_DONGLE_GOLDEN			BIT(7)     // 0 for golden dongle
#define		CUSTOM_DONGLE_AUTO_PARING		BIT(6)     // 0 for  auto paring
#define		CUSTOM_DONGLE_SOFT_PARING		BIT(5)     // 0 for software paring

#define		CUSTOM_DONGLE_AUTO_PARING_M2	BIT(3)     // 0 for  munaul  paring
#define		CUSTOM_DONGLE_MANNUAL_PARING	BIT(2)     // 0 for  munaul  paring

/********************************************************
support_type customization
addr  :  0x3f07
default  0xff  :  mouse only
         other :  mouse/keyboard kit
 *******************************************************/


/***********************************************************************************
vendor  string : addr 0x3de0   max_len:22 characters   default : Telink
product string : addr 0x3e40   max_len:22 characters   default : Wireless Receiver
serial  string : addr 0x3ea0   max_len:22 characters   default : TLSR8366
 **********************************************************************************/
#define     VENDOR_STRING_ADDR              0x3de0
#define     PRODCT_STRING_ADDR              0x3e40
#define     SERIAL_STRING_ADDR              0x3ea0
#define     DEVICE_PARING_INFO_ADDR         (DEVICE_CUSTOMIZATION_INFO_ADDR + 0x30)     //配对信息地址


/***********************************************************************************
OTP 中定制信息写错，修复机制开启  ：CUSTOM_DATA_ERR_FIX_ENABLE
		          最多可写定制信息次数 ：CUSTOM_DATA_MAX_COUNT
 **********************************************************************************/
#define		CUSTOM_DATA_ERR_FIX_ENABLE      1
#define		CUSTOM_DATA_MAX_COUNT		    2

//cust_crystal_12M: the first custom info 
//#define		cust_crystal_12M	( (*(unsigned char*) (DEVICE_CUSTOMIZATION_INFO_ADDR - 1)) == 0 )    //( custom_cfg_t->crystal_12M )
typedef struct{
	u16  vid;		  		  // 00~0x01 vendor id
	u16  id;		  		  // 02~0x03 device id
	u16  vendor_id;        //
	u16  prodct_id;        //
	u8	 cap;	           //
	u8   memory_type;      //
} custom_cfg_t;

extern custom_cfg_t   *p_custom_cfg;

#endif /* DONGLE_CUSTOM_H_ */
