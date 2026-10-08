#ifndef UI_POWER_PAGE_H
#define UI_POWER_PAGE_H

#include "lvgl.h"
extern lv_obj_t *ui_PowerPage;
void ui_PowerPage_screen_init(void);
void ui_PowerPage_screen_destroy(void);
void ui_power_page_key_handler(void *event);

#endif
