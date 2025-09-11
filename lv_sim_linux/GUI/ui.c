// LVGL version: 9.2
// Project name: PowerPico

#include "./ui.h"

///////////////////// VARIABLES ////////////////////

// pages


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
    LV_LOG_INFO("Main timer");
}

/////////////////////// ui_initialize //////////////////////
void ui_init(void)
{
    lv_disp_t * dispp = lv_display_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    PageManager_init();

    // PageManager_register(&xxxpage);
    // test
    lv_obj_t * test_page = lv_obj_create(NULL);
    lv_obj_t * btn1 = lv_button_create(test_page);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, 0);
    lv_disp_load_scr(test_page);

    //timer
    lv_timer_t * ui_MainTimer = lv_timer_create(main_timer, 1000,  NULL);
}
