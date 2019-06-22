#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../common/rf_frame.h"
#include "../link_layer/rf_ll.h"
#include "../common/device_power.h"

#include "../../proj/drivers/keyboard.h"

#include "kb_emi.h"
#include "kb_rf.h"
#include "kb_custom.h"
#include "kb_device_info.h"
#include "kb_batt.h"
#include "kb_ota.h"

extern kb_data_t     kb_event;
extern rf_packet_pairing_t	pkt_pairing;

kb_hw_t 	kb_hw;
kb_data_t 	kb_data;
extern kb_emi_info_t	kb_emi_info;
kb_status_t kb_status;

void kb_sleep_wakeup_init(void)
{
	reg_gpio_wakeup_en |= FLD_GPIO_WAKEUP_EN;
	//reg_wakeup_en = FLD_WAKEUP_SRC_GPIO;  //no need

	for(int i=0;i<8 ;i++){
		gpio_enable_wakeup_pin(drive_pins[i], 0, 1); //low wakeup suspend
	}

	//disable all pad wakeup
	analog_write(0x26,analog_read(0x26) & 0xf3);
	analog_write(0x27,0);
	analog_write(0x28,0);
	analog_write(0x29,0);
	analog_write(0x2a,0);
	analog_write(0x2b,0);

	//time into deep_sleep  1s unit default 10s
	device_sleep.thresh_100ms = p_kb_custom_cfg->deep_slp_tick == U8_MAX ? 100 : p_kb_custom_cfg->deep_slp_tick * 10;
	device_sleep.mcu_sleep_en = 1;
}

void kb_emi_detect(void)
{
	if((kb_event.keycode[0] == kb_emi_info.emi_start) && (kb_event.cnt == 1))
	{
		kb_status.kb_mode  = STATE_EMI;
	}
}


///////////////////////////////////////////////////////////////////////////////////
void kb_platform_init( kb_status_t *pStatus)
{
//	kb_hardware_init(pStatus->hw_define);
//	kb_sleep_wakeup_init(pStatus->hw_define);
	if(pStatus->kb_mode == STATE_POWERON){
		kb_pairing_mode_detect();
		kb_emi_detect();
	}
	kb_led_init(GPIO_LED_IND, 0, 5); //time_per_cycle * device_led.cnt_rate = 12ms * 5 = 60ms

#if(KB_WK_198)
	gpio_set_func(GPIO_LED_CAP,AS_GPIO);
	gpio_set_input_en(GPIO_LED_CAP,0);
	gpio_set_output_en(GPIO_LED_CAP,0);
	gpio_write(GPIO_LED_CAP,0);
	gpio_set_func(GPIO_LED_SCR,AS_GPIO);
	gpio_set_input_en(GPIO_LED_SCR,0);
	gpio_set_output_en(GPIO_LED_SCR,0);
	gpio_write(GPIO_LED_SCR,0);
#endif
}

///////////////////////////////////////////////////////////////////////////////////
u32 key_scaned_pre = 0;


/*********************************************************************************************************
kb_mode ��  ����״̬��
	STATE_POWERON
	STATE_SYNCING
	STATE_PAIRING
	STATE_NORMAL
	STATE_EMI

����user_initʱ����  ��  �������
#1 �����ϵ�
#2 �������ui���ϵ�
#3 ����EMI ui���ϵ�
#4 ֮ǰlink OK �� deep back
#5 ֮ǰlink NG ��deep back

�������������user_init�е�״̬�л�ϸ�ڣ�
   			��ֵ			kb_device_info_load��	 kb_platform_init��
#1		STATE_POWERON  -->	STATE_POWERON	-->	  STATE_POWERON		--->  ��user_init����ʱ��ΪSTATE_SYNCING
#2		STATE_POWERON  -->	STATE_POWERON	-->	  STATE_PAIRING
#3		STATE_POWERON  -->	STATE_POWERON	-->	  STATE_EMI
#4		STATE_POWERON  -->	STATE_NORMAL	-->	  STATE_NORMAL
#5		STATE_POWERON  -->	STATE_SYNCING	-->	  STATE_SYNCING

user_init������ֻʣ����״̬��STATE_POWERON����ΪSTATE_SYNCING��
 ********************************************************************************************************/
void  user_init(void)
{
	//	swire2usb_init();
	kb_status.data = &kb_data;
	kb_status.hw_define = &kb_hw;
	kb_status.led_define = &device_led;
	kb_status.no_ack = 1;

	kb_custom_init( &kb_status );

	kb_ota_init();
	kb_batt_det_init();
	kb_device_info_load( &kb_status );
	kb_rf_init( &kb_status );
	kb_sleep_wakeup_init();

	int status = host_keyboard_status | (kb_status.kb_mode == STATE_POWERON ? KB_NUMLOCK_STATUS_POWERON : 0);
	key_scaned_pre = kb_scan_key (status, !km_dat_sending);
	kb_platform_init( &kb_status );


	if(kb_status.kb_mode == STATE_POWERON){
    	kb_led_setup( kb_led_cfg[E_LED_POWER_ON] );
    	kb_status.kb_mode = STATE_SYNCING;
	}

	write_reg8(0x800063,0xf3); //0.08 mA
	analog_write(0x80,0x80);   //0.07 mA

}

