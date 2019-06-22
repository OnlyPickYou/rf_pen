#ifndef _DEVICE_INFO_H_
#define _DEVICE_INFO_H_


void device_info_load(void);
void device_info_save(void);

#define		PM_REG_START		0x19
#define		PM_REG_END			0x1f

#ifndef 	PM_POWERON_DETECTION_ENABLE
#define		PM_POWERON_DETECTION_ENABLE 0
#endif

#endif
