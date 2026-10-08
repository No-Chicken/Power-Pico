#ifndef UI_MODERN_H
#define UI_MODERN_H

#include "../ui.h"
#include "ui_palette.h"

#define M_BG     0xF7F9FC
#define M_PANEL  0xFFFFFF
#define M_LIST_BG 0xEEF1F6
#define M_CARD_LINE 0xE1E6EE
#define M_CARD_RADIUS 10
#define M_LINE   0xDCE1E7
#define M_TEXT   0x233044
#define M_MUTED  0x78869A
#define M_CYAN   ui_palette_accent()
#define M_GREEN  0x00CA91
#define M_AMBER  0xFF593F
#define M_RED    0xDC5843
#define M_WHITE  0xFFFFFF

static inline const char *m_lang(const char *english, const char *chinese)
{
    return ui_get_language_select() ? chinese : english;
}

/* Layout helpers remove default-theme padding, shadows and transitions. */
static inline void m_box(lv_obj_t *o, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(M_TEXT), 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

static inline lv_obj_t *m_screen(void)
{
    lv_obj_t *o = lv_obj_create(NULL);
    m_box(o, 0, 0, 240, 240, M_BG);
    return o;
}

static inline lv_obj_t *m_label(lv_obj_t *parent, const char *text,
                               int x, int y, int w, const lv_font_t *font,
                               uint32_t color)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_label_set_text(o, text);
    return o;
}

static inline lv_obj_t *m_header(lv_obj_t *screen, const char *text, const char *right)
{
    lv_obj_t *title = m_label(screen, text, 12, 12, 178, ui_round_font18(), M_TEXT);
    lv_obj_t *r = m_label(screen, right, 190, 14, 38, &lv_font_montserrat_14, M_CYAN);
    lv_obj_set_style_text_align(r, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *line = lv_obj_create(screen);
    m_box(line, 12, 35, 216, 1, M_LINE);
    return title;
}

static inline lv_obj_t *m_button(lv_obj_t *parent, lv_obj_t **label,
                                const char *text, int x, int y, int w, int h)
{
    lv_obj_t *o = lv_button_create(parent);
    m_box(o, x, y, w, h, M_PANEL);
    lv_obj_set_style_radius(o, 8, 0);
    /* The fixed outer dimensions remain stable when focus changes. */
    lv_obj_set_style_border_color(o, lv_color_hex(M_CYAN), 0);
    lv_obj_set_style_border_post(o, true, 0);
    *label = m_label(o, text, 0, 0, w - 8, &lv_font_montserrat_18, M_TEXT);
    lv_obj_set_style_text_align(*label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(*label);
    return o;
}

static inline void m_meter(lv_obj_t *screen, lv_obj_t **panel,
                           lv_obj_t **value, lv_obj_t **unit,
                           int x, const char *u, uint32_t color)
{
    *panel = lv_obj_create(screen);
    m_box(*panel, x, 42, 104, 53, color);
    lv_obj_set_style_radius(*panel, 8, 0);
    m_label(*panel, u[0] == 'V' ? m_lang("Voltage", "实测电压") : m_lang("Current", "实测电流"),
            8, 4, 92, &ui_font_modern14, M_WHITE);
    *value = m_label(*panel, "--", 6, 24, 74, &lv_font_montserrat_22, M_WHITE);
    lv_label_set_long_mode(*value, LV_LABEL_LONG_CLIP);
    *unit = m_label(*panel, u, 84, 31, 16, &lv_font_montserrat_14, M_WHITE);
}

static inline void m_fit_number(lv_obj_t *label, int width)
{
    lv_point_t size;
    const lv_font_t *large = width > 100 ? &ui_font_HeiTi48 : &ui_font_HeiTi32;
    const lv_font_t *small = width > 100 ? &ui_font_HeiTi32 : &lv_font_montserrat_22;
    lv_text_get_size(&size, lv_label_get_text(label), large,
                    0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const lv_font_t *font = size.x <= width ? large : small;
    if (lv_obj_get_style_text_font(label, 0) != font)
        lv_obj_set_style_text_font(label, font, 0);
    if (size.x > width) {
        lv_text_get_size(&size, lv_label_get_text(label), small,
                        0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x > width) {
            lv_text_get_size(&size, lv_label_get_text(label), &lv_font_montserrat_16,
                            0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
            if (size.x > width) lv_label_set_text(label, "--");
        }
    }
}

static inline void m_fit_meter(lv_obj_t *label)
{
    lv_point_t size;
    lv_text_get_size(&size, lv_label_get_text(label), &lv_font_montserrat_22,
                    0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const lv_font_t *font = size.x <= 74 ? &lv_font_montserrat_22 : &lv_font_montserrat_16;
    if (lv_obj_get_style_text_font(label, 0) != font)
        lv_obj_set_style_text_font(label, font, 0);
    if (size.x > 74) {
        lv_text_get_size(&size, lv_label_get_text(label), &lv_font_montserrat_16,
                        0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x > 74) lv_label_set_text(label, "--");
    }
}

/* Center visible glyph bounds, rather than the font line box. */
static inline void m_center_digits(lv_obj_t *label, int center_y)
{
    const lv_font_t *font = lv_obj_get_style_text_font(label, 0);
    const unsigned char *p = (const unsigned char *)lv_label_get_text(label);
    int top = font->line_height, bottom = 0;
    for (; *p; ++p) {
        lv_font_glyph_dsc_t glyph;
        if (lv_font_get_glyph_dsc(font, &glyph, *p, p[1]) && glyph.box_h) {
            int y = font->line_height - font->base_line - glyph.ofs_y - glyph.box_h;
            if (y < top) top = y;
            if (y + glyph.box_h > bottom) bottom = y + glyph.box_h;
        }
    }
    if (bottom > top) lv_obj_set_y(label, center_y - (top + bottom) / 2);
}

static inline void m_fit_home_number(lv_obj_t *label)
{
    lv_point_t size;
    const lv_font_t *font = &ui_font_instrument35;
    lv_text_get_size(&size, lv_label_get_text(label), font, 0, 0,
                    LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (size.x > 119) font = &ui_font_instrument17;
    lv_text_get_size(&size, lv_label_get_text(label), font, 0, 0,
                    LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (size.x > 119) lv_label_set_text(label, "--");
    lv_obj_set_style_text_font(label, font, 0);
    m_center_digits(label, 25);
}

static inline void m_focus(lv_obj_t *button, bool selected)
{
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(selected ? M_CYAN : M_PANEL), 0);
    for (unsigned i = 0; i < lv_obj_get_child_count(button); ++i)
        lv_obj_set_style_text_color(lv_obj_get_child(button, i),
                                   lv_color_hex(selected ? M_WHITE : M_TEXT), 0);
}

#endif
