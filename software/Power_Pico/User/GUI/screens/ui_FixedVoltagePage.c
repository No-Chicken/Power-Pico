#include "../ui.h"
#include "ui_FixedVoltagePage.h"
#include "user_FixedVoltage.h"
#include "key.h"
#include "pd_voltage_check.h"
#include "pico_diag.h"
#include <math.h>
#include <string.h>
#if POWER_PICO_UI_MODERN
#include "../modern/ui_palette.h"
#define CONTROL_ACCENT ui_palette_accent()
#else
#define CONTROL_ACCENT 0x255CF5
#endif

lv_obj_t *ui_FixedVoltagePage;
static lv_obj_t *screen, *meters[2], *state_label, *range_label;
static lv_obj_t *buttons[10], *values[2];
static lv_timer_t *refresh_timer;
static unsigned selected, count;
static bool pps_mode;
static uint16_t fixed_mv[7], voltage_mv, current_ma;
static unsigned fixed_count;
static uint32_t capabilities[7];
static fixed_voltage_control_status_t snapshot;
static pd_voltage_check_t voltage_check;
static uint16_t checked_mv;
static uint8_t capability_count;
static pd_voltage_result_t last_check;
static bool pps_pending, pps_submit_failed;
static uint32_t pps_edited_at;

typedef enum {
    INDICATOR_IDLE, INDICATOR_PENDING, INDICATOR_CONFIRMED, INDICATOR_FAILED
} control_indicator_t;

static uint32_t indicator_color(control_indicator_t state)
{
    return state == INDICATOR_CONFIRMED ? CONTROL_ACCENT :
           state == INDICATOR_PENDING ? CONTROL_ACCENT :
           state == INDICATOR_FAILED ? 0xDC5843 : 0xDCE1E7;
}

static control_indicator_t target_indicator(uint16_t mv, bool pps)
{
    bool target = snapshot.target_mv == mv && snapshot.pps == pps;
    bool failed = snapshot.state > fixed_voltage_control_READY || snapshot.last_error > fixed_voltage_control_READY;
    if (failed) return target ? INDICATOR_FAILED : INDICATOR_IDLE;
    if (snapshot.state == fixed_voltage_control_READY && snapshot.contract_mv == mv &&
        snapshot.contract_pps == pps && target && last_check == PD_VOLTAGE_VALID)
        return INDICATOR_CONFIRMED;
    if (target && (snapshot.state == fixed_voltage_control_ACCEPT || snapshot.state == fixed_voltage_control_POWER ||
                   (snapshot.state == fixed_voltage_control_READY && last_check == PD_VOLTAGE_WAITING)))
        return INDICATOR_PENDING;
    if (target && snapshot.state == fixed_voltage_control_READY && last_check == PD_VOLTAGE_MISMATCH)
        return INDICATOR_FAILED;
    return INDICATOR_IDLE;
}

static control_indicator_t pps_indicator(void)
{
    if (pps_submit_failed) return INDICATOR_FAILED;
    if (pps_pending) return INDICATOR_PENDING;
    /* Current is the negotiated limit, not the measured load current. */
    if (snapshot.request_current_ma != current_ma) return INDICATOR_IDLE;
    return target_indicator(voltage_mv, true);
}

static const char *text(const char *en, const char *zh)
{
    return ui_get_language_select() ? zh : en;
}

static lv_obj_t *label(lv_obj_t *parent, const char *s, int x, int y,
                       int w, const lv_font_t *font)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, font, 0);
    lv_label_set_text(o, s);
    return o;
}

static lv_obj_t *button(const char *s, int x, int y, int w)
{
    lv_obj_t *o = lv_button_create(screen);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w,
#if POWER_PICO_UI_MODERN
                    31);
    lv_obj_set_style_radius(o, 9, 0);
#else
                    28);
    lv_obj_set_style_radius(o, 6, 0);
#endif
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_t *caption = label(o, s, 0, 0, w, ui_round_font18());
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(caption);
    return o;
}

/* Use the protocol parser and intersect advertised ranges with board limits. */
static bool pps_allowed(uint16_t mv, uint16_t ma)
{
    return fixed_voltage_control_power_supported(&snapshot, mv, ma, true);
}

