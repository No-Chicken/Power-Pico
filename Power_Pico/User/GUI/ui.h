// LVGL VERSION: 9.2


#ifndef _UI_H
#define _UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#include "stdio.h"

#include "./PageManager.h"

void ui_init(void);
void ui_full_screen_refresh(lv_obj_t * screen);

// FONTS
LV_FONT_DECLARE(ui_font_HeiTi32);
LV_FONT_DECLARE(ui_font_HeiTi48);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
