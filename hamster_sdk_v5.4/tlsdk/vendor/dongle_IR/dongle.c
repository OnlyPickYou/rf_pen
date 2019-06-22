#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/ir.h"
#include "../common/rf_frame.h"
#include "rf_ll.h"
#include "dongle_custom.h"
#include "dongle_usb.h"
#include "dongle_emi.h"
#include "dongle_suspend.h"
#include "dongle.h"
#include "trace.h"

//pairing ack head: 80805113
rf_packet_ack_pairing_t	ack_pairing = {
		sizeof (rf_packet_ack_pairing_t) - 4,	// 0x14=24-4,dma_len

		sizeof (rf_packet_ack_pairing_t) - 5,	// 0x13=24-5,rf_len
		RF_PROTO_BYTE,							// 0x51,proto
		PKT_FLOW_DIR,							// 0x80,flow
		FRAME_TYPE_ACK,							// 0x80,type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
		0,					// info0
		0,					// info1
		0,					// info2

		U32_MAX,			// gid1
		U32_MAX,			// device id
};

//ack empty head: c0805108
rf_ack_empty_t	ack_empty = {
		sizeof (rf_ack_empty_t) - 4,			// 0x09,dma_len

		sizeof (rf_ack_empty_t) - 5,			// 0x08,rf_len
		RF_PROTO_BYTE,							// 0x51,proto
		PKT_FLOW_DIR,							// 0x80,flow
		FRAME_TYPE_ACK_EMPTY,					// 0xc0,type

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
};

//ack debug head: 4080510f
rf_packet_debug_t	ack_debug = {
		sizeof (rf_packet_debug_t) - 4,	// dma_len,0x10

		sizeof (rf_packet_debug_t) - 5,	// rf_len,0x0f
		RF_PROTO_BYTE,					// proto,0x51
		PKT_FLOW_DIR,					// flow,0x80
		FRAME_TYPE_DEBUG,				// type,0x40

//		PIPE0_CODE,			// gid0

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
		0,					// info0
		0,					// info1
		0,					// info2

		U32_MAX,			// gid1
		U32_MAX,			// did,device id
};

//ack kb head: 82805109
rf_packet_ack_keyboard_t	ack_keyboard = {
		sizeof (rf_packet_ack_keyboard_t) - 4,	// dma_len

		sizeof (rf_packet_ack_keyboard_t) - 5,	// rf_len 	0x09
		RF_PROTO_BYTE,		// proto 						0x51
		PKT_FLOW_DIR,		// flow 						0x80
		FRAME_TYPE_ACK_KEYBOARD,					// type 0x82

//		PIPE1_CODE,			// gid1

		0,					// rssi
		0,					// per
		0,					// tick

		0,					// chn
		0,					// status
};
//******************low power mode datas****************//
//u8 low_power_flag = 0;			//flag indicates dongle enters low power mode
//u8 low_power_trigger = 1;		//trigger low power mode,set to be enabled,cleared when received keyboard type pkt
u8 rx_mode = 0;   //0 working mode 1 idle mode
#define WORKING_MODE  0
#define IDLE_MODE     1

u8 tv_mode = 0;
#define FACTORY_PARING	0
#define HOME_PARING		1
//****************working status settings 4 zzcz**********//
#if ZZCZ
#if(IR_ADDR_CODE == 0x00)
#undef IR_ADDR_CODE
#define IR_ADDR_CODE			0xe0
#endif

#define FULL_WORKING  0
#define BOX_SUSPEND	  1
#define FULL_SUSPEND  2

volatile u8  zzcz_status;
#endif

_attribute_ram_code_ void irq_handler(void)
{
	u32 src = reg_irq_src;

	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		irq_host_rx();        
	}

	if(src_rf & FLD_RF_IRQ_TX){
		irq_host_tx();
	}

	if(src & FLD_IRQ_GPIO_RISC2_EN)
	{
		gpio_user_irq_handler();
	}
}