_attribute_ram_code_ void irq_handler(void)
{
#if 0
	u32 src = reg_irq_src;
	if(src & FLD_IRQ_GPIO_RISC2_EN){
		reg_irq_src = FLD_IRQ_GPIO_RISC2_EN;
		gpio_user_irq_handler();
	}
#endif
	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_device_rx();
	}

	if(src_rf & FLD_RF_IRQ_TX){
		irq_device_tx();
	}
}



_attribute_ram_code_ void kb_power_saving_process(void)
{
	device_sleep.quick_sleep = HOST_NO_LINK;
	if ( kb_status.kb_mode <= STATE_PAIRING ){
		if ( kb_status.loop_cnt < KB_NO_QUICK_SLEEP_CNT )  //sync dongle time
			device_sleep.quick_sleep = 0;
	}

	device_sleep.device_busy = DEVICE_PKT_ASK || LED_EVENT_BUSY;

	device_sleep_mode_machine( &device_sleep );

	device_sleep.wakeup_time = KB_MAIN_LOOP_TIME_MS;
	device_sleep.wakeup_src = PM_WAKEUP_TIMER;

	if ( device_sleep.mode & M_SUSPEND_100MS ){
		device_sleep.wakeup_time = 100;
		device_sleep.wakeup_src = PM_WAKEUP_CORE | PM_WAKEUP_TIMER;
		//device_sync = 0;
	}
	else if ( M_SUSPEND_MCU_SLP & device_sleep.mode ){
		device_sleep.wakeup_time = 0;
		device_sleep.wakeup_src = PM_WAKEUP_PAD;
		//device_sync = 0;

		kb_device_info_save(&kb_status);

		//8 drive pin   wakeup deep,  should  change  pullupdn
		for(int i=0;i<8 ;i++){
			gpio_setup_up_down_resistor(drive_pins[i], 0);
			cpu_set_gpio_wakeup(drive_pins[i], 1, 1);    //high wakeup deep
		}
		for(int i=0;i<18 ;i++){
			gpio_setup_up_down_resistor(scan_pins[i], PM_PIN_PULLUP_10K);
		}

	}
}


#ifndef  DEBUG_KB_NO_SUSPEND
#define DEBUG_KB_NO_SUSPEND     0
#endif

_attribute_ram_code_ void kb_pm_proc(void)
{
	ll_add_clock_time (KB_MAIN_LOOP_TIME_MS*1000);
#if(DEBUG_KB_NO_SUSPEND)
    cpu_suspend_wakeup_sim (KB_MAIN_LOOP_TIME_MS*1000);
#else
	u32 next_wakeup_tick = reg_system_wakeup_tick + CLOCK_SYS_CLOCK_1MS * device_sleep.wakeup_time;
	if ( next_wakeup_tick < clock_time() + CLOCK_SYS_CLOCK_1MS * 2 )
		next_wakeup_tick = clock_time() + CLOCK_SYS_CLOCK_1MS * device_sleep.wakeup_time;

	kb_cpu_sleep_wakeup (M_SUSPEND_MCU_SLP & device_sleep.mode, device_sleep.wakeup_src, next_wakeup_tick);

#endif
}

void kb_task_when_rf ( void ){
	kb_led_process(kb_status.led_define);
	static u32 batt_det_count;
	if( device_sleep.mode == M_SUSPEND_8MS ){
		if ( ++batt_det_count >= 600 ){
	    	if( !(analog_read (0x06) & 1)){
	    		batt_det_count = 0;
	    		kb_batt_det_process();
	    		//pkt_km.per = kb_batt_det_process()/6;  //debug
	    	}
		}
	}

	kb_ota_mode_detect(&kb_status);
}

u32		dbg_loop;
/*********************************************************************************************************
����main_loopʱ��5������ֱ��Ӧ��״̬Ϊ��
#1		STATE_SYNCING
#2		STATE_PAIRING
#3		STATE_EMI
#4		STATE_NORMAL
#5		STATE_SYNCING

#3 EMI ��kb_emi_process ��������

STATE_SYNCING �� STATE_PAIRING������pipe0 ����link/paring
			 ���������У�������״̬��kb_paring_and_syncing_proc������������һ��ʱ����Ȼ���л���STATE_NORMAL
			 ״̬����δ��벻�ٱ����á������Ż�ʱ���ú�������Ҫ�ŵ�RAM��ִ�С�

STATE_NORMAL ��ʾ�Ѿ�����dongle��������pipe2������������ݷ��͡�һ������STATE_NORMAL,��Զ�����лص�����״̬
*********************************************************************************************************/
void main_loop(void)
{
	dbg_loop ++;
	dbg_led_low;
	u32 key_scaned = kb_scan_key (host_keyboard_status, !km_dat_sending);

	if(kb_status.kb_mode == STATE_EMI){
		kb_emi_process(&kb_status);
	}
	else if(kb_status.kb_mode == STATE_OTA){
		kb_ota_process(&kb_status);
	}
	else{
		//paring and syncing mode proc
		if(kb_status.kb_mode <= STATE_PAIRING){
			kb_paring_and_syncing_proc(&kb_status);
		}
		p_task_when_rf = kb_task_when_rf;
		kb_rf_proc(key_scaned);
	    if (p_task_when_rf != NULL) {
	       (*p_task_when_rf) ();
	    }

		kb_power_saving_process();
		kb_pm_proc();
	}
}
