// LVGL VERSION: 9.2


#ifndef _UI_H
#define _UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "lvgl.h"
#include "./common/lv_lib_pm.h"
#include "./common/lv_lib_animation.h"
#include "./common/lv_i18n.h"
#include "./ui_helpers.h"

void ui_init(void);

// FONTS
LV_FONT_DECLARE(ui_font_HeiTi32);
LV_FONT_DECLARE(ui_font_HeiTi48);
LV_FONT_DECLARE(ui_font_zhongyuan18);
LV_FONT_DECLARE(ui_font_zhongyuan20);

// IMAGES AND IMAGE SETS
LV_IMG_DECLARE(ui_img_chicken_png);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
