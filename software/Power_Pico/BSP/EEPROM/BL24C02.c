#include "BL24C02.h"
#include "i2c.h"

// hardware settings
#include "gate.h"
#include "lcd_init.h"

#define EEPROM_PAGE_SIZE            16U
#define EEPROM_SYS_SETTINGS_ADDRESS 0x40U

typedef struct {
    uint8_t backlight_level;       // 0-100
    uint8_t key_sound_enable;      // 0:disable, 1:enable
    uint8_t language_select;       // 0:English, 1:Chinese
    uint16_t rotation;             // 0, 90, 180, 270
    uint8_t current_range_mode;    // 0:auto, 1:low, 2:mid, 3:high
    ADC_Calibration_t adc_calibration;
} SysSettings_T;

static SysSettings_T sys_settings = {
    .backlight_level = 50,
    .key_sound_enable = 1,
    .language_select = 0,
    .rotation = 0,
    .current_range_mode = GATE_MODE_AUTO,
    .adc_calibration = {
        .low_scale_multiplier = 1.0f,
        .mid_scale_multiplier = 1.0f,
        .high_scale_multiplier = 1.0f,
        .low_offset_ua = 0.0f,
        .mid_offset_ua = 0.0f,
        .high_offset_ua = 0.0f,
    },
};

static void SysSettings_SetDefault(SysSettings_T *settings)
{
    settings->backlight_level = 50U;
    settings->key_sound_enable = 1U;
    settings->language_select = 0U;
    settings->rotation = 0U;
    settings->current_range_mode = GATE_MODE_AUTO;
    ADC_Calibration_SetDefault(&settings->adc_calibration);
}

static bool SysSettings_IsValid(const SysSettings_T *settings)
{
    bool rotation_valid;
    bool range_valid;

    if (settings == NULL) {
        return false;
    }

    rotation_valid = settings->rotation == 0U
        || settings->rotation == 90U
        || settings->rotation == 180U
        || settings->rotation == 270U;
    range_valid = settings->current_range_mode == GATE_MODE_AUTO
        || settings->current_range_mode == GATE_MODE_LOW
        || settings->current_range_mode == GATE_MODE_MID
        || settings->current_range_mode == GATE_MODE_HIGH;

    return settings->backlight_level <= 100U
        && settings->key_sound_enable <= 1U
        && settings->language_select <= 1U
        && rotation_valid
        && range_valid
        && ADC_Calibration_IsValid(&settings->adc_calibration);
}

bool EEPROM_ReadBytes(uint8_t address, uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0U || ((uint16_t)address + length) > 256U) {
        return false;
    }

    return HAL_I2C_Mem_Read(&hi2c1,
                            BL24C02_ADDRESS << 1,
                            address,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            length,
                            50U) == HAL_OK;
}

bool EEPROM_WritePage(uint8_t address, const uint8_t *data, uint8_t length)
{
    if (data == NULL
        || length == 0U
        || length > EEPROM_PAGE_SIZE
        || ((address & (EEPROM_PAGE_SIZE - 1U)) + length) > EEPROM_PAGE_SIZE
        || ((uint16_t)address + length) > 256U) {
        return false;
    }
    if (HAL_I2C_Mem_Write(&hi2c1,
                          BL24C02_ADDRESS << 1,
                          address,
                          I2C_MEMADD_SIZE_8BIT,
                          (uint8_t *)data,
                          length,
                          50U) != HAL_OK) {
        return false;
    }

    return HAL_I2C_IsDeviceReady(&hi2c1,
                                 BL24C02_ADDRESS << 1,
                                 20U,
                                 2U) == HAL_OK;
}

bool EEPROM_WriteBytes(uint8_t address, const uint8_t *data, uint16_t length)
{
    uint16_t current_address = address;
    uint16_t remaining = length;

    if (data == NULL || length == 0U || ((uint16_t)address + length) > 256U) {
        return false;
    }

    while (remaining != 0U) {
        uint8_t page_space = (uint8_t)(EEPROM_PAGE_SIZE
            - (current_address & (EEPROM_PAGE_SIZE - 1U)));
        uint8_t chunk_length = remaining < page_space
            ? (uint8_t)remaining
            : page_space;

        if (!EEPROM_WritePage((uint8_t)current_address, data, chunk_length)) {
            return false;
        }
        current_address += chunk_length;
        data += chunk_length;
        remaining -= chunk_length;
    }

    return true;
}

/******************************************
EEPROM Data description:
[0x00]: 0x55 for check
[0x01]: 0xAA for check

[0x20-]: update command storage area, "update\r\n"
[0x40-]: SysSettings_T
*******************************************/

