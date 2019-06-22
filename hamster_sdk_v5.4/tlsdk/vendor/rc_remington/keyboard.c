
#include "../../proj/tl_common.h"
#include "../../proj/drivers/usbkeycode.h"
#include "keyboard.h"

#include "kb_led.h"

//#include "../os/ev.h"
//#include "../os/sys.h"

//#include "../../vendor/common/keyboard_cfg.h"
//#include "../../vendor/common/custom.h"

u16 drive_pins[] = KB_DRIVE_PINS;	//8
u16 scan_pins[] = KB_SCAN_PINS;		//18

kb_data_t	kb_event;
STATIC_ASSERT(IMPLIES((!KB_ONLY_SINGLEKEY_SUPP), KB_RETURN_KEY_MAX > 1));

#ifndef		SCAN_PIN_50K_PULLUP_ENABLE
#define		SCAN_PIN_50K_PULLUP_ENABLE		0
#endif

#ifndef		KB_MAP_DEFAULT
#define		KB_MAP_DEFAULT		1
#endif

#ifndef		KB_LINE_MODE
#define		KB_LINE_MODE		0
#endif

#ifndef		KB_LINE_HIGH_VALID
#define		KB_LINE_HIGH_VALID		1
#endif

#ifndef		KB_KEY_FLASH_PIN_MULTI_USE
#define		KB_KEY_FLASH_PIN_MULTI_USE		0
#endif

#ifndef		KB_HAS_CTRL_KEYS
#define		KB_HAS_CTRL_KEYS		1
#endif

#ifndef		KB_ONLY_SINGLEKEY_SUPP
#define		KB_ONLY_SINGLEKEY_SUPP		0
#endif

#ifndef		KB_RM_GHOST_KEY_EN
#define		KB_RM_GHOST_KEY_EN		0
#endif

#ifndef		KB_HAS_FN_KEY
#define		KB_HAS_FN_KEY		1
#endif

#ifndef		KB_DRV_DELAY_TIME
#define		KB_DRV_DELAY_TIME		10
#endif

#ifndef		KB_KEY_VALUE_MERGE
#define		KB_KEY_VALUE_MERGE		1
#endif

typedef u8 kb_k_mp_t[ARRAY_SIZE(drive_pins)]; //typedef u8 kb_k_mp_t[8]

#if		KB_MAP_DEFAULT

static const u8 kb_map_normal15[ARRAY_SIZE(scan_pins)][ARRAY_SIZE(drive_pins)] = KB_MAP_NORMAL_15;
static const u8 kb_map_normal16[ARRAY_SIZE(scan_pins)][ARRAY_SIZE(drive_pins)] = KB_MAP_NORMAL_16;

#define small_keyboard_enable  		0

kb_k_mp_t *	kb_p_map[4] = {
		kb_map_normal15,
		kb_map_normal16,
};



u32	scan_pin_need;

static u8 	kb_is_fn_pressed = 0;

//u8 (*kb_k_mp)[ARRAY_SIZE(drive_pins)];
kb_k_mp_t * kb_k_mp;
static void kb_rmv_ghost_key(u32 * pressed_matrix)
{
	u32 mix_final = 0;
	foreach_arr(i, drive_pins){
		for(int j = (i+1); j < ARRAY_SIZE(drive_pins); ++j){
			u32 mix = (pressed_matrix[i] & pressed_matrix[j]);
			// >=2 根线重合,  那就是 ghost key
			//four or three key at "#" is pressed at the same time, should remove ghost key
			if( mix && (!BIT_IS_POW2(mix) || (pressed_matrix[i] ^ pressed_matrix[j])) ){
				// remove ghost keys
				//pressed_matrix[i] &= ~mix;
				//pressed_matrix[j] &= ~mix;
				mix_final |= mix;
			}
		}
		pressed_matrix[i] &= ~mix_final;
	}
}

extern u32     sys_host_status;

