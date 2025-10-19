#ifndef __BL24C02_H
#define __BL24C02_H

#include <stdint.h>

#define BL24C02_ADDRESS	0x50

typedef struct {
	uint8_t  backlight_level;   // 0-100
	uint8_t  key_sound_enable;  // 0:disable, 1:enable
	uint8_t  language_select;   // 0:English, 1:Chinese
	uint16_t  rotation;		    // 0, 90, 180, 270
} SysSettings_T;

extern SysSettings_T sys_settings;

uint8_t EEPROM_Init_Check(void);
void Sys_Setting_Save(void);
void Sys_Setting_Get(void);

#endif
