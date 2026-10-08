// LVGL VERSION: 9.2


#ifndef _UI_H
#define _UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "lvgl.h"
#include "ui_theme.h"
#include "./common/lv_lib_pm.h"
#include "./common/lv_lib_animation.h"
#include "./common/lv_i18n.h"
#include "./ui_helpers.h"
#include "version.h"

void ui_init(void);

// FONTS
LV_FONT_DECLARE(ui_font_HeiTi32);
LV_FONT_DECLARE(ui_font_HeiTi48);
LV_FONT_DECLARE(ui_font_zhongyuan18);
LV_FONT_DECLARE(ui_font_zhongyuan20);
LV_FONT_DECLARE(ui_font_fixed_voltage_control16);
const lv_font_t *ui_round_font18(void);
#if POWER_PICO_UI_MODERN
LV_FONT_DECLARE(ui_font_modern14);
LV_FONT_DECLARE(ui_font_modern16);
LV_FONT_DECLARE(ui_font_instrument35);
LV_FONT_DECLARE(ui_font_instrument17);
LV_FONT_DECLARE(ui_font_title17);
LV_FONT_DECLARE(ui_font_menu_bold17);
LV_FONT_DECLARE(ui_font_home_label18);
#endif

// IMAGES AND IMAGE SETS
LV_IMG_DECLARE(ui_img_chicken96_png);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
