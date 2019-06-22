/*
 * kb_custom.h
 *
 *  Created on: 2015-1-21
 *      Author: Administrator
 */

#ifndef KB_CUSTOM_H_
#define KB_CUSTOM_H_

#define BAT_DET 	1

typedef struct{
	u16	pipe_pairing;			// pipe 0,pairig
	u16 pipe_kb_data;			// pipe 2,kb data
	u32 did;					// device id

	u8	cap;
	u8  tx_power_paring;
    u8  tx_power;
	u8  tx_power_emi;

	u16 gpio_lvd;  //3f0c
	u16 gpio_scr;
	u16 gpio_cap;
	u16 gpio_num;

	u8  level_lvd;
	u8  level_scr;
	u8  level_cap;
	u8  level_num;

}kb_custom_cfg_t;


extern kb_custom_cfg_t *p_kb_custom_cfg;

#define kb_cust_tx_power        ((p_kb_custom_cfg->tx_power 	   == 0xff) ? RF_POWER_8dBm   : p_kb_custom_cfg->tx_power)
#define kb_cust_tx_power_emi	((p_kb_custom_cfg->tx_power_emi	   == 0xff) ? RF_POWER_8dBm   : p_kb_custom_cfg->tx_power_emi)
#define kb_cust_tx_power_paring ((p_kb_custom_cfg->tx_power_paring == 0xff) ? RF_POWER_3dBm : p_kb_custom_cfg->tx_power_paring )


extern void kb_custom_init(void);

#endif /* KB_CUSTOM_H_ */