static u32 key_debounce_filter( u32 mtrx_cur[], u32 filt_en )
{
#if(0)
	u32 kc = 0;
    static u32 mtrx_pre1[ARRAY_SIZE(drive_pins)];
    static u32 mtrx_pre2[ARRAY_SIZE(drive_pins)];
    static u32 mtrx_pre3[ARRAY_SIZE(drive_pins)];
    static u32 mtrx_pre4[ARRAY_SIZE(drive_pins)];
    static u32 mtrx_last[ARRAY_SIZE(drive_pins)];
    foreach_arr(i, drive_pins){
        u32 mtrx_tmp = mtrx_cur[i];
        if( filt_en ){
            //mtrx_cur[i] = (mtrx_last[i] ^ mtrx_tmp) ^ (mtrx_last[i] | mtrx_tmp);  //key_matrix_pressed is valid when current and last value is the same
            mtrx_cur[i] = ( ~mtrx_last[i] & (mtrx_pre4[i] & mtrx_pre3[i] & mtrx_pre2[i] & mtrx_pre1[i] & mtrx_tmp) ) |  \
            		       ( mtrx_last[i] & (mtrx_pre4[i] | mtrx_pre3[i] | mtrx_pre2[i] | mtrx_pre1[i] | mtrx_tmp) );
        }
        if ( mtrx_cur[i] != mtrx_last[i] ) {
        	kc = 1;
        }

        mtrx_pre4[i] = mtrx_pre3[i];
        mtrx_pre3[i] = mtrx_pre2[i];
        mtrx_pre2[i] = mtrx_pre1[i];
        mtrx_pre1[i] = mtrx_tmp;


        mtrx_last[i] = mtrx_cur[i];
    }
    return kc;
#else
	u32 kc = 0;
    static u32 mtrx_pre[ARRAY_SIZE(drive_pins)];
    static u32 mtrx_last[ARRAY_SIZE(drive_pins)];
    foreach_arr(i, drive_pins){
        u32 mtrx_tmp = mtrx_cur[i];
        if( filt_en ){
            //mtrx_cur[i] = (mtrx_last[i] ^ mtrx_tmp) ^ (mtrx_last[i] | mtrx_tmp);  //key_matrix_pressed is valid when current and last value is the same
            mtrx_cur[i] = ( ~mtrx_last[i] & (mtrx_pre[i] & mtrx_tmp) ) | ( mtrx_last[i] & (mtrx_pre[i] | mtrx_tmp) );
        }
        if ( mtrx_cur[i] != mtrx_last[i] ) {
        	kc = 1;
        }
        mtrx_pre[i] = mtrx_tmp;
        mtrx_last[i] = mtrx_cur[i];
    }
    return kc;

#endif
}


// input:          pressed_matrix,
// key_code:   output keys array
// key_max:    max keys should be returned
//u8 a_debug_key = 0xee;
static inline void kb_remap_key_row(int drv_ind, u32 m, int key_max, kb_data_t *kb_data)
{
	foreach_arr(i, scan_pins){
		//a_debug_key = 0xe5;
		if(m & 0x01){
			//a_debug_key = 0xe4;
			u8 kc = kb_k_mp[i][drv_ind];

			kb_data->keycode[kb_data->cnt++] = kc;

			if(kb_data->cnt >= key_max){
				break;
			}
		}
		m = m >> 1;
		if(!m){
			break;
		}
	}
}
//u8 a_debug_remap = 0xcd;
static inline void kb_remap_key_code(u32 * pressed_matrix, int key_max, kb_data_t *kb_data, int numlock_status)
{

//	kb_k_mp = kb_p_map[small_keyboard_enable | ((numlock_status&1) << 1) | (kb_is_fn_pressed << 2)];

	foreach_arr(i, drive_pins){
		u32 m = pressed_matrix[i];
//		a_debug_remap = 0xcc;
		if(!m) continue;
//		a_debug_remap = 0xce;
		kb_remap_key_row(i, m, key_max, kb_data);
//		a_debug_remap = 0xcf;
		if(kb_data->cnt >= key_max){
			break;
		}
	}
}