u32	pwm_tick = 0;
volatile dbg_gpioirq_cnt;
u32 pwm_tick_cnt;
u8 pwm_valid_cnt;
_attribute_ram_code_ void gpio_user_irq_handler(void)
{
	u32 us_cnt;
	static u32 last_tick;
	u32 gpio_tick = clock_time();
	pwm_tick = (u32)(gpio_tick - last_tick);
	us_cnt = pwm_tick/CLOCK_SYS_CLOCK_1MS;
	reg_irq_src = FLD_IRQ_GPIO_RISC2_EN;

	rx_mode = 0;  //tv mode from standby to on,

	if(pwm_valid_cnt == 0&&us_cnt != 0)
	{
		pwm_tick_cnt = us_cnt;
		pwm_valid_cnt++;
	}
	else if (pwm_valid_cnt != 0&&us_cnt != 0)
	{
		if(us_cnt >= pwm_tick_cnt-2&&us_cnt <= pwm_tick_cnt+1)
		{
			pwm_tick_cnt = us_cnt;
			pwm_valid_cnt++;
		}
		else
		{
			pwm_valid_cnt = 0;
		}
	}
	last_tick = gpio_tick;
	dbg_gpioirq_cnt++;
}

u32	get_device_id (u8 type)
{
	return U32_MAX;
}


/////////////////////////////////////////////
// debug_data
/////////////////////////////////////////////
u32 debug_a_loop;
u32 rx_rcv_loop_tag;
u32 loop_tick = 0;

u32 debug_callback_pairing;
u32 debug_manual_paring;
u32 debug_manual_soft_paring_ok;
u32 debug_send_pipe1_code;
u32 debug_callback_kb;
u32 debug_kb_data;


/////////////////////////////////////////////
// dongle_custom.c  中的变量
/////////////////////////////////////////////
extern int     rf_paring_enable;
//extern int     golden_dongle_enable;
extern int     auto_paring_enable;
extern int     auto_paring_enable_m2;
//extern int     soft_paring_enable;
extern int     manual_paring_enable;
extern s8 		custom_rssi_paring_th_offset;
extern u8		host_keyboard_status;

/****************************************************************************************************************
     	callback_pairing

函数调用条件：  非golden_dongle的 link包/paring包可以进入该函数
              golden_dongle发link/paring包的时候，dongle直接给PIPE1_CODE，不会进该函数
-----------------------------------------------------------------------------------------------------------------
        配对类型         |   存储配对设备ID号方式        |   是否响应paring包和link包              |        触发配对的条件
-----------------------------------------------------------------------------------------------------------------
auto_paring   | 存入ram，不存入firmware  |   响应 paring包和link包                     |  1.ram中没有存储配对过的设备ID
soft_paring   |   存入firmware和ram     	|   响应paring包，不响应link包        |  1.配对时间允许内
			  |						    |					    		|  2.必须是paring包
	          |  						|								|  3.能量满足
manual_paring |   存入firmware和ram    	|   响应paring包，不响应link包        |  1.配对时间允许内
			  |							|						        |  2.必须是paring包
			  |							|							    |  3.能量满足
			  |						    |							    |  4.ram中已经存储配对ID后，不响应上电配对包
------------------------------------------------------------------------------------------------------------------
 ***************************************************************************************************************/
void	callback_pairing (u8 * p)
{
	// if valid device, return PIPE1_CODE
	// device_id == 0: ROM device
	debug_callback_pairing++;

	rf_packet_pairing_t *p_pkt = (rf_packet_pairing_t *) (p + 8);
    static u32 rx_rssi = 0;
    if ( (p_pkt->flow & PKT_FLOW_PARING) )
    {
        static u8 rssi_last;
        u8 rssi_cur = RECV_PKT_RSSI(p);
        if ( abs(rssi_last - rssi_cur) < 12 ){
            rx_rssi = ( ( (rx_rssi<<1) + rx_rssi + rssi_cur ) >> 2 );
        }
        rssi_last = rssi_cur;
    }
	int  type                   = p_pkt->type;  //pkt->type = : 1: mouse,  2:keyboard
	int  rssi_paring_good       = rx_rssi > (27 - custom_rssi_paring_th_offset);  //rssi > 37(-73 dbm)   manual、soft 对能量有要求, auto没有
	int  device_paring_flag     = p_pkt->flow & PKT_FLOW_PARING;                            //有PKT_FLOW_PARING 标志的为paring包，否则是link包

#if	PARING_MANUAL_MODE_ENABLE
	//manual须满足：1.  rf_paring_enable(配对时间允许内)
	//              2.  device_paring_flag（必须是paring包）
	//              3.  rssi_paring_good（能量满足）
	//manual配对排除：   ram中已经存储配对的ID，接收到的配对包是上电配对包
	if( manual_paring_enable && rf_paring_enable && device_paring_flag && rssi_paring_good   )
	{
		if(p_pkt->rsvd==tv_mode)
		{
			set_device_id_in_firmware(type - FRAME_TYPE_MOUSE, p_pkt->did);
			set_device_id_in_ram(type,p_pkt->did);
			if(tv_mode==HOME_PARING)
			{
				ir_send_code(IR_ADDR_CODE, 0xa0);
				tv_mode = FACTORY_PARING;
				rf_paring_enable = 0;
			}
		}
	}
#endif

}

