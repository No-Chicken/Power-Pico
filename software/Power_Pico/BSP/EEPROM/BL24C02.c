#include "BL24C02.h"
#include "i2c.h"
// hardware settings
#include "lcd_init.h"
#include "gate.h"
#include "cmsis_os2.h"
#include <stddef.h>
#include <math.h>

typedef struct {
	uint8_t  backlight_level;   // 0-100
	uint8_t  key_sound_enable;  // 0:disable, 1:enable
	uint8_t  language_select;   // 0:English, 1:Chinese
	uint16_t  rotation;		    // 0, 90, 180, 270
	uint8_t  current_range_mode; // 0:auto, 1:low, 2:mid, 3:high
} SysSettings_T;

/* This is the upstream 1.1.7 on-EEPROM representation. Keep runtime
 * calibration in the legacy raw-scale form, and convert only at storage. */
typedef struct {
	uint8_t  backlight_level;
	uint8_t  key_sound_enable;
	uint8_t  language_select;
	uint16_t rotation;
	uint8_t  current_range_mode;
	uint8_t  reserved;
	float low_scale_multiplier;
	float mid_scale_multiplier;
	float high_scale_multiplier;
	float low_offset_ua;
	float mid_offset_ua;
	float high_offset_ua;
} SysSettingsStorage_T;

_Static_assert(sizeof(SysSettingsStorage_T) == 32U,
               "EEPROM settings layout must match upstream 1.1.7");
_Static_assert(offsetof(SysSettingsStorage_T, rotation) == 4U &&
               offsetof(SysSettingsStorage_T, low_scale_multiplier) == 8U,
               "EEPROM field offsets must match upstream");

#define LEGACY_BACKUP_ADDRESS 0xC0U
#define LEGACY_BACKUP_MAGIC 0x31474D50U
typedef struct {
	uint32_t magic;
	uint8_t settings[6];
	uint8_t reserved[2];
	ADC_Calibration_t calibration;
	uint32_t checksum;
} LegacyBackup_T;
_Static_assert(sizeof(LegacyBackup_T) == 40U,
               "Backup must fit at 0xC0..0xE7, outside diagnostics");
static LegacyBackup_T legacy_backup;
static bool migration_pending;

static SysSettings_T sys_settings = {
	.backlight_level = 50,
	.key_sound_enable = 1,
	.language_select = 0,
	.rotation = 0,
	.current_range_mode = GATE_MODE_AUTO
};

// Runtime calibration cache; EEPROM holds the persistent copy.
static ADC_Calibration_t adc_calibration;
static bool calibration_initialized;

static bool BL24C02_Write(uint8_t addr, uint16_t length, uint8_t buff[])
{
    int32_t lock = osKernelLock();
	bool ok = HAL_I2C_Mem_Write(&hi2c1, BL24C02_ADDRESS<<1, addr, I2C_MEMADD_SIZE_8BIT, buff, length, 10) == HAL_OK;
    /* This CMSIS wrapper nests a suspension on RestoreLock(1). */
    if (lock == 0) osKernelRestoreLock(0);
    return ok;
}

static bool BL24C02_Read(uint8_t addr, uint8_t length, uint8_t buff[])
{
    int32_t lock = osKernelLock();
	bool ok = HAL_I2C_Mem_Read(&hi2c1, BL24C02_ADDRESS<<1, addr, I2C_MEMADD_SIZE_8BIT, buff, length, 10) == HAL_OK;
    if (lock == 0) osKernelRestoreLock(0);
    return ok;
}

static void delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}

/* Every transaction is serialized with the existing I2C users. Splitting at
 * eight-byte boundaries supports either EEPROM page size used by the board. */
static bool WriteVerified(uint8_t address, const void *data, uint8_t length)
{
	uint8_t verify[sizeof(LegacyBackup_T)];
	if(data == NULL || length == 0U || length > sizeof(verify) ||
	   (uint16_t)address + length > 256U) return false;
	int32_t lock = osKernelLock();
	bool ok = true;
	const uint8_t *bytes = data;
	for(uint8_t offset = 0U; offset < length;) {
		uint8_t chunk = 8U - ((address + offset) & 7U);
		if(chunk > length - offset) chunk = length - offset;
		if(!BL24C02_Write(address + offset, chunk, (uint8_t *)bytes + offset)) {
			ok = false;
			break;
		}
		delay_ms(6U);
		offset += chunk;
	}
	if(ok) {
		ok = BL24C02_Read(address, length, verify) &&
		     memcmp(bytes, verify, length) == 0;
	}
	if(lock == 0) osKernelRestoreLock(0);
	return ok;
}

