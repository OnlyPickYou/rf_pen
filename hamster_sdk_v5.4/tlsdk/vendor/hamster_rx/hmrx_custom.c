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

#include "hmrx_custom.h"
#include "hmrx_usb.h"

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

u32		custom_binding_device_id = U32_MAX;

int     rf_paring_enable;

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
		p_custom_cfg = (custom_cfg_t *) (DEVICE_CUSTOMIZATION_INFO_ADDR+(i*0x10));  //3f00
		if(p_custom_cfg->cap != 0){
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
	if(custom_id != U16_MAX){
		u32 code = rf_access_code_16to32(custom_id);
		rf_set_access_code1 (code);
	}


	u8 dongle_custom_cap              = p_custom_cfg->cap;
    if ( p_custom_cfg->cap != U8_MAX )
        cap_internal_adjust( p_custom_cfg->cap );

	rf_paring_enable       = 1;

#if(USB_DESCRIPTER_CONFIGURATION_FOR_KM_DONGLE)  //for km dongle customization (add by sihui)
	get_usb_descripter_configuration();
#endif

#if(USB_ID_AND_STRING_CUSTOM)
	get_usb_id_and_string(p_custom_cfg->vendor_id,p_custom_cfg->prodct_id);
#endif
}

/****************************************************************************************************************
     device_program_on
     device_program
     device_program_off
 ***************************************************************************************************************/

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
	WaitUs (1000);

	write_reg8(0x800643,irqst);
}
