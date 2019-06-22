#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../common/emi.h"
#include "../common/device_power.h"

#include "vacuum_device_info.h"
#include "vacuum_controller.h"
#include "vacuum_controller_rf.h"
//#include "vacuum_controller_button.h"
#include "vacuum_controller_batt.h"
#include "vacuum_controller_button.h"
#include "vacuum_controller_rf.h"
#include "vacuum_controller_emi.h"
//#include "vacuum_controller_custom.h"
//#include "vacuum_controller_iouart.h"

#include "trace.h"

u8 first_poweron;
extern u8 vacuum_deepsleep_cnt;
int custom_binding_idx;
u32 vc_btn[3] = {BTN_UP, BTN_POWER, BTN_DOWN};		//GP10,GP7,GP8
#if (STORE_2BYTE_ID)
u16	custom_binding = 0xffff;		//receiver id
#else
u32	custom_binding = 0xffffffff;		//receiver id
#endif

u8	*p_vc_level	= (u8 *)(CUSTOM_CONTROLLER_ADDR);
vc_cfg_t *p_vc_cfg = (vc_cfg_t *)(DEVICE_ID_ADDRESS);

vc_data_t	 vc_event;

#if(WITH_SPELL_PACKET_EN)
vc_data_t	 vc_s_event;
#endif

vc_status_t  vc_status;

extern u8* kb_rf_pkt;

extern vcEvt_buf_t  vcEvt_buf;
extern btn_status_t btn_status;

#if(WITH_SPELL_PACKET_EN)
extern rf_packet_vacuum_t	pkt_km;
#else
extern rf_packet_keyboard_t	pkt_km;
#endif


extern repair_and_reset_t repair_and_reset;


/*********************************************************************
     device_program_on
     device_program
     device_program_off
 *******************************************************************/
void (*device_program) (int adr, unsigned char id);
u8 (*device_program_read) (u16 addr);
void (*device_program_on) (void);
int  (*device_power_on_check)(void);
void (*device_program_off) (void);


void otp_clock_init(void){

	write_reg8(0x800071, 0x1f);// open DCDC 6.75 clk to 8M
	sleep_us(300);
	write_reg8(0x800071, 0x13);// set DCDC 6.75 clk to 60M
	sleep_us(300);
}

void reg_poweron_dcdc(void)
{
	analog_write(0x85, 0x14);
	sleep_us(1000);
	analog_write(0x85, 0x54);
}

//////////////////////////////////////////////////////
//copy from  dut otp code
//////////////////////////////////////////////////////
void otp_device_program_on (void){
	/* open vpp 6.75V */
	unsigned char irqst = read_reg8(0x800643);

	write_reg8(0x800643,0);

	otp_clock_init();
	sleep_us(1000);
	reg_poweron_dcdc();
	sleep_us(100);
#if 0
	write_reg8(0x800071, 0x13);    // set DCDC 6.75 clk to 60M
	sleep_us(100);
	analog_write(0x85, 0x14);
	sleep_us(1000);   //wait at least 1000us
	analog_write(0x85, 0x54);
	sleep_us(100);
#endif

	write_reg8(0x800643,irqst);
}

void otp_device_program_off(void)
{
	analog_write(0x85, 0x0c);

	write_reg8(0x800071, 0x03);
}

/******************************************************************
 *			          vpp_power_on_check
 *****************************************************************/