static uint32_t BackupChecksum(const LegacyBackup_T *backup)
{
	const uint8_t *bytes = (const uint8_t *)backup;
	uint32_t checksum = 2166136261U;
	for(unsigned i = 0; i < offsetof(LegacyBackup_T, checksum); ++i)
		checksum = (checksum ^ bytes[i]) * 16777619U;
	return checksum;
}

static bool LegacySettingsValid(const uint8_t settings[6])
{
	uint16_t rotation = (uint16_t)settings[3] | ((uint16_t)settings[4] << 8);
	return settings[0] <= 100U && settings[1] <= 1U && settings[2] <= 1U &&
	       (rotation == 0U || rotation == 90U ||
	        rotation == 180U || rotation == 270U) &&
	       settings[5] <= GATE_MODE_HIGH;
}

static bool LoadBackup(LegacyBackup_T *backup)
{
	return BL24C02_Read(LEGACY_BACKUP_ADDRESS, sizeof(*backup), (uint8_t *)backup) &&
	       backup->magic == LEGACY_BACKUP_MAGIC &&
	       backup->checksum == BackupChecksum(backup) &&
	       LegacySettingsValid(backup->settings) &&
	       ADC_Calibration_IsValid(&backup->calibration);
}

static void LoadLegacyRuntime(const LegacyBackup_T *backup)
{
	sys_settings.backlight_level = backup->settings[0];
	sys_settings.key_sound_enable = backup->settings[1];
	sys_settings.language_select = backup->settings[2];
	sys_settings.rotation = (uint16_t)backup->settings[3] |
	                       ((uint16_t)backup->settings[4] << 8);
	sys_settings.current_range_mode = backup->settings[5];
	adc_calibration = backup->calibration;
	calibration_initialized = true;
}

/******************************************
EEPROM Data description:
[0x00]: 0x55 for check
[0x01]: 0xAA for check

[0x20-]: update command storage area, "update\r\n"
[0x40-]: upstream 1.1.7 SysSettings_T (settings + calibration)
*******************************************/

// to check the Data is right and the EEPROM is working, 0-ok, 1-erro
uint8_t EEPROM_Init_Check(void)
{
	uint8_t check_buff[2] = {0};
	delay_ms(10);
	if(!BL24C02_Read(0,2,check_buff)) return 1U;
	if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
	{
		return 0;//check OK
	}
	else
	{
		check_buff[0] = 0x55;
		check_buff[1] = 0xAA;
		delay_ms(10);
		if(!BL24C02_Write(0,2,check_buff)) return 1U;
		memset(check_buff,0,2);
		delay_ms(10);
		if(!BL24C02_Read(0,2,check_buff)) return 1U;
		if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
			return 0;//check ok
	}
	return 1;//check erro
}


static void RuntimeToStorage(SysSettingsStorage_T *storage)
{
	SysSettings_T settings;
	ADC_Calibration_t calibration;
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	settings = sys_settings;
	calibration = adc_calibration;
	__set_PRIMASK(primask);
	memset(storage, 0, sizeof(*storage));
	storage->backlight_level = settings.backlight_level;
	storage->key_sound_enable = settings.key_sound_enable;
	storage->language_select = settings.language_select;
	storage->rotation = settings.rotation;
	storage->current_range_mode = settings.current_range_mode;
	storage->low_scale_multiplier =
		calibration.low_scale_ua_per_lsb / SCALE_LOW;
	storage->mid_scale_multiplier =
		calibration.mid_scale_ua_per_lsb / SCALE_MID;
	storage->high_scale_multiplier =
		calibration.high_scale_ua_per_lsb / SCALE_HIGH;
	storage->low_offset_ua = calibration.low_offset_ua;
	storage->mid_offset_ua = calibration.mid_offset_ua;
	storage->high_offset_ua = calibration.high_offset_ua;
}

static bool StoredRangeValid(float scale, float multiplier, float offset)
{
	if(!isfinite(multiplier) || !isfinite(offset) ||
	   multiplier < 0.75f || multiplier > 1.25f) return false;
	float offset_lsb = offset / (scale * multiplier);
	return isfinite(offset_lsb) && fabsf(offset_lsb) <= 32.0f;
}