#define		PER32S128(a, b)			((((a)*31)>>5) + ((b)<<10))
u32		fh_pkt_per = 0;

/****************************************************************************************************************
     	callback_keyboard
 ***************************************************************************************************************/
void	callback_keyboard (u8 *p)
{

//	if(golden_dongle_enable && RECV_PKT_RSSI(p) < (55-custom_rssi_paring_th_offset))  //golden dongle能量不满足
//	{
//			return;  //由于mouse在发link包(或配对包)的时候，无条件给PIPE1_CODE，先判断能量，若不满足，放弃数据
//	}

	debug_callback_kb++;

	rf_packet_keyboard_t *p_pkt = (rf_packet_keyboard_t *)  (p + 8);
	static u8 seq_no_keyboard = 0;
	if (p_pkt->seq_no != seq_no_keyboard) {	//skip same packet
		seq_no_keyboard = p_pkt->seq_no;
		kb_ir_report((kb_data_t *)p_pkt->data);

	}
}

u8 ir_key_buff[16];
u8 ir_wptr;
u8 ir_rptr;
u8 last_key = 0xff;
int kb_ir_report(kb_data_t *data)
{
	kb_data_t * kb_data = (kb_data_t*)(data);
	u8 keycode;
	static u32 last_tick = 0;
	u32 temp_tick;

	keycode = kb_data->keycode[0];
	if(((ir_wptr+1)&0xf)!=(ir_rptr&0xf))
	{
		temp_tick = (u32)(clock_time()-last_tick);
		if(temp_tick >95*CLOCK_SYS_CLOCK_1MS)
		{
			if(last_key==keycode && temp_tick <=118*CLOCK_SYS_CLOCK_1MS )
			{
				ir_key_buff[ir_wptr&0xf]=0xf0;  //0xff represents repeat key
			}
			else
			{
				ir_key_buff[ir_wptr&0xf]=keycode;
				last_key = keycode;
			}
			ir_wptr++;
			last_tick = clock_time();

		}
		else
		{
			last_tick = clock_time();
			return 0;
		}

		return 1;

	}
	else return 0;

//	dbg_cb_cnt++;

}

volatile u32 last_rpt_ir;
volatile u32 last_key_cnt;
void proc_ir()
{
	static u32 key_cnt;
	if((ir_wptr&0xf)!=(ir_rptr&0xf))
	{
		if(ir_key_buff[ir_rptr&0xf]!=0xf0)
			ir_send_code(IR_ADDR_CODE, ir_key_buff[ir_rptr&0x0f]);
		else if((u32)(clock_time()-last_rpt_ir) < 120*CLOCK_SYS_CLOCK_1MS && key_cnt==(last_key_cnt+1))
			ir_send_repeat();
		else
			ir_send_code(IR_ADDR_CODE, last_key);

		ir_rptr++;
		last_key_cnt = key_cnt;
		last_rpt_ir = clock_time();
	}
	key_cnt++;
}

#if ZZCZ
u32 last_high_tick,current_tick;
u8  key_release_flag;
void status_proc()
{
	static u32 low_tick;
	if( gpio_read(GPIO_PWRKY)== 1)		//if key is high do nothing
	{
		last_high_tick = clock_time();
		key_release_flag = 1;
	}
	else
	{
		low_tick++;
		key_release_flag = 0;
		if(zzcz_status == FULL_WORKING )
		{
			if(low_tick == 18)  //presses for 2s,if more do nothing
			{
				low_tick = 0;
				zzcz_status = FULL_SUSPEND;
				gpio_write(GPIP_PWRCRL,0);
			}
			else if(key_release_flag && low_tick <= 5)	//one click
			{
				low_tick = 0;
				zzcz_status = BOX_SUSPEND;
				//send_ir_suspend();			//suspend code unkown
			}
		}
		else if( zzcz_status == BOX_SUSPEND )  // long push
		{
			if(low_tick == 18)
			{
				zzcz_status = FULL_SUSPEND;
				gpio_write(GPIP_PWRCRL,0);
			}
			else if(key_release_flag && low_tick <= 5)	//one click
			{
				zzcz_status = FULL_WORKING;
				//send_ir_wakeup();
			}
		}
		else if(zzcz_status == FULL_SUSPEND )
		{
			zzcz_status = FULL_WORKING;
			//send_ir_wakeup();				//wakeup code unkown
		}
	}
}
#endif

