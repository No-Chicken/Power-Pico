#ifndef __BL24C02_H
#define __BL24C02_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define BL24C02_ADDRESS	0x50

uint8_t EEPROM_Init_Check(void);
void EEPROM_SysSetting_Save(void);
void EEPROM_SysSetting_Get(void);
void EEPROM_UpdateCommand_Write(bool is_update);
bool EEPROM_UpdateCommand_Check(void);

// set functions

void Sys_Set_BacklightLevel(uint8_t level);
void Sys_Set_KeySoundEnable(bool enable);
void Sys_Set_LanguageSelect(uint8_t lang);
void Sys_Set_Rotation(uint16_t rotation);

// get functions

uint8_t Sys_Get_BacklightLevel(void);
uint8_t Sys_Get_KeySoundEnable(void);
uint8_t Sys_Get_LanguageSelect(void);
uint16_t Sys_Get_Rotation(void);

#endif
