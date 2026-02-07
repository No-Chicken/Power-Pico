// LVGL version: 9.2
// Project name: PowerPico

#include "BL24C02.h" // system settings
#include "tim.h"     // elapsed time
#include "adc.h"     //  for voltage/current minitor
#include "user_PDUFPTask.h" // comunicate with PD UFP Task

#include "./ui_helpers.h"

///////////////////// ui help funtions ////////////////////

void ui_full_screen_refresh(lv_obj_t * screen) {
    // 标记整个屏幕为脏区域
    lv_obj_invalidate(screen);
    // 或者立即刷新整个屏幕
    lv_refr_now(NULL);
}

//////////// interface for system hw settings ///////////

//////////////// get functions ///////////////

uint64_t ui_GetElaspseMicroseconds(void) {
    return GetMicrosecondCounter();
}

// 获取经过的时间, 单位为时分秒, 必须定时运行以更新32位计数器溢出
void ui_GetElapsedTime_HMS(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {

    uint64_t total_us = GetMicrosecondCounter();
    uint32_t total_seconds = total_us / 1000000;

    if (hours) {
        *hours = (total_seconds / 3600); // 这里不取模，可以显示超过24小时
    }
    if (minutes) {
        *minutes = (total_seconds % 3600) / 60;
    }
    if (seconds) {
        *seconds = total_seconds % 60;
    }
}

uint8_t ui_get_back_light_level(void) {
    return Sys_Get_BacklightLevel();
}

bool ui_get_key_sound_enable(void) {
    return Sys_Get_KeySoundEnable();
}

uint8_t ui_get_language_select(void) {
    return Sys_Get_LanguageSelect();
}

uint16_t ui_get_display_rotation(void) {
    return Sys_Get_Rotation();
}

// 获取当前电压, 单位 V; 当前电流, 单位 uA
void ui_get_vol_cur(float *voltage, float *current) {
    Data_Monitor_Get_Values(voltage, current);
}

//////////////// set functions ///////////////

void ui_clear_microsecond_counter(void) {
    ClearMicrosecondCounter();
}

void ui_set_back_light_level(uint8_t level) {
    Sys_Set_BacklightLevel(level);
}

void ui_set_key_sound_enable(bool enable) {
    Sys_Set_KeySoundEnable(enable);
}

void ui_set_language_select(uint8_t lang) {
    Sys_Set_LanguageSelect(lang);
}

void ui_set_display_rotation(uint16_t rotation) {
    Sys_Set_Rotation(rotation);
}

void ui_clear_data_monitor(void) {
    Data_Monitor_Clear();
}

//////////////// sys save functions ///////////////
void ui_system_settings_save(void) {
    EEPROM_SysSetting_Save();
}

///////////// interface for com with PD UFP Task //////////////

void ui_send_pps_start_msg(void) {
    PD_command_msg_t pd_ui_msg;
    pd_ui_msg.event = PD_CMD_START;
    osMessageQueuePut(PD_cmd_MessageQueue, &pd_ui_msg, 0, 1);
}

void ui_send_pps_stop_msg(void) {
    PD_command_msg_t pd_ui_msg;
    pd_ui_msg.event = PD_CMD_STOP;
    osMessageQueuePut(PD_cmd_MessageQueue, &pd_ui_msg, 0, 1);
}

void ui_send_pps_set_msg(float voltage, float current) {
    PD_command_msg_t pd_ui_msg;
    pd_ui_msg.event = PD_CMD_SET_PPS;
    pd_ui_msg.set_voltage = voltage;
    pd_ui_msg.set_current = current;
    osMessageQueuePut(PD_cmd_MessageQueue, &pd_ui_msg, 0, 1);
}

// 非阻塞获取消息队列, 查看是否ready
int8_t MsgQueueGet_PPS_ready(void) {
    PD_handle_event_t pd_handle_event;
    if(osMessageQueueGet(PD_handle_event_MsgQueue, &pd_handle_event, NULL, 0)==osOK) {
        if(pd_handle_event == PD_EVT_PPS_READY) {
            return 1;
        } else if(pd_handle_event == PD_EVT_PPS_FAILED) {
            return -1;
        }
    }
    return 0;
}