uint8_t EEPROM_Init_Check(void)
{
    uint8_t check_buff[2];

    HAL_Delay(10U);
    if (!EEPROM_ReadBytes(0U, check_buff, sizeof(check_buff))) {
        return EEPROM_ERROR;
    }
    if (check_buff[0] == 0x55U && check_buff[1] == 0xAAU) {
        return EEPROM_OK;
    }

    check_buff[0] = 0x55U;
    check_buff[1] = 0xAAU;
    if (!EEPROM_WritePage(0U, check_buff, sizeof(check_buff))) {
        return EEPROM_ERROR;
    }
    memset(check_buff, 0, sizeof(check_buff));
    if (!EEPROM_ReadBytes(0U, check_buff, sizeof(check_buff))) {
        return EEPROM_ERROR;
    }

    return (check_buff[0] == 0x55U && check_buff[1] == 0xAAU)
        ? EEPROM_OK
        : EEPROM_ERROR;
}

bool EEPROM_SysSetting_Save(void)
{
    SysSettings_T snapshot;

    __disable_irq();
    snapshot = sys_settings;
    __enable_irq();

    return EEPROM_WriteBytes(EEPROM_SYS_SETTINGS_ADDRESS,
                             (const uint8_t *)&snapshot,
                             sizeof(snapshot));
}

bool EEPROM_SysSetting_Get(void)
{
    SysSettings_T loaded;

    if (!EEPROM_ReadBytes(EEPROM_SYS_SETTINGS_ADDRESS,
                          (uint8_t *)&loaded,
                          sizeof(loaded))
        || !SysSettings_IsValid(&loaded)) {
        __disable_irq();
        SysSettings_SetDefault(&sys_settings);
        __enable_irq();
        return false;
    }

    __disable_irq();
    sys_settings = loaded;
    __enable_irq();
    return true;
}

void EEPROM_UpdateCommand_Write(bool is_update)
{
    const char *command = is_update ? "update\r\n" : "-nope-\r\n";
    EEPROM_WritePage(0x20U, (const uint8_t *)command, 8U);
}

bool EEPROM_UpdateCommand_Check(void)
{
    char command[9] = {0};

    if (!EEPROM_ReadBytes(0x20U, (uint8_t *)command, 8U)) {
        return false;
    }
    return strcmp(command, "update\r\n") == 0;
}

void Sys_Set_BacklightLevel(uint8_t level)
{
    if (level <= 100U) {
        sys_settings.backlight_level = level;
        LCD_Set_Light(level);
    }
}

void Sys_Set_KeySoundEnable(bool enable)
{
    sys_settings.key_sound_enable = enable ? 1U : 0U;
}

void Sys_Set_LanguageSelect(uint8_t lang)
{
    if (lang <= 1U) {
        sys_settings.language_select = lang;
    }
}

void Sys_Set_Rotation(uint16_t rotation)
{
    if (rotation == 0U || rotation == 90U || rotation == 180U || rotation == 270U) {
        sys_settings.rotation = rotation;
        LCD_SetRotation(rotation);
    }
}

void Sys_Set_CurrentRangeMode(uint8_t mode)
{
    if (mode == GATE_MODE_AUTO
        || mode == GATE_MODE_LOW
        || mode == GATE_MODE_MID
        || mode == GATE_MODE_HIGH) {
        sys_settings.current_range_mode = mode;
        Gate_Set_Mode(mode);
    }
}

bool Sys_Set_AdcCalibration(const ADC_Calibration_t *calibration)
{
    if (!ADC_Calibration_IsValid(calibration)) {
        return false;
    }

    __disable_irq();
    sys_settings.adc_calibration = *calibration;
    __enable_irq();
    return true;
}

uint8_t Sys_Get_BacklightLevel(void)
{
    return sys_settings.backlight_level;
}

uint8_t Sys_Get_KeySoundEnable(void)
{
    return sys_settings.key_sound_enable;
}

uint8_t Sys_Get_LanguageSelect(void)
{
    return sys_settings.language_select;
}

uint16_t Sys_Get_Rotation(void)
{
    return sys_settings.rotation;
}

uint8_t Sys_Get_CurrentRangeMode(void)
{
    return sys_settings.current_range_mode;
}

void Sys_Get_AdcCalibration(ADC_Calibration_t *calibration)
{
    if (calibration == NULL) {
        return;
    }

    __disable_irq();
    *calibration = sys_settings.adc_calibration;
    __enable_irq();
}
