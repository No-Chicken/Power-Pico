#include "ui_palette.h"
#if POWER_PICO_UI_MODERN
#include "ui_modern.h"
#include "key.h"

static const struct {
    uint32_t accent;
    const char *en, *zh;
} palettes[UI_PALETTE_COUNT] = {
    {0x507FEB, "Blue", "亮蓝"},
    {0x248067, "Jade", "翡翠"},
    {0xA6171B, "Vermilion", "朱红"},
    {0x096DAE, "Lake blue", "湖蓝"},
    {0x81021F, "Burgundy", "酒红"},
    {0x013E77, "Navy", "深蓝"},
    {0xE6742D, "Orange", "活力橙"},
    {0x009A53, "Emerald", "翠绿"},
};
/* Session-only UI preference: do not change EEPROM/startup contracts. */
static uint8_t palette_index, selected;
lv_obj_t *ui_ThemePage;
static lv_obj_t *theme_rows[UI_PALETTE_COUNT], *theme_labels[UI_PALETTE_COUNT];

uint8_t ui_palette_index(void) { return palette_index; }
void ui_palette_select(uint8_t index)
{
    if (index < UI_PALETTE_COUNT) palette_index = index;
}
uint32_t ui_palette_accent(void) { return palettes[palette_index].accent; }
uint32_t ui_palette_edge(void)
{
    uint32_t c = ui_palette_accent();
    return ((((c >> 16) & 255) * 9 / 10) << 16) |
           ((((c >> 8) & 255) * 9 / 10) << 8) | ((c & 255) * 9 / 10);
}
const char *ui_palette_name(uint8_t index)
{
    if (index >= UI_PALETTE_COUNT) index = 0;
    return ui_get_language_select() ? palettes[index].zh : palettes[index].en;
}

static void theme_focus(void)
{
    for (unsigned i = 0; i < UI_PALETTE_COUNT; ++i) {
        lv_obj_set_style_bg_color(theme_rows[i],
            lv_color_hex(i == selected ? M_CYAN : M_PANEL), 0);
        lv_obj_set_style_border_color(theme_rows[i],
            lv_color_hex(i == selected ? M_CYAN : M_CARD_LINE), 0);
        lv_obj_set_style_text_color(theme_labels[i],
            lv_color_hex(i == selected ? M_WHITE : M_TEXT), 0);
        lv_obj_set_style_text_font(theme_labels[i],
            i == selected ? &ui_font_menu_bold17 : ui_round_font18(), 0);
    }
    lv_obj_update_layout(ui_ThemePage);
    lv_area_t area;
    lv_obj_get_coords(theme_rows[selected], &area);
    int32_t delta = area.y1 < 43 ? area.y1 - 43 :
                    area.y2 > 227 ? area.y2 - 227 : 0;
    if (delta)
        lv_obj_scroll_to_y(ui_ThemePage, lv_obj_get_scroll_y(ui_ThemePage) + delta, LV_ANIM_OFF);
}

void ui_theme_page_key_handler(void *event)
{
    key_event_t *key = event;
    if (key->type != KEY_EVT_CLICK) return;
    if (key->id == KEY_ID_B || key->id == KEY_ID_N) {
        lv_lib_pm_goto("Set Page", 0);
    } else if (key->id == KEY_ID_Y) {
        ui_palette_select(selected);
        lv_lib_pm_goto("Set Page", 0);
    } else if (key->id == KEY_ID_L || key->id == KEY_ID_R) {
        selected = (selected + (key->id == KEY_ID_R ? 1 : UI_PALETTE_COUNT - 1)) % UI_PALETTE_COUNT;
        theme_focus();
    }
}

void ui_ThemePage_screen_init(void)
{
    selected = ui_palette_index();
    ui_ThemePage = m_screen();
    lv_obj_set_style_bg_color(ui_ThemePage, lv_color_hex(M_LIST_BG), 0);
    lv_obj_add_flag(ui_ThemePage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui_ThemePage, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_ThemePage, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_bottom(ui_ThemePage, 12, 0);
    lv_obj_t *header = lv_obj_create(ui_ThemePage);
    m_box(header, 0, 0, 240, 40, M_LIST_BG);
    lv_obj_add_flag(header, LV_OBJ_FLAG_FLOATING);
    lv_obj_t *title = m_label(ui_ThemePage, m_lang("Theme", "主题"),
                              13, 11, 190, &ui_font_title17, M_TEXT);
    lv_obj_add_flag(title, LV_OBJ_FLAG_FLOATING);
    for (unsigned i = 0; i < UI_PALETTE_COUNT; ++i) {
        theme_rows[i] = lv_obj_create(ui_ThemePage);
        m_box(theme_rows[i], 12, 44 + 52 * i, 216, 44, M_PANEL);
        lv_obj_set_style_radius(theme_rows[i], M_CARD_RADIUS, 0);
        lv_obj_set_style_border_width(theme_rows[i], 1, 0);
        theme_labels[i] = m_label(theme_rows[i], ui_palette_name(i), 12, 10, 158,
                                  ui_round_font18(), M_TEXT);
        lv_obj_t *swatch = lv_obj_create(theme_rows[i]);
        m_box(swatch, 188, 13, 16, 16, palettes[i].accent);
        lv_obj_set_style_radius(swatch, 4, 0);
        lv_obj_set_style_border_width(swatch, i == palette_index ? 2 : 1, 0);
        lv_obj_set_style_border_color(swatch, lv_color_hex(M_WHITE), 0);
    }
    lv_obj_move_foreground(header);
    lv_obj_move_foreground(title);
    lv_obj_t *footer = lv_obj_create(ui_ThemePage);
    m_box(footer, 0, 228, 240, 12, M_LIST_BG);
    lv_obj_add_flag(footer, LV_OBJ_FLAG_FLOATING);
    theme_focus();
}
#endif