static void pps_default(void)
{
    if (pps_allowed(voltage_mv, current_ma)) return;
    PD_protocol_t p = {0};
    p.power_data_obj_count = snapshot.count;
    memcpy(p.power_data_obj, snapshot.pdo, sizeof(p.power_data_obj));
    for (unsigned i = 0; i < p.power_data_obj_count; ++i) {
        PD_power_info_t info;
        if (!PD_protocol_get_power_info(&p, i, &info) ||
            info.type != PD_PDO_TYPE_AUGMENTED_PDO) continue;
        uint16_t mv = info.min_v * 50U, ma = info.max_i * 10U;
        if (mv < 5000) mv = 5000;
        if (ma > 1000) ma = 1000;
        if (pps_allowed(mv, ma)) {
            voltage_mv = mv;
            current_ma = ma;
            return;
        }
    }
}

static void action_buttons(void)
{
    buttons[count++] = button(text("Reset 5V", "恢复5V"), 12, 200, 104);
    buttons[count++] = button(text("Back", "返回"), 124, 200, 104);
}

static void focus(void)
{
    for (unsigned i = 0; i < count; ++i) {
        control_indicator_t indicator = !pps_mode && i < fixed_count ?
            target_indicator(fixed_mv[i], false) : INDICATOR_IDLE;
        if (!pps_mode && i == count - 2) indicator = INDICATOR_IDLE;
#if POWER_PICO_UI_MODERN
        if (i >= count - 2) {
            bool action_focus = i == selected;
            lv_obj_set_style_bg_opa(buttons[i],
                                    action_focus ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_color(buttons[i],
                                      lv_color_hex(0xE8F0FF), 0);
            lv_obj_set_style_border_width(buttons[i], action_focus ? 2 : 0, 0);
            lv_obj_set_style_border_color(buttons[i],
                                          lv_color_hex(CONTROL_ACCENT), 0);
            lv_obj_set_style_text_color(lv_obj_get_child(buttons[i], 0),
                lv_color_hex(action_focus ? CONTROL_ACCENT : 0x71809A), 0);
            lv_obj_set_style_text_font(lv_obj_get_child(buttons[i], 0),
                action_focus ? &ui_font_menu_bold17 : ui_round_font18(), 0);
            continue;
        }
#endif
        bool confirmed_state = indicator == INDICATOR_CONFIRMED;
        bool focused_state = i == selected;
        uint32_t bg = confirmed_state ? CONTROL_ACCENT :
            focused_state ? 0xE8F0FF : 0xEEF2F8;
        lv_obj_set_style_bg_color(buttons[i],
            lv_color_hex(bg), 0);
        lv_obj_set_style_text_color(buttons[i],
            lv_color_hex(confirmed_state ? 0xFFFFFF :
                         focused_state ? CONTROL_ACCENT : 0x202D46), 0);
        lv_obj_set_style_border_width(buttons[i], 2, 0);
        lv_obj_set_style_border_color(buttons[i], lv_color_hex(
            indicator != INDICATOR_IDLE ? indicator_color(indicator) :
            focused_state ? CONTROL_ACCENT : 0xDCE1E7), 0);
    }
    if (pps_mode) {
        control_indicator_t indicator = pps_indicator();
        for (unsigned i = 0; i < 2; ++i) {
            bool confirmed_state = indicator == INDICATOR_CONFIRMED;
            lv_obj_set_style_border_color(values[i],
                                          lv_color_hex(indicator_color(indicator)), 0);
            lv_obj_set_style_bg_color(values[i],
                                      lv_color_hex(confirmed_state ? CONTROL_ACCENT : 0xFFFFFF), 0);
            lv_obj_set_style_text_color(values[i],
                lv_color_hex(confirmed_state ? 0xFFFFFF : 0x202D46), 0);
        }
    }
}

static void rebuild_fixed(void)
{
    for (unsigned i = 0; i < count; ++i) lv_obj_delete(buttons[i]);
    count = fixed_count = 0;
    PD_protocol_t p = {0};
    p.power_data_obj_count = snapshot.count;
    memcpy(p.power_data_obj, snapshot.pdo, sizeof(p.power_data_obj));
    for (unsigned i = 0; i < p.power_data_obj_count; ++i) {
        PD_power_info_t info;
        if (!PD_protocol_get_power_info(&p, i, &info) ||
            info.type != PD_PDO_TYPE_FIXED_SUPPLY || !info.max_i ||
            info.max_v < PD_V(5.0) || info.max_v > PD_V(20.0)) continue;
        uint16_t mv = info.max_v * 50U;
        bool duplicate = false;
        for (unsigned n = 0; n < fixed_count; ++n)
            if (fixed_mv[n] == mv) duplicate = true;
        if (duplicate) continue;
        char buf[16];
        snprintf(buf, sizeof(buf), "%gV", mv / 1000.0);
        fixed_mv[fixed_count] = mv;
        buttons[count] = button(buf, 12 + (count % 4) * 55,
                                104 + (count / 4) * 34, 51);
        lv_obj_set_style_text_font(lv_obj_get_child(buttons[count], 0),
#if POWER_PICO_UI_MODERN
                                  &lv_font_montserrat_18, 0);
#else
                                  &lv_font_montserrat_14, 0);
#endif
        lv_obj_set_style_border_width(buttons[count], 2, 0);
        lv_obj_set_width(lv_obj_get_child(buttons[count], 0), 47);
        lv_obj_center(lv_obj_get_child(buttons[count], 0));
        ++count;
        ++fixed_count;
    }
    bool retained = false;
    for (unsigned i = 0; i < fixed_count; ++i)
        if (fixed_mv[i] == voltage_mv) retained = true;
    if (!retained) voltage_mv = fixed_count ? fixed_mv[0] : 5000;
    action_buttons();
    selected = 0;
    focus();
}