static bool StorageIsValid(const SysSettingsStorage_T *storage)
{
	if(storage == NULL ||
	   storage->backlight_level > 100U ||
	   storage->key_sound_enable > 1U ||
	   storage->language_select > 1U ||
	   (storage->rotation != 0U && storage->rotation != 90U &&
	    storage->rotation != 180U && storage->rotation != 270U) ||
	   (storage->current_range_mode != GATE_MODE_AUTO &&
	    storage->current_range_mode != GATE_MODE_LOW &&
	    storage->current_range_mode != GATE_MODE_MID &&
	    storage->current_range_mode != GATE_MODE_HIGH)) {
		return false;
	}
	return StoredRangeValid(SCALE_LOW, storage->low_scale_multiplier,
	                        storage->low_offset_ua) &&
	       StoredRangeValid(SCALE_MID, storage->mid_scale_multiplier,
	                        storage->mid_offset_ua) &&
	       StoredRangeValid(SCALE_HIGH, storage->high_scale_multiplier,
	                        storage->high_offset_ua);
}

static void StorageToRuntime(const SysSettingsStorage_T *storage)
{
	sys_settings.backlight_level = storage->backlight_level;
	sys_settings.key_sound_enable = storage->key_sound_enable;
	sys_settings.language_select = storage->language_select;
	sys_settings.rotation = storage->rotation;
	sys_settings.current_range_mode = storage->current_range_mode;
	adc_calibration.low_scale_ua_per_lsb =
		storage->low_scale_multiplier * SCALE_LOW;
	adc_calibration.mid_scale_ua_per_lsb =
		storage->mid_scale_multiplier * SCALE_MID;
	adc_calibration.high_scale_ua_per_lsb =
		storage->high_scale_multiplier * SCALE_HIGH;
	adc_calibration.low_offset_ua = storage->low_offset_ua;
	adc_calibration.mid_offset_ua = storage->mid_offset_ua;
	adc_calibration.high_offset_ua = storage->high_offset_ua;
}

static bool SaveStorage(void)
{
	SysSettingsStorage_T storage;
	int32_t lock = osKernelLock();
	RuntimeToStorage(&storage);
	bool ok = StorageIsValid(&storage) &&
	          WriteVerified(EEPROM_SYS_SETTINGS_ADDRESS, &storage, sizeof(storage));
	if(lock == 0) osKernelRestoreLock(0);
	return ok;
}

// Save settings and calibration in the upstream 1.1.7 layout at 0x40.
bool EEPROM_SysSetting_Save(void)
{
	if(migration_pending) return false;
	return SaveStorage();
}

// Eight-byte chunks support both older 8-byte and newer 16-byte-page parts.
bool EEPROM_AdcCalibration_Save(const ADC_Calibration_t *calibration)
{
	SysSettingsStorage_T storage;
	if(calibration == NULL || !ADC_Calibration_IsValid(calibration)) {
		return false;
	}
	if(migration_pending) return false;
	RuntimeToStorage(&storage);
	storage.low_scale_multiplier = calibration->low_scale_ua_per_lsb / SCALE_LOW;
	storage.mid_scale_multiplier = calibration->mid_scale_ua_per_lsb / SCALE_MID;
	storage.high_scale_multiplier = calibration->high_scale_ua_per_lsb / SCALE_HIGH;
	storage.low_offset_ua = calibration->low_offset_ua;
	storage.mid_offset_ua = calibration->mid_offset_ua;
	storage.high_offset_ua = calibration->high_offset_ua;
	return StorageIsValid(&storage) &&
	       WriteVerified(EEPROM_SYS_SETTINGS_ADDRESS, &storage, sizeof(storage));
}

// Use hardware scale defaults if the EEPROM read or record validation fails.
void EEPROM_AdcCalibration_Get(ADC_Calibration_t *calibration)
{
	SysSettingsStorage_T storage;
	if(calibration == NULL) {
		return;
	}
	bool ok = BL24C02_Read(EEPROM_SYS_SETTINGS_ADDRESS, sizeof(storage),
	                      (uint8_t *)&storage);
	if(ok && StorageIsValid(&storage)) {
		calibration->low_scale_ua_per_lsb = storage.low_scale_multiplier * SCALE_LOW;
		calibration->mid_scale_ua_per_lsb = storage.mid_scale_multiplier * SCALE_MID;
		calibration->high_scale_ua_per_lsb = storage.high_scale_multiplier * SCALE_HIGH;
		calibration->low_offset_ua = storage.low_offset_ua;
		calibration->mid_offset_ua = storage.mid_offset_ua;
		calibration->high_offset_ua = storage.high_offset_ua;
	} else {
		ADC_Calibration_SetDefault(calibration);
	}
}


