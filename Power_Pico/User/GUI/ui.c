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
static void main_timer_cb(lv_timer_t * timer)
{
    // do nothing
}

////////////////////////// Animation //////////////////////

void ui_start_animation_done() {
    // 初始化页面并加载主要界面
    lv_lib_pm_Init();
    for (uint8_t i = 0; i < sizeof(pages) / sizeof(pages[0]); i++) {
        lv_lib_pm_register(&pages[i]);
    }
    lv_lib_pm_load_init_screen();
}

void ui_anima_set_light_level(void * var, int32_t v) {
    ui_set_back_light_level(v);
}

static void _ui_Start_animation(void) {
    uint8_t target_level = ui_get_back_light_level();
    lv_lib_anim_user_animation(NULL, 0, 2000, 0, target_level, 0, 0, 0, 0, lv_anim_path_ease_in, ui_anima_set_light_level, ui_start_animation_done);
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

    // main timer (you can add someting to do in the timer_cb if needed)
    lv_timer_t * ui_MainTimer = lv_timer_create(main_timer_cb, 1000,  NULL);

    // start up, just load one time only
    ui_StartPage_screen_init();
    lv_scr_load(ui_StartPage);
    _ui_Start_animation();
}