int otp_power_on_check(void)
{
//	otp_power_on();
	u8 count;
	for(count=0; count < 3; count++)
	{
		analog_write(0x85, 0x0c);   //OTP_VPP   normal
		sleep_us(1000);
		otp_device_program_on();		//OTP_VPP   dcdc and clock open
		sleep_us(10000);
		if(analog_read(0x89) & 0x02)
			break;
	}
	if(count < 3)
		return 1;   //VPP successful
	else
		return  0;  //VPP fail
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

_attribute_ram_code_ u8 otp_device_read(u16 addr){
	volatile u8 value;
	addr = ((addr-4) & 0x3fff);
	U8_SET(0x800012, 0x7e);   //core12_<1> OTP  fast clk
	reg_otp_addr_para = addr;

	U8_SET(0x80001a, 0x00); //normal read

	value = reg_otp_byte_dat;
	sleep_us(15);
	value = U8_GET(0x800019);
	sleep_us(15);
	return U8_GET(0x800019);
}


_attribute_ram_code_ flash_device_program (int adr, unsigned char id)
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


u8 flash_device_read(u16 addr){

	u8 value;

	value = *(volatile u8 *)(addr+1024);
	value = *(volatile u8 *)(addr+2048);
	value = *(volatile u8 *)addr;

	return value;
}


u8 saveId_err = 0;
/****************************************************************************************************************
     set_device_id_in_firmware
     load paired device id into flash and ram
 ***************************************************************************************************************/
int set_device_id_in_firmware (u32 id)
{
	u16 check;
	u16 device_id = 0;

	saveId_err = 0;

	if (custom_binding == id)
	{
		return 0;
	}
	else if (custom_binding_idx < MAX_PARING_TIMES)
	{
		device_id = rf_access_code_32to16(id);

		if( device_power_on_check() ){
			u32 adr = (PARING_START_ADDR + 2 * custom_binding_idx);

			device_program (adr, device_id&0xff);
			device_program (adr + 1, device_id >> 8);

			check = ( device_program_read(adr) | (device_program_read(adr+1)<<8) ) ;
			if(check != device_id){
				for(int j=0;j<3;j++){
					device_program (adr,device_id);
					device_program (adr + 1,device_id >> 8);
				}

				check = ( device_program_read(adr) | (device_program_read(adr+1)<<8) ) ;
				if( check != device_id ){
					saveId_err = 2;
				}
			}
		}
		else{
			saveId_err = 1;
		}

		device_program_off ();

		if(saveId_err){  //err
			return 0;
		}
		else{  //OK
			custom_binding = device_id;
			custom_binding_idx ++;
		}
	}

	return 0;
}

#if(FACTORY_PRODUCT_TEST_EN)

test_led_t test_led = {
		0,
		0,
		0,
		0xff,

		500,		// 500ms, turn on led
		500,		// 500ms, turn off led
		0,


};

void test_device_led_on_off(u8 on)
{
	gpio_write(GPIO_LED, on^test_led.polar);
	gpio_set_output_en(GPIO_LED, on);
	test_led.isOn = on;
}

void test_device_led_init(u8 polarity) // polarity : 1 for high level led on, 0 for low level led on
{
	test_led.polar = !polarity;
	gpio_set_func(GPIO_LED, AS_GPIO);
	gpio_set_input_en(GPIO_LED, 0);
	gpio_set_output_en(GPIO_LED, 0);

	test_device_led_on_off(test_led.OnTimes_ms ? 1: 0);
}

void test_device_led_proc(void)
{

	if(!test_led.isOn){
		if( ((test_led.StartCnt++) % 100) < 50 ){
			test_device_led_on_off(1);
		}
	}
	else{
		if( ((test_led.StartCnt++ ) % 100) >= 50 )
		test_device_led_on_off(0);
	}

}

void task_keycode_sending_proc()
{
	if(vc_event.keycode[0] == VC_POWER && vc_event.cnt == 1)
		test_led.keystatus_cnt ++;

	if(test_led.keystatus_cnt & 1){

		vc_event.cnt = 1;
		btn_status.btn_new  = 1;
		vc_event.keycode[0] = TEST_POWERDOWN;
		test_device_led_proc();
	}
	else{
		vc_event.cnt = 0;
		btn_status.btn_new = 0;
		test_device_led_on_off(0);		// turn off led
	}

}

#endif

vc_slp_cfg_t vc_sleep = {
	SLEEP_MODE_BASIC_SUSPEND,  	//mode
	0,                  		//device_busy
	0,							//quick_sleep
	0,                  		//wakeup_src

    0,							//cnt_basic_suspend
    4000/KB_MAIN_LOOP_TIME_MS,  //4000ms enter deep sleep

    0,
    0,
    0,
};

batt_info_t batt_info = {

		STATE_HIGH_BATT,			//battery status
#if(WITH_NEW_DETECT_BATT_FUNC)
		0x00,	0x00,	0x00,		//battery value
#else
		0x64,	0x64,	0x64,		//battery value
#endif
};

key_repeat_t key_repeat = {
		0,
		0,
		0,

};


_attribute_ram_code_ void irq_handler(void)
{
	u16  src_rf = reg_rf_irq_status;
	u32  src = reg_irq_src;

	if(src_rf & FLD_RF_IRQ_RX){
		irq_device_rx();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		//irq_device_tx();
		reg_rf_irq_status = FLD_RF_IRQ_TX;
	}


}

void vc_config_init(void)
{

#if	CONTROLLER_ID_IN_FW							//store receiver's id in otp or flash
	if(p_vc_cfg->memory_type == U8_MAX){		//flash -> for debug
		device_program = otp_device_program;
		device_program_read = otp_device_read;
		device_program_on = otp_device_program_on;
		device_power_on_check = otp_power_on_check;
		device_program_off = otp_device_program_off;
	}
	else{
		device_program = flash_device_program;
		device_program_read = flash_device_read;
		device_program_on = flash_device_program_on;
		device_power_on_check = flash_power_on_check;
		device_program_off = flash_device_program_off;
	}
#endif

    if(p_vc_cfg->pipe_pairing != U16_MAX){
		rf_set_access_code0 (rf_access_code_16to32(p_vc_cfg->pipe_pairing));
	}

	if (p_vc_cfg->did != U32_MAX) {
		pkt_pairing.did = p_vc_cfg->did;
#if(USE_CURRENT_VERSION_1P6)
		pkt_km.did = p_vc_cfg->did;
#endif

	}

	vc_status.tx_power = RF_POWER_2dBm;
	vc_status.cust_tx_power = (p_vc_cfg->tx_power == 0xff) ? RF_POWER_8dBm : p_vc_cfg->tx_power;

	vc_status.golden_tx_en = (*p_vc_level == 0xab);


// use for double key function. Customize the delay time
	extern u8 DelayT;
	if(p_vc_cfg->k_delay != 0xff){
		DelayT = p_vc_cfg->k_delay;
	}

	if(vc_status.golden_tx_en){

		vc_status.mode_link = LINK_RCV_DONGLE_DATA;
		vc_status.vc_mode = STATE_TEST;
	}
	else{

		u16 *p = (u16 *)(PARING_START_ADDR);

		for (custom_binding_idx=0; custom_binding_idx<MAX_PARING_TIMES; custom_binding_idx++) {
			if (*p == 0xffff){
				break;
			}
			custom_binding = p[0];
			p += 1;
		}

		u8 id_stored = 0;

		if(custom_binding == U16_MAX || custom_binding == 0){		//auto pairing

			vc_status.mode_link = (analog_read(PM_REG_START+4) & 0x80) >> 7;
#if (MANUAL_PAIR_EN)
			repair_and_reset.repair_cnt = analog_read(PM_REG_START+4) & 0x7f;		//BIT(7):mode_link
#endif
			if(vc_status.mode_link&LINK_PIPE_CODE_OK){
				device_info_load();
			}
			else{
				vc_status.dongle_id = 0;
				vc_status.vc_mode = STATE_PAIRING;
			}

		}
		else{
			//paired ok, transmit data correctly
			id_stored = 1;
			vc_status.mode_link = LINK_PIPE_CODE_OK;
			vc_status.dongle_id = rf_access_code_16to32(custom_binding);
#if(USE_CURRENT_VERSION_1P6)
			pkt_km.type = FRAME_TYPE_VACUUM_CONF;
#endif
			vc_status.vc_mode = STATE_NORMAL;
			repair_and_reset.auto_pair = 0;
		}

		u8 btn_press = 0;
		for(int j=0; j<MAX_BTN_SIZE; j++){
			if(BTN_VALID_LEVEL != !gpio_read(vc_btn[j])){
				btn_press |= BIT(j);
			}
		}
		first_poweron =  analog_read(PM_REG_END);


		if((btn_press & 0xff) == btn_ui.factory_reset_ui && !first_poweron && id_stored){				//power be pressed
			repair_and_reset.poweron_reset = 1;
		}

		if((btn_press & 0xff) == btn_ui.emi_ui && !first_poweron ){		//up, power were pressed, EMI mode
			vc_status.vc_mode = STATE_EMI;
			vc_status.mode_link = LINK_PIPE_CODE_OK;
		}

		pkt_km.seq_no = (analog_read(PM_REG_START + 5) & 0x0f);		//PM_REG_START + 5
		if( !analog_read(PM_REG_END) ){
			analog_write(PM_REG_END, 1);
		}
		else{
			u8 ana_reg1f = analog_read(PM_REG_END) + 1;
			analog_write(PM_REG_END, ana_reg1f);
		}

	}

}


//_attribute_ram_code_
void vc_slp_mode_machine(vc_slp_cfg_t *s_cfg)
{
    if ( s_cfg->device_busy ){
        s_cfg->mode = SLEEP_MODE_BASIC_SUSPEND;
        s_cfg->cnt_basic_suspend = 0;
    }
    else if (s_cfg->mode == SLEEP_MODE_BASIC_SUSPEND){
        s_cfg->cnt_basic_suspend ++;
        if ( s_cfg->cnt_basic_suspend >= s_cfg->thresh_basic_suspend ){
        	s_cfg->mode = SLEEP_MODE_DEEPSLEEP;
        }
    }

	if ( s_cfg->quick_sleep ) {
        s_cfg->mode = SLEEP_MODE_DEEPSLEEP;
	}
}
#if 0
_attribute_ram_code_
#endif
void vc_pm_proc(void)
{

    //rf event or button not be released -> device busy
	vc_sleep.device_busy = ( vc_status.rf_sending || btn_status.btn_not_release );

	if(vc_status.golden_tx_en && vc_status.loop_cnt <  KB_NO_QUICK_SLEEP_CNT){
		vc_sleep.quick_sleep = 0;
	}
	else{
		vc_sleep.quick_sleep = HOST_NO_LINK;
	}
	if ( vc_status.vc_mode <= STATE_PAIRING && vc_status.loop_cnt < KB_NO_QUICK_SLEEP_CNT){
		vc_sleep.quick_sleep = 0; //在pairing状态下进quick_sleep，取消掉
	}

	vc_slp_mode_machine( &vc_sleep );


	vc_sleep.wakeup_src = PM_WAKEUP_TIMER;
	vc_sleep.wakeup_ms = KB_MAIN_LOOP_TIME_MS;

	if(vc_status.loop_cnt < MAINLOOP_8MS_DURING_CNT){			//500ms之内mian_loop时间是8ms
		vc_status.tx_retry = 5;
		vc_sleep.wakeup_ms = 8;
	}
	else{
		vc_status.tx_retry = 3;
	}

	if ( vc_sleep.mode == SLEEP_MODE_DEEPSLEEP){
		device_info_save();
//		for(int i=0; i<MAX_BTN_SIZE; i++){
//			gpio_setup_up_down_resistor(vc_btn[i], PM_PIN_PULLUP_10K);		//10K PULL UP resistor
//		}
		vc_sleep.wakeup_src = PM_WAKEUP_PAD;
		vc_sleep.wakeup_ms = 0;
	}

	cpu_sleep_wakeup_rc(vc_sleep.mode == SLEEP_MODE_DEEPSLEEP, vc_sleep.wakeup_src, vc_sleep.wakeup_ms);
	vc_sleep.wakeup_tick = clock_time();

}


void gpio_wakeup_init(void)
{
	///////////  设置唤醒suspned,对应唤醒源类型为CORE唤醒  //////////////////////
	reg_gpio_wakeup_en |= FLD_GPIO_WAKEUP_EN;	//gpio wakeup enable
	reg_wakeup_en = FLD_WAKEUP_SRC_GPIO;        //gpio wakeup as core enbable

#if DBG_GPIO_EN
	gpio_set_output_en(GPIO_GP6, 1);
	gpio_set_input_en(GPIO_GP6, 0);
#endif

	for(int i=0; i<MAX_BTN_SIZE; i++){
	    gpio_set_output_en(vc_btn[i],0);			//output disable
	    gpio_set_input_en(vc_btn[i],1);			//input mode enable

	    gpio_setup_up_down_resistor(vc_btn[i], PM_PIN_PULLUP_1M);		//1M PULL UP resistor

	    gpio_enable_wakeup_pin(vc_btn[i],0,1);	//0：low level wakeup； 1：wakeup enable
	    cpu_set_gpio_wakeup(vc_btn[i],0,1);		//0：low level wakeup； 1：wakeup enable
	}

	vc_sleep.wakeup_tick = clock_time();

}


void back_from_deepsleep_proc(u8 en, int t_ms)
{
	if(en){
		//u32 tick  =  clock_time();
		//while(!clock_time_exceed(tick, t_ms * CLOCK_SYS_CLOCK_1MS));
		sleep_us(t_ms * 1000);
	}
}

extern u8 *pkt_ack;

void  user_init(void)
{
	//usb_log_init(); //usb

	//swire2usb_init();  //swire

	gpio_wakeup_init();

	vc_config_init();

	vc_rf_init();
#if(FACTORY_PRODUCT_TEST_EN)
	test_device_led_init(1);
#endif
    shutdown_internal_cap1;
    shutdown_internal_cap2;

    back_from_deepsleep_proc(1, 2);		//2ms delay, when deep back or power on

}

void main_loop(void)
{

#if	TEST_TX_REPEAT_MODE

	static u32 repeat_cnt = 0;
	static u32 main_loop_cnt = 0;

	btn_status.btn_new = 0;
	cpu_rc_tracking_en (RC_TRACKING_32K_ENABLE);

	main_loop_cnt++;
	static u32 main_loop_tick = 0;
	main_loop_tick = clock_time();


	if(vc_status.vc_mode <= STATE_PAIRING){

		vc_paring_and_syncing_proc();
	}

	if((main_loop_cnt > 300) && (repeat_cnt < REPEAT_TIMES)){
		repeat_cnt++;
		vc_event.cnt = 1;
		main_loop_cnt = 0;
		btn_status.btn_new = 1;
		vc_event.keycode[0] = VC_POWER;
	}

	vc_rf_proc(btn_status.btn_new);				//auto pair packet

#if	DBG_GPIO_EN
		GPIO_TOGGLE(GPIO_GP6);
#endif

	cpu_rc_tracking_disable;
//	vc_pm_proc();

	cpu_sleep_wakeup_rc(0, PM_WAKEUP_TIMER, 10);

#else
	//if( !(analog_read(PM_REG_END) && vc_status.vc_mode == STATE_PAIRING))
	btn_status.btn_new = 0;
	cpu_rc_tracking_en (RC_TRACKING_32K_ENABLE);

	button_detect_proc();


#if (FW_WITH_EMI_FUNC)
	if(vc_status.vc_mode == STATE_EMI){			//emi state

#if (!TEST_TX_PAIR_MODE)
		vc_emi_process();
#endif
	}
	else
#endif
	{										//work state

#if TEST_TX_PAIR_MODE
		if( (vc_event.cnt == 1) && (vc_event.keycode[0] == VC_POWER) ){

			callback_auto_paring();
		}
#endif

		if(vc_status.vc_mode <= STATE_PAIRING){

			vc_paring_and_syncing_proc();
		}

#if (FACTORY_PRODUCT_TEST_EN)

		task_keycode_sending_proc();
		vc_rf_proc(btn_status.btn_new);
#else

		if( !repair_and_reset.poweron_reset && !vacuum_deepsleep_cnt){
			vc_rf_proc(btn_status.btn_new);
		}
#endif

		cpu_rc_tracking_disable;

		//GPIO_TOGGLE(GPIO_MAINLOOP_T);

		vc_pm_proc();

		vc_status.loop_cnt++;
	}
#endif
}