static u32 kb_key_pressed(u8 * gpio)
{
	foreach_arr(i,drive_pins){
		gpio_write(drive_pins[i], KB_LINE_HIGH_VALID);
		gpio_set_output_en(drive_pins[i], 1);
	}
	sleep_us (20);
	gpio_read_all (gpio);

	u32 ret = 0;
	static u8 release_cnt = 0;
	static u32 ret_last = 0;

	foreach_arr(i,scan_pins){
		if(KB_LINE_HIGH_VALID != !gpio_read_cache (scan_pins[i], gpio)){
			ret |= (1 << i);
			release_cnt = 6;
			ret_last = ret;
		}
		//ret = ret && gpio_read(scan_pins[i]);
	}
	if(release_cnt){
		ret = ret_last;
		release_cnt--;
	}
	foreach_arr(i,drive_pins){
		gpio_write(drive_pins[i], 0);
		gpio_set_output_en(drive_pins[i], 0);
	}
	return ret;
}

#if(__PROJECT_KEYBOARD__)
_attribute_ram_code_
#endif
static u32 kb_scan_row(int drv_ind, u8 * gpio)
{
	/*
	 * set as gpio mode if using spi flash pin
	 * */
	u8 sr = irq_disable();
#if	(KB_KEY_FLASH_PIN_MULTI_USE)
	MSPI_AS_GPIO;
#endif

#if(!KB_LINE_MODE)
	u32 drv_pin = drive_pins[drv_ind];
	gpio_write(drv_pin, KB_LINE_HIGH_VALID);
	gpio_set_output_en(drv_pin, 1);
#endif

	u32 matrix = 0;
	foreach_arr(j, scan_pins){
		if(scan_pin_need & BIT(j)){
			int key = !gpio_read_cache (scan_pins[j], gpio);
			if(KB_LINE_HIGH_VALID != key) {
				if (KB_HAS_FN_KEY && (kb_k_mp[j][(drv_ind+7)&7] == VK_FN)) {
					kb_is_fn_pressed = 1;
				}
				matrix |= (1 << j);
			}
		}
	}
	//sleep_us(KB_DRV_DELAY_TIME);
	gpio_read_all (gpio);
	/*
	 * set as spi mode  if using spi flash pin
	 * */
#if	(KB_KEY_FLASH_PIN_MULTI_USE)
	MSPI_AS_SPI;
#endif

#if(!KB_LINE_MODE)
	////////		float drive pin	////////////////////////////
	//sleep_us(KB_SCAN_DELAY_TIME);
	gpio_write(drv_pin, 0);
	gpio_set_output_en(drv_pin, 0);
#endif

	irq_restore(sr);
	return matrix;
}
/////////////////////external interface for keyboard module/////////////////////
#if(KB_KEY_VALUE_MERGE)
#define  KEY_BUF_SIZE   			8
typedef struct{
	u8	 		key_cnt;
	u8          rsv[3];
	u8          loop[KEY_BUF_SIZE];
	kb_data_t 	value[KEY_BUF_SIZE];
}keyEvt_buf_t;
keyEvt_buf_t keyEvt_buf;

#define KEY_SPECIAL(key) 			(key.cnt==0 || key.ctrl_key != 0)
#define KEY_MERGABLE(key1,key2)     (key1.cnt + key2.cnt <= KB_RETURN_KEY_MAX)

#define SAME_KEY_REMOVER_WHEN_MERGE     0

#define DBG_KEY_MERGE     0
#if DBG_KEY_MERGE
keyEvt_buf_t keyEvt_dbg;
#endif

