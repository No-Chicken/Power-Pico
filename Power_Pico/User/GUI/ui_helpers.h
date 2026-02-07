// LVGL VERSION: 9.2


#ifndef _UI_HELPERS_H
#define _UI_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "stdio.h"

///////////////////// help funtions ////////////////////

void ui_full_screen_refresh(lv_obj_t * screen);

///////////// interface for system settings ////////////

// get functions

uint64_t ui_GetElaspseMicroseconds(void);
void ui_GetElapsedTime_HMS(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
uint8_t ui_get_back_light_level(void);
bool ui_get_key_sound_enable(void);
uint8_t ui_get_language_select(void);
uint16_t ui_get_display_rotation(void);
void ui_get_vol_cur(float *voltage, float *current);

// set functions

void ui_clear_microsecond_counter(void);
void ui_set_back_light_level(uint8_t level);
void ui_set_key_sound_enable(bool enable);
void ui_set_language_select(uint8_t lang);
void ui_set_display_rotation(uint16_t rotation);
void ui_clear_data_monitor(void);

// sys save function
void ui_system_settings_save(void);

///////////////////////////////////////////////////////////////


///////////// interface for com with PD UFP Task //////////////

void ui_send_pps_start_msg(void);
void ui_send_pps_stop_msg(void);
void ui_send_pps_set_msg(float voltage, float current);
int8_t MsgQueueGet_PPS_ready(void);

///////////////////////////////////////////////////////////////


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