volatile u32 rx_rsp_tick;
_attribute_ram_code_ u8 *  rf_rx_response(u8 * p, callback_rx_func *p_post)
{
	static u32 rf_rx_pkt;
	rf_rx_pkt++;
	rf_packet_pairing_t *p_pkt = (rf_packet_pairing_t *) (p + 8);

	rx_mode = 0;
	rx_rcv_loop_tag = debug_a_loop;

	if (p_pkt->proto == RF_PROTO_BYTE)
	{
		///////////////  Paring/Link request //////////////////////////
		if (rf_get_pipe(p) == PIPE_PARING)
		{	//paring/link request
  			if (p_pkt->did == get_device_id_from_ram(p_pkt->type) ) {
				debug_send_pipe1_code++;
				ack_pairing.gid1 = rf_get_access_code1();
                ack_pairing.did = p_pkt->did;
				*p_post = NULL;
				return (u8 *) &ack_pairing;		//send ACK
			}
//			else if ( !binding_device[(p_pkt->type - FRAME_TYPE_MOUSE) & 1] ){
  			else
  			{
				*p_post = (void *) callback_pairing;
				return (u8 *) &ack_empty;
			}
		}
#if USB_KEYBOARD_ENABLE
		///////////////  keyboard     //////////////////////////
		else if (rf_get_pipe(p) == PIPE_KEYBOARD && p_pkt->type == FRAME_TYPE_KEYBOARD) {
			if(((u32)(clock_time()-rx_rsp_tick)>92*CLOCK_SYS_CLOCK_1MS)&&(custom_binding[p_pkt->type - FRAME_TYPE_MOUSE] == p_pkt->did))
				*p_post = callback_keyboard;
			else
				*p_post = NULL;
				//seq_no_keyboard = p_pkt->seq_no;
//			binding_device[(p_pkt->type - FRAME_TYPE_MOUSE) & 1] = 1;        //working dongle cannot be re-paired

			ack_keyboard.status = host_keyboard_status;
			frq_hopping_data.device_pktRcv_flg |= PKT_RCVD_FLG_KB;
			rx_rsp_tick = clock_time();
			return (u8 *) &ack_keyboard;
		}
		////////// end of PIPE1 /////////////////////////////////////
#endif
	}

	return (u8 *) &ack_empty;
}


void platform_init()
{
	gpio_set_func(GPIO_GP8,AS_GPIO);
	gpio_set_output_en(GPIO_GP8, 0);
	gpio_set_input_en(GPIO_GP8, 1);

	gpio_set_func(GPIO_GP10,AS_GPIO);
	gpio_set_output_en(GPIO_GP10, 0);
	gpio_set_input_en(GPIO_GP10, 1);
	analog_write(0x09,analog_read(0x09)|0xcc);
	reg_gpio_pol |= 0x80;		//rise edge
	reg_gpio_2risc2 |= 0x80;

	reg_irq_mask |= FLD_IRQ_GPIO_RISC2_EN;	//FLD_IRQ_GPIO_RISC1;
	reg_irq_src = FLD_IRQ_GPIO_RISC2_EN;
}
void device_info_load()
{

}

void ir_init(void)
{
	gpio_set_func(GPIO_IR,AS_GPIO);
	gpio_set_output_en(GPIO_IR, 1);
	gpio_set_input_en(GPIO_IR, 0);
	gpio_write(GPIO_IR,1);
}

#if ZZCZ
void power_ctrl_init(void)
{
	gpio_set_func(GPIP_PWRCRL,AS_GPIO);
	gpio_set_output_en(GPIP_PWRCRL, 1);
	gpio_set_input_en(GPIP_PWRCRL, 0);
	gpio_write(GPIO_IR,1);

	gpio_set_func(GPIO_PWRKY,AS_GPIO);
	gpio_set_output_en(GPIO_PWRKY, 0);
	gpio_set_input_en(GPIO_PWRKY, 1);
	gpio_write(GPIO_PWRKY,0);
}
#endif

