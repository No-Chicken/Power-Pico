// LVGL version: 9.2
// Project name: PowerPico

#include "./ui.h"
#include "./screens/ui_StartPage.h"
#include "./screens/ui_mainPage.h"
#include "./screens/ui_chartPage.h"
#include "./screens/ui_SetPage.h"
#include "./screens/ui_PPSPage.h"
#include "./screens/ui_PDFixedPage.h"
#include "./screens/ui_FixedVoltagePage.h"
#include "./screens/ui_PowerPage.h"
#if POWER_PICO_UI_MODERN
#include "./modern/ui_palette.h"
#endif

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
    {
        .init = ui_PPSPage_screen_init,
        .deinit = ui_PPSPage_screen_destroy,
        .page_obj = &ui_PPSPage,
        .key_event_handler = ui_pps_page_key_handler,
        .name = "PPS Page"
    },
    {
        .init = ui_PDFixedPage_screen_init,
        .deinit = ui_PDFixedPage_screen_destroy,
        .page_obj = &ui_PDFixedPage,
        .key_event_handler = ui_pdfixed_page_key_handler,
        .name = "PDFixed Page"
    },
    // 可以在这里添加更多页面
    {
        .init = ui_PowerPage_screen_init,
        .deinit = ui_PowerPage_screen_destroy,
        .page_obj = &ui_PowerPage,
        .key_event_handler = ui_power_page_key_handler,
        .name = "Power Page"
    },
    {
        .init = ui_FixedVoltagePage_screen_init,
        .deinit = ui_FixedVoltagePage_screen_destroy,
        .page_obj = &ui_FixedVoltagePage,
        .key_event_handler = ui_fixed_voltage_control_key_handler,
        .name = "Fixed Voltage Page"
    },
#if POWER_PICO_UI_MODERN
    {
        .init = ui_ThemePage_screen_init,
        .page_obj = &ui_ThemePage,
        .key_event_handler = ui_theme_page_key_handler,
        .name = "Theme Page"
    },
#endif
};

///////////////////// UI configuration checks ////////////////////
_Static_assert(sizeof(pages) / sizeof(pages[0]) <= MAX_PAGES,
               "Page registry capacity is too small");
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif

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

#if POWER_PICO_UI_MODERN
static void modern_start_done(lv_timer_t *timer)
{
    (void)timer;
    ui_start_animation_done();
}
#endif

/////////////////////// ui_initialize //////////////////////
void ui_init(void)
{
    lv_disp_t * dispp = lv_display_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    // set system settings
    ui_set_display_rotation(ui_get_display_rotation());

    // internationalization init
    lv_i18n_init(lv_i18n_language_pack); //lv_i18n 的语言初始化
    if(ui_get_language_select() == 0)
        lv_i18n_set_locale("en");      //lv_i18n 设置当前语言
    else
        lv_i18n_set_locale("zh-cn");  //lv_i18n 设置当前语言

    // start up, just load one time only
    ui_StartPage_screen_init();
    lv_screen_load(ui_StartPage);
#if POWER_PICO_UI_MODERN
    ui_set_back_light_level(ui_get_back_light_level());
    lv_timer_t *startup = lv_timer_create(modern_start_done, 500, NULL);
    lv_timer_set_repeat_count(startup, 1);
#else
    _ui_Start_animation();
#endif
}
