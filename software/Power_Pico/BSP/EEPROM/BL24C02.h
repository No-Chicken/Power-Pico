#ifndef __BL24C02_H
#define __BL24C02_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "user_AdcDataStrategy.h"

#define BL24C02_ADDRESS	0x50
#define EEPROM_ADC_CAL_ADDRESS   0x40U
#define EEPROM_SYS_SETTINGS_ADDRESS 0x40U

uint8_t EEPROM_Init_Check(void);
bool EEPROM_SysSetting_Save(void);
void EEPROM_SysSetting_Get(void);
bool EEPROM_AdcCalibration_Save(const ADC_Calibration_t *calibration);
void EEPROM_AdcCalibration_Get(ADC_Calibration_t *calibration);
void EEPROM_UpdateCommand_Write(bool is_update);
bool EEPROM_UpdateCommand_Check(void);

// set functions

void Sys_Set_BacklightLevel(uint8_t level);
void Sys_Set_KeySoundEnable(bool enable);
void Sys_Set_LanguageSelect(uint8_t lang);
void Sys_Set_Rotation(uint16_t rotation);
void Sys_Set_CurrentRangeMode(uint8_t mode);
bool Sys_Set_AdcCalibration(const ADC_Calibration_t *calibration);
void Sys_AdcCalibration_Init(void);
bool EEPROM_RestoreLegacyBaseline(void);

// get functions

uint8_t Sys_Get_BacklightLevel(void);
uint8_t Sys_Get_KeySoundEnable(void);
uint8_t Sys_Get_LanguageSelect(void);
uint16_t Sys_Get_Rotation(void);
uint8_t Sys_Get_CurrentRangeMode(void);
void Sys_Get_AdcCalibration(ADC_Calibration_t *calibration);

#endif