void  user_init(void)
{
	platform_init();

	custom_init();

//	device_info_load ();   //its already in the custom init

	ll_host_init ((u8 *) &ack_debug);

	rf_set_channel (0, RF_CHN_TABLE);
	rf_set_rxmode ();

	usb_dp_pullup_en (1);

	rf_set_power_level_index (RF_POWER_8dBm);

	last_rpt_ir = clock_time();
	rx_rsp_tick = clock_time();
#if ZZCZ
	zzcz_status = FULL_WORKING;
	last_high_tick = clock_time();
#endif
}

u8 chn_tbl[4] = {0,4,8,12};
extern u8 host_channel;

static inline void device_sleep_wakeup()
{
	u8 r = irq_disable();			// must
	cpu_sleep_wakeup_rc (0 ,PM_WAKEUP_TIMER, 75);  //PM_WAKEUP_CORE
	clock_init();
	rf_power_enable (1);
	rf_drv_init(1);
//	reg_tmr_ctrl |= FLD_TMR1_EN;

	host_channel++;
	rf_set_channel (chn_tbl[host_channel&3], RF_CHN_TABLE);
	rf_set_rxmode ();
	irq_restore(r);
}

volatile int dbg_wk_loop;
volatile int dbg_idl_loop;
void main_loop(void)
{
	debug_a_loop++;

	if(gpio_read(GPIO_GP8)!=0)					//if tv is in standby mode
	{
		sleep_us(10);
		if(gpio_read(GPIO_GP8)!=0)
		{
			rf_paring_enable = 1;
			tv_mode = FACTORY_PARING;
		}
	}
	else
	{
		rf_paring_enable = (~rx_mode)&&tv_mode;		//when tv on, only satisfy the conditions:1:home mode2.remote active3.tv active
	}

	if(rx_mode == 0 )
	{
		dbg_wk_loop++;
		host_channel++;
		tv_status_scan();

		rf_set_channel (chn_tbl[host_channel&3], RF_CHN_TABLE);
		rf_set_rxmode ();

		proc_ir();
#if ZZCZ
		status_proc();
#endif

		if((u32)(debug_a_loop - rx_rcv_loop_tag) > 0xff )
		{ //110*16*16*6 3mins
			rx_mode = 1;
		}

		while(!clock_time_exceed(loop_tick,110 * 1000));
	}
	else if(rx_mode == 1){
		dbg_idl_loop++;
		pwm_tick_cnt = 0;
		pwm_valid_cnt = 0;
		device_sleep_wakeup();
		sleep_us(29000);
	}
	loop_tick = clock_time();
}

volatile u8 dbg_50;
volatile u8 dbg_150;
volatile u8 dbg_30;
_attribute_ram_code_ void tv_status_scan()
{
//	if red led is off
//	if(gpio_read(GPIO_GP10)!=0)					//if tv is in standby mode
//	{
//		sleep_us(10);
//		if(gpio_read(GPIO_GP10)!=0)
//		{
			u8 sw_hw = 0;
			if(pwm_valid_cnt >= 8)	//if 3 intervals of pwm cycle is the same, assume the interval is countable
			{						//whether send once or multi times remains
				if(pwm_tick_cnt>=32&&pwm_tick_cnt<=35)
				{
					dbg_30++;
					rf_paring_enable = 1;
					tv_mode = HOME_PARING;
	//				dbg_hm ++;
				}			//first time mode
				else if(pwm_tick_cnt>=5&&pwm_tick_cnt<=8)
				{
					sw_hw++;
					dbg_150++;
				}			//show version info
				else if(pwm_tick_cnt>=19&&pwm_tick_cnt<=21 )
				{
					u8 temp = *(volatile u8*)(0x3f30);
					dbg_50++;
					u8 r = irq_disable();
					if(temp!=0xff)
					{
						ir_send_code(IR_ADDR_CODE, 0xa1);
					}
					else
					{
						ir_send_code(IR_ADDR_CODE, 0xa2);
					}
					irq_restore(r);
				}			//show pairing info
				pwm_tick_cnt = 0;
				pwm_valid_cnt = 0;
			}
			if(sw_hw>0)
			{
				u8 r = irq_disable();
				for(u8 i=0;i<3;i++)
				{
					ir_send_code(IR_ADDR_CODE, sw_ver);
					ir_send_repeat2();
					ir_send_code(IR_ADDR_CODE, hw_ver);
					ir_send_repeat2();
				}
				irq_restore(r);
				sw_hw = 0;
			}
//		}
//	}
}

