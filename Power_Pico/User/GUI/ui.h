// LVGL VERSION: 9.2


#ifndef _UI_H
#define _UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#include "stdio.h"

#include "./common/lv_lib_pm.h"
#include "./common/lv_lib_animation.h"

void ui_init(void);

///////////////////// help funtions ////////////////////

void ui_full_screen_refresh(lv_obj_t * screen);

///////////// interface for system settings ////////////

// get functions

void ui_GetElapsedTime_HMS(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
uint8_t ui_get_back_light_level(void);
bool ui_get_key_sound_enable(void);
uint16_t ui_get_display_rotation(void);

// set functions

void ui_set_back_light_level(uint8_t level);
void ui_set_key_sound_enable(bool enable);
void ui_set_display_rotation(uint16_t rotation);

// sys save function
void ui_system_settings_save(void);

// FONTS
LV_FONT_DECLARE(ui_font_HeiTi32);
LV_FONT_DECLARE(ui_font_HeiTi48);

// IMAGES AND IMAGE SETS
LV_IMG_DECLARE(ui_img_chicken_png);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