// to Get the settings
void EEPROM_SysSetting_Get(void)
{
	SysSettingsStorage_T storage;
	migration_pending = false;
	ADC_Calibration_SetDefault(&adc_calibration);
	calibration_initialized = true;

	/* A failed read must never be treated as proof of an old layout. */
	if(!BL24C02_Read(EEPROM_SYS_SETTINGS_ADDRESS,
	                sizeof(storage), (uint8_t *)&storage)) return;
	if(StorageIsValid(&storage)) {
		StorageToRuntime(&storage);
		return;
	}

	/* An interrupted first migration can recover from its verified backup.
	 * Existing upstream records are preferred and never overwritten by it. */
	if(!LoadBackup(&legacy_backup)) {
		memset(&legacy_backup, 0, sizeof(legacy_backup));
		legacy_backup.magic = LEGACY_BACKUP_MAGIC;
		if(!BL24C02_Read(0x10, sizeof(legacy_backup.settings),
		                 legacy_backup.settings)) return;
		memcpy(&legacy_backup.calibration, &storage, sizeof(ADC_Calibration_t));
		if(!LegacySettingsValid(legacy_backup.settings) ||
		   !ADC_Calibration_IsValid(&legacy_backup.calibration)) return;
		legacy_backup.checksum = BackupChecksum(&legacy_backup);
	}
	LoadLegacyRuntime(&legacy_backup);
	migration_pending = true;
}

/* Recovery image only: restore the exact pre-migration record. */
bool EEPROM_RestoreLegacyBaseline(void)
{
	LegacyBackup_T backup;
	if(!LoadBackup(&backup)) return false;
	if(!WriteVerified(0x10, backup.settings, sizeof(backup.settings)) ||
	   !WriteVerified(0x40, &backup.calibration, sizeof(backup.calibration)))
		return false;
	LoadLegacyRuntime(&backup);
	migration_pending = false;
	return true;
}

// to write the update command
void EEPROM_UpdateCommand_Write(bool is_update)
{
	if(is_update) {
		char cmd[] = "update\r\n";
		BL24C02_Write(0x20, strlen(cmd), (uint8_t *)cmd);
	} else {
		char cmd[] = "-nope-\r\n";
		BL24C02_Write(0x20, strlen(cmd), (uint8_t *)cmd);
	}
}

// to check the update command
bool EEPROM_UpdateCommand_Check(void)
{
	char cmd[10] = {0};
	BL24C02_Read(0x20, 8, (uint8_t *)cmd);
	if(strcmp(cmd, "update\r\n") == 0) {
		return true;
	} else {
		return false;
	}
}

// Set functions

void Sys_Set_BacklightLevel(uint8_t level)
{
	if(level <= 100 && level >= 0) {
		sys_settings.backlight_level = level;
		LCD_Set_Light(level);
	}
}

void Sys_Set_KeySoundEnable(bool enable)
{
	if(enable) {
		sys_settings.key_sound_enable = 1;
	} else {
		sys_settings.key_sound_enable = 0;
	}
}

void Sys_Set_LanguageSelect(uint8_t lang)
{
	if(lang <= 1) {
		sys_settings.language_select = lang;
	}
}

void Sys_Set_Rotation(uint16_t rotation)
{
	if(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270) {
		sys_settings.rotation = rotation;
		LCD_SetRotation(rotation);
	}
}

void Sys_Set_CurrentRangeMode(uint8_t mode)
{
	if(mode == GATE_MODE_AUTO || mode == GATE_MODE_LOW || mode == GATE_MODE_MID || mode == GATE_MODE_HIGH) {
		sys_settings.current_range_mode = mode;
		Gate_Set_Mode(mode);
	}
}

bool Sys_Set_AdcCalibration(const ADC_Calibration_t *calibration)
{
	if(!ADC_Calibration_IsValid(calibration)) {
		return false;
	}

	if(!EEPROM_AdcCalibration_Save(calibration)) {
		return false;
	}

	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	adc_calibration = *calibration;
	__set_PRIMASK(primask);

	return true;
}

void Sys_AdcCalibration_Init(void)
{
	if(!calibration_initialized) EEPROM_SysSetting_Get();
	/* Defer the one-time migration writes until after the UI has started.
	 * Never overwrite the active old record before a verified backup exists. */
	if(migration_pending) {
		LegacyBackup_T existing;
		if(!LoadBackup(&existing) &&
		   !WriteVerified(LEGACY_BACKUP_ADDRESS, &legacy_backup,
		                  sizeof(legacy_backup))) return;
		if(SaveStorage()) migration_pending = false;
	}
}

// Get functions

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
	if(calibration == NULL) {
		return;
	}

	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	*calibration = adc_calibration;
	__set_PRIMASK(primask);
}