inline void key_merge(int index,int max){

	int j,k;
	int des_cnt = keyEvt_buf.value[index].cnt;
	int src_cnt = keyEvt_buf.value[index+1].cnt;
	for(j=0;j<src_cnt;j++){
#if(SAME_KEY_REMOVER_WHEN_MERGE)
		for(k=0;k<des_cnt;k++){
			if(keyEvt_buf.value[index+1].keycode[j] == keyEvt_buf.value[index].keycode[k]){
				break;
			}
		}
		if(k<des_cnt){  //same key,jump
			continue;
		}
#endif
		keyEvt_buf.value[index].keycode[des_cnt++] = keyEvt_buf.value[index+1].keycode[j];
	}
	keyEvt_buf.value[index].cnt = des_cnt;

	for(j=index+1;j<=max;j++){
		memcpy(&keyEvt_buf.value[j],&keyEvt_buf.value[j+1],sizeof(kb_data_t));
	}
}


inline void key_buf_merge(void)
{
#if DBG_KEY_MERGE
	memcpy(&keyEvt_dbg,&keyEvt_buf,sizeof(keyEvt_buf_t));
#endif
	int i;
	int des_max = KEY_BUF_SIZE - 2;
	int src_max = KEY_BUF_SIZE - 1;
	for(i=0;i<=des_max;i++){
		if(KEY_SPECIAL(keyEvt_buf.value[i])){
			continue;
		}
		while(i+1 <= src_max){
			if(KEY_SPECIAL(keyEvt_buf.value[i+1])){
				i++;
				break;
			}
			if(KEY_MERGABLE(keyEvt_buf.value[i],keyEvt_buf.value[i+1])){  //merge
				key_merge(i,des_max);
				des_max--;
				src_max--;
			}
			else{
				break;
			}
		}
	}

	if(des_max ==  KEY_BUF_SIZE - 2){ //no merge,overwrite
		for(i=0;i<KEY_BUF_SIZE-1;i++){
			memcpy(&keyEvt_buf.value[i],&keyEvt_buf.value[i+1],sizeof(kb_event));
		}
		keyEvt_buf.key_cnt = KEY_BUF_SIZE - 1;
	}
	else{
		keyEvt_buf.key_cnt = des_max + 2;
	}
#if DBG_KEY_MERGE
	kb_device_led_setup(kb_led_cfg[6]);
#endif
}

#endif

u32 	matrix_buff[4][ARRAY_SIZE(drive_pins)];//matrix_buff[4][8]
int		matrix_wptr, matrix_rptr;