static void refresh(lv_timer_t *timer)
{
    (void)timer;
    user_pd_power_status(&snapshot);
    if (snapshot.count > 7) snapshot.count = 0;
    float v, i;
    char buf[80];
    ui_get_vol_cur(&v, &i);
    snprintf(buf, sizeof(buf), isfinite(v) && fabsf(v) < 100 ? "%.2f V" : "-- V", v);
    lv_label_set_text(meters[0], buf);
    snprintf(buf, sizeof(buf), isfinite(i) && fabsf(i) < 10000000 ? "%.3f A" : "-- A",
             i / 1000000.0f);
    lv_label_set_text(meters[1], buf);
    if (capability_count != snapshot.count ||
        memcmp(capabilities, snapshot.pdo, sizeof(capabilities)) != 0) {
        pps_pending = false;
        capability_count = snapshot.count;
        memcpy(capabilities, snapshot.pdo, sizeof(capabilities));
        if (!pps_mode) rebuild_fixed();
        else pps_default();
    }
    if (pps_mode && pps_pending) {
        uint32_t elapsed = lv_tick_elaps(pps_edited_at);
        /* Coalesce edits, never overlap negotiations or retry failed requests. */
        if (!pps_allowed(voltage_mv, current_ma) ||
            snapshot.state > fixed_voltage_control_READY || snapshot.last_error > fixed_voltage_control_READY ||
            elapsed >= 2400) {
            pps_pending = false;
            pps_submit_failed = true;
        } else if (elapsed >= 400 && snapshot.state == fixed_voltage_control_READY) {
            pps_pending = false;
            bool unchanged = snapshot.contract_pps &&
                snapshot.contract_mv == voltage_mv && snapshot.request_current_ma == current_ma;
            if (!unchanged) {
                pps_submit_failed = !user_pd_power_request(voltage_mv, current_ma, true);
                if (!pps_submit_failed) {
                    voltage_check = (pd_voltage_check_t){0};
                    user_pd_power_status(&snapshot);
                }
            }
        }
    }
    if (pps_mode) {
        snprintf(buf, sizeof(buf), "%.2f V", voltage_mv / 1000.0f);
        lv_label_set_text(values[0], buf);
        snprintf(buf, sizeof(buf), "%.2f A", current_ma / 1000.0f);
        lv_label_set_text(values[1], buf);
    }
    snprintf(buf, sizeof(buf), "CC%u  %s %.2fV", snapshot.cc,
             snapshot.contract_pps ? "PPS" : "PD", snapshot.contract_mv / 1000.0f);
    lv_label_set_text(range_label, buf);
    if (checked_mv != snapshot.target_mv) {
        checked_mv = snapshot.target_mv;
        voltage_check = (pd_voltage_check_t){0};
    }
    pd_voltage_result_t check = pd_voltage_check_target(&voltage_check,
        snapshot.state == fixed_voltage_control_READY, v, snapshot.target_mv / 1000.0f, lv_tick_get());
    if (check != last_check) {
        pico_diag_log(DIAG_VOLTAGE, isfinite(v) && v >= 0 && v < 100 ?
                      (uint32_t)(v * 1000.0f) : UINT32_MAX);
        pico_diag_log(DIAG_PD_VOLTAGE_CHECK, check);
        last_check = check;
    }
    const char *s = text("Reading capabilities", "读取能力");
    bool error = false;
    if (snapshot.state == fixed_voltage_control_READY) {
        s = check == PD_VOLTAGE_VALID ? text("Voltage confirmed", "电压已确认") :
            check == PD_VOLTAGE_WAITING ? text("Checking voltage", "等待实测电压") :
            text("Voltage mismatch", "实测电压不符");
        error = check == PD_VOLTAGE_MISMATCH;
    } else if (snapshot.state == fixed_voltage_control_ACCEPT || snapshot.state == fixed_voltage_control_POWER) {
        s = text("Negotiating", "正在协商");
    } else if (snapshot.state > fixed_voltage_control_READY) {
        error = true;
        s = snapshot.state == fixed_voltage_control_REJECTED ? text("Request rejected", "请求被拒绝") :
            snapshot.state == fixed_voltage_control_NO_9V ? text("Unsupported setting", "不支持此设置") :
            snapshot.state == fixed_voltage_control_BUSY ? text("PD busy", "PD 已占用") :
            text("Negotiation failed", "协商失败");
    }
    if (pps_mode && snapshot.count && !pps_allowed(voltage_mv, current_ma)) {
        s = text("PPS not supported", "不支持此 PPS 设置");
        error = true;
    } else if (pps_mode && pps_submit_failed) {
        s = text("Busy / unavailable", "忙或设置不支持");
        error = true;
    } else if (pps_mode && pps_pending) {
        s = text("Pending request", "等待请求");
    }
    if (snapshot.last_error > fixed_voltage_control_READY) {
        s = snapshot.last_error == fixed_voltage_control_REJECTED ? text("Request rejected", "请求被拒绝") :
            text("Negotiation failed", "协商失败");
        error = true;
    }
    lv_label_set_text(state_label, s);
    lv_obj_set_style_text_color(state_label,
        lv_color_hex(error ? 0xDC5843 :
            (pps_pending || snapshot.state == fixed_voltage_control_ACCEPT ||
             snapshot.state == fixed_voltage_control_POWER || check == PD_VOLTAGE_WAITING) ?
            0xC27A08 : check == PD_VOLTAGE_VALID ? CONTROL_ACCENT : 0x697684), 0);
    focus();
}

