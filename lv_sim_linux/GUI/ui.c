// LVGL version: 9.2
// Project name: PowerPico

#include "./ui.h"
#include "./screens/ui_mainPage.h"

///////////////////// VARIABLES ////////////////////

// pages
Page_t pages[] = {
    {
        .init = ui_main_screen_init,
        .deinit = ui_main_screen_destroy,
        .page_obj = &ui_HomeScreen,
        .name = "Main Page"
    },
    // 可以在这里添加更多页面
};

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 32
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif

/////////////////////// Timer //////////////////////
/**
 * Main timer for Refreshing the screens
 */
static void main_timer(lv_timer_t * timer)
{
    LV_LOG_WARN("Main timer");
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

    //timer
    lv_timer_t * ui_MainTimer = lv_timer_create(main_timer, 1000,  NULL);
}
