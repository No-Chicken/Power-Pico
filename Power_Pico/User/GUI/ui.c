// LVGL version: 9.2
// Project name: PowerPico

#include "BL24C02.h" // system settings
#include "./ui.h"
#include "./screens/ui_mainPage.h"
#include "./screens/ui_chartPage.h"
#include "./screens/ui_SetPage.h"

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
    // do nothing
    // LV_LOG_WARN("Main timer");
}

/////////////////////// ui_initialize //////////////////////
void ui_init(void)
{
    lv_disp_t * dispp = lv_display_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    PageManager_init();
    for(uint8_t i = 0; i < sizeof(pages)/sizeof(pages[0]); i++) {
        PageManager_register(&pages[i]);
    }
    PageManager_load_init_screen();
    lv_display_set_rotation(dispp, Sys_Get_Rotation() / 90);

    //timer
    // lv_timer_t * ui_MainTimer = lv_timer_create(main_timer, 1000,  NULL);
}