void ui_power_control_key(void *event)
{
    key_event_t *key = event;
    bool adjustment_repeat = pps_mode && selected < 4 && key->id == KEY_ID_Y &&
        (key->type == KEY_EVT_LONG || key->type == KEY_EVT_REPEAT);
    if (key->type != KEY_EVT_CLICK && !adjustment_repeat) return;
    if (key->id == KEY_ID_B) {
        lv_lib_pm_goto("Power Page", 0);
        return;
    }
    if (key->id == KEY_ID_L || key->id == KEY_ID_R) {
        selected = (selected + (key->id == KEY_ID_R ? 1 : count - 1)) % count;
        focus();
    } else if (key->id == KEY_ID_Y) {
        if (selected == count - 1) {
            lv_lib_pm_goto("Power Page", 0);
            return;
        }
        if (!pps_mode && selected < fixed_count) {
            if (user_pd_power_request(fixed_mv[selected], current_ma, false)) {
                voltage_mv = fixed_mv[selected];
                voltage_check = (pd_voltage_check_t){0};
            } else {
                lv_label_set_text(state_label, text("Busy / unavailable", "忙或设置不支持"));
                return;
            }
        } else if (pps_mode && selected < 4) {
            int mv = voltage_mv, ma = current_ma;
            if (selected < 2) mv += selected == 0 ? -100 : 100;
            else ma += selected == 2 ? -50 : 50;
            if (pps_allowed(mv, ma)) {
                voltage_mv = mv;
                current_ma = ma;
                pps_pending = true;
                pps_submit_failed = false;
            }
            if (pps_pending) pps_edited_at = lv_tick_get();
        } else {
            pps_pending = false;
            pps_submit_failed = false;
            bool restore = selected == count - 2;
            if (user_pd_power_request(restore ? 5000 : voltage_mv,
                    current_ma, pps_mode && !restore)) {
                voltage_check = (pd_voltage_check_t){0};
            } else {
                lv_label_set_text(state_label, text("Busy / unavailable", "忙或设置不支持"));
                return;
            }
        }
        refresh(NULL);
    }
}

