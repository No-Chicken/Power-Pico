#ifndef __BL24C02_H
#define __BL24C02_H

#include <stdint.h>

#define BL24C02_ADDRESS	0x50

uint8_t EEPROM_Check(void);
uint8_t SettingSave(uint8_t *buf, uint8_t addr, uint8_t lenth);
uint8_t SettingGet(uint8_t *buf, uint8_t addr, uint8_t lenth);

#endif