#if(__PROJECT_KEYBOARD__)
_attribute_ram_code_
#endif
u32 kb_scan_key (int numlock_status, int read_key)
{
	int i;
	u8 gpio[8];
    scan_pin_need = kb_key_pressed (gpio);


	if(scan_pin_need){

		kb_event.cnt = 0;
		kb_event.ctrl_key = 0;
		kb_is_fn_pressed = 0;

		u32 pressed_matrix[ARRAY_SIZE(drive_pins)] = {0};

		kb_scan_row (0, gpio);
		for (int i=0; i<=ARRAY_SIZE(drive_pins); i++) {
			u32 r = kb_scan_row (i < ARRAY_SIZE(drive_pins) ? i : 0, gpio);
			if (i) {
				pressed_matrix[i - 1] = r;
			}
		}

#if(KB_RM_GHOST_KEY_EN)
		kb_rmv_ghost_key(&pressed_matrix[0]);
#endif
//in pairing mode key_debounce_filter cannot detect key pressed.
//need discussing
		static u32 data_same_cnt;
		static u8  first_click_flag = 0;

		u32  key_changed;

		if(kb_status.kb_mode == STATE_POWERON)
		{
			key_changed = 1;
		}
		else
		{
			key_changed = key_debounce_filter( pressed_matrix, 1);
			if(key_changed)
			{
				data_same_cnt = 0;
				first_click_flag = 0;
			}
			else
			{
				data_same_cnt++;
				if(first_click_flag == 0)
				{
					if(data_same_cnt >= (286/KB_MAIN_LOOP_TIME_MS)){  //200ms
						key_changed = 1;
						data_same_cnt = 0;
						first_click_flag = 1;
					}
				}
				else
				{
					if(data_same_cnt >= (110/KB_MAIN_LOOP_TIME_MS))
					{
						key_changed = 1;
						data_same_cnt = 0;
					}
				}
			}
		}


#if(KB_KEY_VALUE_MERGE)
		u32 *pd;
		kb_data_t *key_ptr;
		if(key_changed){
			pd = matrix_buff[matrix_wptr&3];
			for (int k=0; k<ARRAY_SIZE(drive_pins); k++) {
				*pd++ = pressed_matrix[k];
			}
			matrix_wptr = (matrix_wptr + 1) & 7;
			if ( ((matrix_wptr - matrix_rptr) & 7) > 4 ) {	//overwrite older data
				matrix_rptr = (matrix_wptr - 4) & 7;
			}
		}
		if( (numlock_status & KB_NUMLOCK_STATUS_INVALID)
			#if(!__PROJECT_REMOTE__)
			&& (kb_status.kb_mode == STATE_NORMAL)
			#endif
			){  //KB_NUMLOCK_STATUS_ALL
			return 1;
		}

		if(matrix_wptr != matrix_rptr){ //
			pd = matrix_buff[matrix_rptr&3];
			matrix_rptr = (matrix_rptr + 1) & 7;

			if(keyEvt_buf.key_cnt == KEY_BUF_SIZE){ //overwrite
				key_buf_merge();
			}
			key_ptr = (kb_data_t *)(&keyEvt_buf.value[keyEvt_buf.key_cnt]);
			keyEvt_buf.key_cnt++;
			key_ptr->cnt = 0;
			key_ptr->ctrl_key = 0;
			kb_remap_key_code(pd, KB_RETURN_KEY_MAX,key_ptr,numlock_status);
		}

		if (!keyEvt_buf.key_cnt || !read_key) {
			return 0;
		}
		memcpy(&kb_event,&keyEvt_buf.value[0],sizeof(kb_data_t));
		for(i=0;i<keyEvt_buf.key_cnt-1;i++){
			memcpy(&keyEvt_buf.value[i],&keyEvt_buf.value[i+1],sizeof(kb_data_t));
		}
		keyEvt_buf.key_cnt --;

#else
		///////////////////////////////////////////////////////////////////
		//	insert buffer here
		//       key mapping requires NUMLOCK status
		///////////////////////////////////////////////////////////////////
		u32 *pd;
		if (key_changed)
		{
			/////////// push to matrix buffer /////////////////////////
			pd = matrix_buff[matrix_wptr&3];
			for (int k=0; k<ARRAY_SIZE(drive_pins); k++)
			{
				*pd++ = pressed_matrix[k];
			}
			matrix_wptr = (matrix_wptr + 1) & 7;
			if ( ((matrix_wptr - matrix_rptr) & 7) > 4 )
			{	//overwrite older data
				matrix_rptr = (matrix_wptr - 4) & 7;
			}
		}

		if (numlock_status & KB_NUMLOCK_STATUS_INVALID) {
			return 1;		//return empty key
		}

		////////// read out //////////
//		if (matrix_wptr == matrix_rptr || !read_key) {
		if (matrix_wptr == matrix_rptr){
			return 0;			//buffer empty, no data
		}
		pd = matrix_buff[matrix_rptr&3];
		matrix_rptr = (matrix_rptr + 1) & 7;

		///////////////////////////////////////////////////////////////////
		kb_remap_key_code(pd, KB_RETURN_KEY_MAX, &kb_event, numlock_status);
#endif
		return 2;
	}
    return 0;
}


void kb_keyScan_init(void)
{
	//u8 map_index = (read_reg8(0x598)&0x80)!=0;		//kevin	//use different key mapping according to GP29 level
	//kb_k_mp = kb_p_map[map_index];		//kevin
	kb_k_mp = kb_p_map[0];     //kevin
}


#endif


///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