lv_obj_t *ui_power_control_init(bool pps)
{
    pps_mode = pps;
    pps_pending = pps_submit_failed = false;
    count = fixed_count = selected = 0;
    capability_count = 0;
    last_check = PD_VOLTAGE_IDLE;
    memset(capabilities, 0, sizeof(capabilities));
    voltage_check = (pd_voltage_check_t){0};
    checked_mv = 0;
    user_pd_power_status(&snapshot);
    if (snapshot.count > 7) snapshot.count = 0;
    voltage_mv = snapshot.contract_mv >= 5000 ? snapshot.contract_mv : 5000;
    current_ma = snapshot.contract_pps && snapshot.request_current_ma ?
        snapshot.request_current_ma : 1000;
    if (pps && !pps_allowed(voltage_mv, current_ma)) {
        voltage_mv = 5000;
        current_ma = 1000;
        pps_default();
    }
    screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, 240, 240);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFAFCFF), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0x253342), 0);
    lv_obj_t *title = label(screen, pps ? text("PPS control", "PPS 调节") :
          text("Fixed voltage", "固定电压"), 12, 10, 180,
          &ui_font_menu_bold17);
    lv_obj_set_style_text_color(title, lv_color_hex(0x202D46), 0);
#if POWER_PICO_UI_MODERN
    lv_obj_t *protocol = label(screen, pps ? "PPS" : "PD", 194, 15, 34,
                               &lv_font_montserrat_14);
    lv_obj_set_style_text_align(protocol, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(protocol, lv_color_hex(CONTROL_ACCENT), 0);
#endif
#if POWER_PICO_UI_MODERN
    const lv_font_t *small_font = &ui_font_modern14;
#else
    const lv_font_t *small_font = &ui_font_fixed_voltage_control16;
#endif
    for (unsigned n = 0; n < 2; ++n) {
        lv_obj_t *band = lv_obj_create(screen);
        lv_obj_remove_style_all(band);
        lv_obj_remove_flag(band, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(band, 12 + n * 112, 35);
        lv_obj_set_size(band, 104, 47);
        lv_obj_set_style_radius(band, 8, 0);
        lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(band, lv_color_hex(0xFAFCFF), 0);
        lv_obj_set_style_bg_opa(band, LV_OPA_TRANSP, 0);
        lv_obj_t *caption = label(screen, n ? text("Current", "实测电流") :
            text("Voltage", "实测电压"), 18 + n * 112, 39, 92, small_font);
        lv_obj_set_style_text_color(caption, lv_color_hex(0x71809A), 0);
        meters[n] = label(screen, "--", 18 + n * 112, 58, 106, &lv_font_montserrat_22);
        lv_obj_set_style_text_color(meters[n], lv_color_hex(0x202D46), 0);
    }
    range_label = label(screen, "", 12, 83, 216, &lv_font_montserrat_14);
    lv_obj_set_style_text_color(range_label, lv_color_hex(CONTROL_ACCENT), 0);
    state_label = label(screen, "", 12, 174, 216, &ui_font_menu_bold17);
    if (pps) {
        for (unsigned n = 0; n < 2; ++n) {
            buttons[count++] = button("-", 12, 104 + n * 37, 40);
            values[n] = label(screen, n ? "1.00 A" : "5.00 V",
                              58, 104 + n * 37, 124, &lv_font_montserrat_20);
            lv_obj_set_size(values[n], 124, 31);
            lv_obj_set_style_radius(values[n], 8, 0);
            lv_obj_set_style_border_width(values[n], 2, 0);
            lv_obj_set_style_border_color(values[n], lv_color_hex(0xDCE3ED), 0);
            lv_obj_set_style_bg_color(values[n], lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_opa(values[n], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(values[n], lv_color_hex(0x202D46), 0);
            lv_obj_set_style_text_align(values[n], LV_TEXT_ALIGN_CENTER, 0);
            buttons[count++] = button("+", 188, 104 + n * 37, 40);
        }
        action_buttons();
    } else rebuild_fixed();
    refresh(NULL);
    focus();
    refresh_timer = lv_timer_create(refresh, 200, NULL);
    return screen;
}

void ui_power_control_destroy(void)
{
    pps_pending = pps_submit_failed = false;
    if (refresh_timer) lv_timer_delete(refresh_timer);
    refresh_timer = NULL;
}

void ui_FixedVoltagePage_screen_init(void) { ui_FixedVoltagePage = ui_power_control_init(false); }
void ui_FixedVoltagePage_screen_destroy(void) { ui_power_control_destroy(); }
void ui_fixed_voltage_control_key_handler(void *event) { ui_power_control_key(event); }
