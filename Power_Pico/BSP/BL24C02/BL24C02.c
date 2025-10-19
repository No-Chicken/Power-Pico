#include "BL24C02.h"
#include "i2c.h"

SysSettings_T sys_settings = {
	.backlight_level = 80,
	.key_sound_enable = 1,
	.language_select = 0,
	.rotation = 180
};

static void BL24C02_Write(uint8_t addr, uint16_t length, uint8_t buff[])
{
	HAL_I2C_Mem_Write(&hi2c1, BL24C02_ADDRESS<<1, addr, I2C_MEMADD_SIZE_8BIT, buff, length, 10);
}

static void BL24C02_Read(uint8_t addr, uint8_t length, uint8_t buff[])
{
	HAL_I2C_Mem_Read(&hi2c1, BL24C02_ADDRESS<<1, addr, I2C_MEMADD_SIZE_8BIT, buff, length, 10);
}

static void delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}

/******************************************
EEPROM Data description:
[0x00]:0x55 for check
[0x01]:0xAA for check

[0x10]:user wrist setting, HWInterface.IMU.wrist_is_enabled
[0x11]:user ui_APPSy_EN setting

*******************************************/

// to check the Data is right and the EEPROM is working
uint8_t EEPROM_Init_Check(void)
{
	uint8_t check_buff[2];
	delay_ms(10);
	BL24C02_Read(0,2,check_buff);
	if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
	{
		return 0;//check OK
	}
	else
	{
		check_buff[0] = 0x55;
		check_buff[1] = 0xAA;
		delay_ms(10);
		BL24C02_Write(0,2,check_buff);
		memset(check_buff,0,2);
		delay_ms(10);
		BL24C02_Read(0,2,check_buff);
		if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
			return 0;//check ok
	}
	return 1;//check erro
}


// to Save the settings
void Sys_Setting_Save(void)
{
	uint8_t buff[5];
	buff[0] = sys_settings.backlight_level;
	buff[1] = sys_settings.key_sound_enable;
	buff[2] = sys_settings.language_select;
	buff[3] = (uint8_t)(sys_settings.rotation & 0x00FF);
	buff[4] = (uint8_t)((sys_settings.rotation >> 8) & 0x00FF);
	BL24C02_Write(0x10,5,buff);
}


// to Get the settings
void Sys_Setting_Get(void)
{
	uint8_t buff[5];
	BL24C02_Read(0x10,5,buff);
	// 判断是否在范围内
	if(buff[0] <= 100)
		sys_settings.backlight_level = buff[0];
	else
		sys_settings.backlight_level = 80;
	if(buff[1] <= 1)
		sys_settings.key_sound_enable = buff[1];
	else
		sys_settings.key_sound_enable = 1;
	if(buff[2] <= 1)
		sys_settings.language_select = buff[2];
	else
		sys_settings.language_select = 0;
	uint16_t rotation = (uint16_t)buff[3] | ((uint16_t)buff[4] << 8);
	if(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270)
		sys_settings.rotation = rotation;
	else
		sys_settings.rotation = 180;
}
