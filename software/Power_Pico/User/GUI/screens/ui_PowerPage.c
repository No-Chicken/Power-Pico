#include "../ui.h"
#include "ui_PowerPage.h"
#include "key.h"
#if POWER_PICO_UI_MODERN
#include "../modern/ui_modern.h"
#define POWER_ACCENT ui_palette_accent()
#else
#define POWER_ACCENT 0x255CF5
#endif

lv_obj_t *ui_PowerPage;
static lv_obj_t *rows[2], *captions[2], *arrows[2], *message;
static lv_timer_t *poll_timer;
static unsigned selected;
static bool waiting;

static const char *text(const char *en, const char *zh)
{
    return ui_get_language_select() ? zh : en;
}

static lv_obj_t *label(lv_obj_t *parent, const char *value, int x, int y,
                       int width, const lv_font_t *font)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, width);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(0x253342), 0);
    lv_label_set_text(o, value);
    return o;
}

static void focus(void)
{
    for (unsigned i = 0; i < 2; ++i) {
        uint32_t accent = POWER_ACCENT;
        lv_obj_set_style_bg_color(rows[i],
#if POWER_PICO_UI_MODERN
            lv_color_hex(i == selected ? accent : M_PANEL), 0);
#else
            lv_color_hex(i == selected ? accent : 0xF7F9FC), 0);
#endif
        lv_obj_set_style_text_color(captions[i],
            lv_color_hex(i == selected ? 0xFFFFFF : 0x22282F), 0);
#if POWER_PICO_UI_MODERN
        lv_obj_set_style_text_font(captions[i],
            i == selected ? &ui_font_menu_bold17 : ui_round_font18(), 0);
#endif
        lv_obj_set_style_text_color(arrows[i],
            lv_color_hex(i == selected ? 0xFFFFFF : accent), 0);
        lv_obj_set_style_border_color(rows[i],
#if POWER_PICO_UI_MODERN
            lv_color_hex(i == selected ? accent : M_CARD_LINE), 0);
#else
            lv_color_hex(i == selected ? accent : 0xDCE1E7), 0);
#endif
    }
}

static void poll(lv_timer_t *timer)
{
    (void)timer;
    if (!waiting) return;
    int8_t result = MsgQueueGet_PD_ready();
    if (!result) return;
    waiting = false;
    if (result == 1) {
        lv_lib_pm_goto("PPS Page", 0);
    } else if (result == 2) {
        lv_label_set_text(message, text("PPS not supported", "不支持 PPS"));
    } else {
        ui_send_pdsink_stop_msg();
        lv_label_set_text(message, text("Negotiation failed", "协商失败"));
        lv_obj_set_style_text_color(message, lv_color_hex(0xDC5843), 0);
    }
}

void ui_power_page_key_handler(void *event)
{
    key_event_t *key = event;
    if (key->type != KEY_EVT_CLICK || waiting) return;
    if (key->id == KEY_ID_B) {
        lv_lib_pm_goto("Set Page", 0);
    } else if (key->id == KEY_ID_L || key->id == KEY_ID_R) {
        selected ^= 1U;
        lv_label_set_text(message, "");
        focus();
    } else if (key->id == KEY_ID_Y) {
        if (selected == 0) {
            lv_lib_pm_goto("Fixed Voltage Page", 0);
        } else {
#if POWER_PICO_UI_MODERN
            lv_lib_pm_goto("PPS Page", 0);
#else
            /* Opening the menu never starts or changes a power contract. */
            while (MsgQueueGet_PD_ready() != 0) {}
            waiting = true;
            lv_label_set_text(message, text("Negotiating PPS", "等待 PPS 协商"));
            lv_obj_set_style_text_color(message, lv_color_hex(0x365DDD), 0);
            ui_send_pdsink_start_msg();
#endif
        }
    }
}

void ui_PowerPage_screen_init(void)
{
    waiting = false;
    ui_PowerPage = lv_obj_create(NULL);
    lv_obj_remove_style_all(ui_PowerPage);
    lv_obj_set_size(ui_PowerPage, 240, 240);
    lv_obj_remove_flag(ui_PowerPage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_PowerPage, lv_color_hex(0xF7F9FC), 0);
#if POWER_PICO_UI_MODERN
    lv_obj_set_style_bg_color(ui_PowerPage, lv_color_hex(M_LIST_BG), 0);
#endif
    lv_obj_set_style_bg_opa(ui_PowerPage, LV_OPA_COVER, 0);
    label(ui_PowerPage, text("Power control", "电源调节"), 12, 14, 192,
          ui_round_font18());
    label(ui_PowerPage, LV_SYMBOL_CHARGE, 212, 14, 16, &lv_font_montserrat_16);
    for (unsigned i = 0; i < 2; ++i) {
        rows[i] = lv_obj_create(ui_PowerPage);
        lv_obj_remove_style_all(rows[i]);
        lv_obj_remove_flag(rows[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(rows[i], 12, 56 + 76 * i);
        lv_obj_set_size(rows[i], 216, 64);
        lv_obj_set_style_radius(rows[i], 6, 0);
        lv_obj_set_style_border_width(rows[i], 1, 0);
        lv_obj_set_style_border_side(rows[i], LV_BORDER_SIDE_BOTTOM, 0);
#if POWER_PICO_UI_MODERN
        lv_obj_set_style_radius(rows[i], M_CARD_RADIUS, 0);
        lv_obj_set_style_border_side(rows[i], LV_BORDER_SIDE_FULL, 0);
#endif
        lv_obj_set_style_bg_opa(rows[i], LV_OPA_COVER, 0);
        captions[i] = label(rows[i], i ? text("PPS control", "PPS 调节") :
            text("Fixed voltage", "固定电压"), 16, 23, 168,
            ui_round_font18());
        arrows[i] = label(rows[i], LV_SYMBOL_RIGHT, 186, 24, 18,
                          &lv_font_montserrat_16);
    }
    message = label(ui_PowerPage, "", 12, 202, 216, ui_round_font18());
    focus();
    poll_timer = lv_timer_create(poll, 200, NULL);
}

void ui_PowerPage_screen_destroy(void)
{
    if (poll_timer) lv_timer_delete(poll_timer);
    poll_timer = NULL;
    waiting = false;
}
