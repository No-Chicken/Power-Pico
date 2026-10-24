#ifndef TEST_BL24C02_H
#define TEST_BL24C02_H

#include <stdbool.h>
#include "user_AdcDataStrategy.h"

void Sys_Get_AdcCalibration(ADC_Calibration_t *calibration);
bool Sys_Set_AdcCalibration(const ADC_Calibration_t *calibration);
bool EEPROM_SysSetting_Save(void);
void EEPROM_UpdateCommand_Write(bool is_update);

#endif
