#ifndef UI_fixed_voltage_control_PAGE_H
#define UI_fixed_voltage_control_PAGE_H
#include "lvgl.h"
extern lv_obj_t *ui_FixedVoltagePage;
void ui_FixedVoltagePage_screen_init(void);
void ui_FixedVoltagePage_screen_destroy(void);
void ui_fixed_voltage_control_key_handler(void *event);
lv_obj_t *ui_power_control_init(bool pps);
void ui_power_control_destroy(void);
void ui_power_control_key(void *event);
#endif
