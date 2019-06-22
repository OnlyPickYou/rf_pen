/*
 * dongle_custom.c
 *
 *  Created on: 2014-3-10
 *      Author: sihui
 */

#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"

#include "vacuum_receiver_custom.h"

void (*device_program) (int adr, unsigned char id);
void (*device_program_on) (void);
void (*device_program_off) (void);

void otp_device_program_on (void);
void otp_device_program_off (void);
void otp_device_program(int , unsigned char);

void flash_device_program_on (void);
void flash_device_program_off (void);
void flash_device_program(int , unsigned char);


custom_cfg_t   *p_custom_cfg;

u32     rf_paring_tick = 0;
u32		custom_binding = 0xffffffff;


s8 		custom_auto_paring_rssi;
u8      dongle_custom_cap;

int		custom_binding_idx;

int     keyboard_paring_enable;
int     mouse_paring_enable;

int     auto_paring_enable;


u8 		receiver_emi_test_en;

u8      channel_mask_custom;

void dongle_emi_cust_init( void ){
    write_reg8( 0x800598, read_reg8(0x800598) & 0xf0 ); //8366 disable mspi IO drive-strenth to satisfy 192M
}

/****************************************************************************************************************
     custom_init
 ***************************************************************************************************************/
void	custom_init (void)
{
#if(CUSTOM_DATA_ERR_FIX_ENABLE)
	for(int i=0;i<CUSTOM_DATA_MAX_COUNT;i++){
		p_custom_cfg = (custom_cfg_t *) (DEVICE_CUSTOMIZATION_INFO_ADDR+(i*22));  //3f00
		if(p_custom_cfg->paring_limit_t != 0){
			break;
		}
	}
#else
	p_custom_cfg = (custom_cfg_t *)(DEVICE_CUSTOMIZATION_INFO_ADDR);  //3f00
#endif

	u16 custom_id;
	custom_id                      = p_custom_cfg->vid;
	if(custom_id != U16_MAX){
		rf_set_access_code0 (rf_access_code_16to32(custom_id));
	}
	custom_id                      = p_custom_cfg->id;
	u32 code;
	if(custom_id != U16_MAX){
		code = rf_access_code_16to32(custom_id);
		rf_set_access_code1 (code);
	}
	else{
		code = rf_access_code_16to32(0x5665);
		rf_set_access_code1 (code);
	}

    u8 pair_type =  ((p_custom_cfg->paring_type == U8_MAX) ? 0xaf : p_custom_cfg->paring_type);

	auto_paring_enable             = !(pair_type & CUSTOM_DONGLE_AUTO_PARING);


	receiver_emi_test_en 		   = (p_custom_cfg->emi_test_en != U8_MAX);
	channel_mask_custom            = p_custom_cfg->channal_msk;


	keyboard_paring_enable = 0;
	mouse_paring_enable    = 0;

    if ( p_custom_cfg->emi_ini_patch != U8_MAX )
        pf_emi_cust_init = dongle_emi_cust_init;

}

#if DONGLE_ID_IN_FW

/****************************************************************************************************************
     device_program_on
     device_program
     device_program_off
 ***************************************************************************************************************/

#if (0)

//////////////////////////////////////////////////////
//copy from  dut otp code
//////////////////////////////////////////////////////
void otp_device_program_on (void){
	/* open vpp 6.75V */
	unsigned char irqst = read_reg8(0x800643);
	write_reg8(0x800643,0);

	write_reg8(0x800071, 0x13);;    // set DCDC 6.75 clk to 60M
	sleep_us(100);
	analog_write(0x85, 0x14);
	sleep_us(1000);   //wait at least 1000us
	analog_write(0x85, 0x54);
	sleep_us(100);

	write_reg8(0x800643,irqst);
}

void otp_device_program_off(void)
{
	analog_write(0x85, 0x0c);
	write_reg8(0x800071, 0x03);
}


_attribute_ram_code_ void otp_device_program(int addr, unsigned char value){
	unsigned char irqst = read_reg8(0x800643);
	write_reg8(0x800643,0);

	write_reg8(0x800012, 0x7e);
	write_reg16(0x800010, addr);
	write_reg8(0x80001a, 0x02);
	write_reg8(0x800013, value);
	WaitUs (100);
	write_reg8(0x800013, value);
	write_reg8(0x80001a, 0x0);
	WaitUs (20);

	write_reg8(0x800643,irqst);
}

//#else

void flash_device_program_on (void){}
void flash_device_program_off(void){}
//flash program to emulate OTP
_attribute_ram_code_ void flash_device_program (int adr, unsigned char id)
{
	unsigned char irqst = read_reg8(0x800643);
	write_reg8(0x800643,0);
	write_reg8 (0x80000d, 0x00);
	write_reg8 (0x80000c, 0x06);	//write enable
	while (read_reg8(0x80000d));
	write_reg8 (0x80000d, 0x01);

	WaitUs (1);

	write_reg8 (0x80000d, 0x00);
	write_reg8 (0x80000c, 0x02);	//write command
	while (read_reg8(0x80000d));

	write_reg8 (0x80000c, adr>>16);
	while (read_reg8(0x80000d));

	write_reg8 (0x80000c, adr>>8);
	while (read_reg8(0x80000d));

	write_reg8 (0x80000c, adr);
	while (read_reg8(0x80000d));

	write_reg8 (0x80000c, id);
	while (read_reg8(0x80000d));

	write_reg8 (0x80000d, 0x01);
#if 0
	write_reg8 (0x80000d, 0x00);
	write_reg8 (0x80000c, 0x05);	//read status
	while (read_reg8(0x80000d));
	int i;
	for (i=0; i<1000000; i++)
	{
		write_reg8 (0x80000c, 0x00);	//launch 8-cycle to trigger read
		while (read_reg8(0x80000d));
		if ( !(read_reg8(0x80000c) & 1) )
			       break;
	}
	write_reg8 (0x80000d, 0x01);
#else
	WaitUs (1000);
#endif
	write_reg8(0x800643,irqst);
}
#endif



//#endif
#endif



