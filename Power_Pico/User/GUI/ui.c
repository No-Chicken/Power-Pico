// LVGL version: 9.2
// Project name: PowerPico

#include "BL24C02.h" // system settings
#include "rtc.h"     // elapsed time
#include "./ui.h"
#include "./screens/ui_StartPage.h"
#include "./screens/ui_mainPage.h"
#include "./screens/ui_chartPage.h"
#include "./screens/ui_SetPage.h"


//////////// interface for system hw settings ///////////

// get functions

void ui_GetElapsedTime_HMS(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    GetElapsedTime_HMS(hours, minutes, seconds);
}

uint8_t ui_get_back_light_level(void) {
    return Sys_Get_BacklightLevel();
}

bool ui_get_key_sound_enable(void) {
    return Sys_Get_KeySoundEnable();
}

uint16_t ui_get_display_rotation(void) {
    return Sys_Get_Rotation();
}

// set functions

void ui_set_back_light_level(uint8_t level) {
    Sys_Set_BacklightLevel(level);
}

void ui_set_key_sound_enable(bool enable) {
    Sys_Set_KeySoundEnable(enable);
}

void ui_set_display_rotation(uint16_t rotation) {
    Sys_Set_Rotation(rotation);
}

// sys save function
void ui_system_settings_save(void) {
    EEPROM_SysSetting_Save();
}

///////////////////// VARIABLES ////////////////////

// pages
Page_t pages[] = {
    {
        .init = ui_main_screen_init,
        .deinit = ui_main_screen_destroy,
        .page_obj = &ui_HomeScreen,
        .key_event_handler = ui_main_page_key_handler,
        .name = "Main Page"
    },
    // {
    //     .init = ui_ChartPage_screen_init,
    //     .deinit = ui_ChartPage_screen_destroy,
    //     .page_obj = &ui_ChartPage,
    //     .key_event_handler = ui_chart_page_key_handler,
    //     .name = "Chart Page"
    // },
    {
        .init = ui_SetPage_screen_init,
        .deinit = ui_SetPage_screen_destroy,
        .page_obj = &ui_SetPage,
        .key_event_handler = ui_set_page_key_handler,
        .name = "Set Page"
    },
    // 可以在这里添加更多页面
};

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif

///////////////////// help funtions ////////////////////

void ui_full_screen_refresh(lv_obj_t * screen) {
    // 标记整个屏幕为脏区域
    lv_obj_invalidate(screen);
    // 或者立即刷新整个屏幕
    lv_refr_now(NULL);
}

/////////////////////// Timer //////////////////////

/**
 * Main timer for Refreshing the screens
 */
static void main_timer(lv_timer_t * timer)
{
    static uint8_t first_time_in = 0;
    static uint8_t count = 0;
    static uint8_t backlight_level = 0; // 初始亮度为 0%
    static uint8_t target_backlight_level = 0; // 目标亮度

    if (first_time_in == 0) {
        // 获取目标亮度
        target_backlight_level = ui_get_back_light_level();
        first_time_in = 1;
    }

    // 调整背光亮度
    if (backlight_level < target_backlight_level) {
        backlight_level++;
        ui_set_back_light_level(backlight_level); // 设置当前亮度
    } else {
        // 初始化页面并加载主要界面
        lv_lib_pm_Init();
        for (uint8_t i = 0; i < sizeof(pages) / sizeof(pages[0]); i++) {
            lv_lib_pm_register(&pages[i]);
        }
        lv_lib_pm_load_init_screen();

        // 停止定时器，避免重复初始化
        lv_timer_del(timer);
    }
}

/////////////////////// ui_initialize //////////////////////
void ui_init(void)
{
    lv_disp_t * dispp = lv_display_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    // set system settings
    ui_set_display_rotation(ui_get_display_rotation());

    // timer
    lv_timer_t * ui_MainTimer = lv_timer_create(main_timer, 50,  NULL);

    // start up, just load one time only
    ui_StartPage_screen_init();
    lv_scr_load(ui_StartPage);
}
