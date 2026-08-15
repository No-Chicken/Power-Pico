#ifndef __BL24C02_H
#define __BL24C02_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "user_AdcDataStrategy.h"

#define BL24C02_ADDRESS	0x50
#define EEPROM_OK          0U
#define EEPROM_ERROR       1U

bool EEPROM_ReadBytes(uint8_t address, uint8_t *data, uint16_t length);
bool EEPROM_WritePage(uint8_t address, const uint8_t *data, uint8_t length);
bool EEPROM_WriteBytes(uint8_t address, const uint8_t *data, uint16_t length);

uint8_t EEPROM_Init_Check(void);
bool EEPROM_SysSetting_Save(void);
bool EEPROM_SysSetting_Get(void);
void EEPROM_UpdateCommand_Write(bool is_update);
bool EEPROM_UpdateCommand_Check(void);

// set functions

void Sys_Set_BacklightLevel(uint8_t level);
void Sys_Set_KeySoundEnable(bool enable);
void Sys_Set_LanguageSelect(uint8_t lang);
void Sys_Set_Rotation(uint16_t rotation);
void Sys_Set_CurrentRangeMode(uint8_t mode);
bool Sys_Set_AdcCalibration(const ADC_Calibration_t *calibration);

// get functions

uint8_t Sys_Get_BacklightLevel(void);
uint8_t Sys_Get_KeySoundEnable(void);
uint8_t Sys_Get_LanguageSelect(void);
uint16_t Sys_Get_Rotation(void);
uint8_t Sys_Get_CurrentRangeMode(void);
void Sys_Get_AdcCalibration(ADC_Calibration_t *calibration);

#endif
