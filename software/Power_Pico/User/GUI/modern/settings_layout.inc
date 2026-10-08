ui_SetPage = m_screen();
lv_obj_set_style_bg_color(ui_SetPage, lv_color_hex(M_LIST_BG), 0);
lv_obj_add_flag(ui_SetPage, LV_OBJ_FLAG_SCROLLABLE);
lv_obj_set_scroll_dir(ui_SetPage, LV_DIR_VER);
lv_obj_set_scrollbar_mode(ui_SetPage, LV_SCROLLBAR_MODE_ON);
lv_obj_set_style_width(ui_SetPage, 2, LV_PART_SCROLLBAR);
lv_obj_set_style_pad_top(ui_SetPage, 43, LV_PART_SCROLLBAR);
lv_obj_set_style_pad_bottom(ui_SetPage, 12, LV_PART_SCROLLBAR);
lv_obj_set_style_pad_right(ui_SetPage, 4, LV_PART_SCROLLBAR);
lv_obj_set_style_bg_color(ui_SetPage, lv_color_hex(M_CYAN), LV_PART_SCROLLBAR);
lv_obj_set_style_bg_opa(ui_SetPage, LV_OPA_50, LV_PART_SCROLLBAR);
lv_obj_set_style_pad_bottom(ui_SetPage, 12, 0);
ui_ModernSettingsTitle = m_label(ui_SetPage, m_lang("Settings", "设置"),
                                 13, 11, 150, &ui_font_title17, M_TEXT);
lv_obj_t **row[] = {&ui_PanelBL, &ui_PanelKS, &ui_PanelLang,
                   &ui_PanelRotate, &ui_PanelRange, &ui_PanelPPS, &ui_PanelTheme, &ui_PanelAbout};
const int heights[] = {80, 44, 44, 44, 44, 44, 44, 86};
int y = 44;
for (unsigned i = 0; i < 8; ++i) {
    *row[i] = lv_obj_create(ui_SetPage);
    m_box(*row[i], 12, y, 216, heights[i], M_PANEL);
    lv_obj_add_flag(*row[i], LV_OBJ_FLAG_CHECKABLE);
    /* A constant border and inset prevent content shifting with focus. */
    lv_obj_set_style_pad_all(*row[i], 11, 0);
    lv_obj_set_style_radius(*row[i], M_CARD_RADIUS, 0);
    lv_obj_set_style_border_width(*row[i], 1, 0);
    lv_obj_set_style_border_color(*row[i], lv_color_hex(M_CARD_LINE), 0);
    lv_obj_set_style_bg_color(*row[i], lv_color_hex(M_CYAN), LV_STATE_CHECKED);
    lv_obj_set_style_border_color(*row[i], lv_color_hex(M_CYAN), LV_STATE_CHECKED);
    y += heights[i] + 8;
}
const char *icons[] = {LV_SYMBOL_IMAGE, LV_SYMBOL_VOLUME_MID, LV_SYMBOL_KEYBOARD,
                      LV_SYMBOL_REFRESH, LV_SYMBOL_BARS, LV_SYMBOL_CHARGE, LV_SYMBOL_TINT};
for (unsigned i = 0; i < 7; ++i) {
    ui_ModernIcons[i] = m_label(*row[i], icons[i], 0, 0, 18,
                                &lv_font_montserrat_16, M_CYAN);
    lv_obj_set_style_text_align(ui_ModernIcons[i], LV_TEXT_ALIGN_CENTER, 0);
    if (i == 0) lv_obj_set_y(ui_ModernIcons[i], 3);
    else lv_obj_align(ui_ModernIcons[i], LV_ALIGN_LEFT_MID, 0, 0);
}
ui_LabelBL = m_label(ui_PanelBL, "", 26, 0, 114, &ui_font_modern16, M_TEXT);
ui_ModernBLValue = m_label(ui_PanelBL, "", 144, 1, 48, &lv_font_montserrat_16, M_CYAN);
lv_obj_set_style_text_align(ui_ModernBLValue, LV_TEXT_ALIGN_RIGHT, 0);
ui_SliderBL = lv_slider_create(ui_PanelBL);
lv_obj_remove_style_all(ui_SliderBL);
lv_obj_set_size(ui_SliderBL, 192, 20);
lv_obj_align(ui_SliderBL, LV_ALIGN_BOTTOM_MID, 0, 0);
lv_obj_set_style_radius(ui_SliderBL, LV_RADIUS_CIRCLE, LV_PART_MAIN);
lv_obj_set_style_radius(ui_SliderBL, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
lv_obj_set_style_radius(ui_SliderBL, LV_RADIUS_CIRCLE, LV_PART_KNOB);
lv_obj_set_style_bg_color(ui_SliderBL, lv_color_hex(M_LINE), LV_PART_MAIN);
lv_obj_set_style_bg_opa(ui_SliderBL, LV_OPA_COVER, LV_PART_MAIN);
lv_obj_set_style_pad_ver(ui_SliderBL, 3, LV_PART_MAIN);
lv_obj_set_style_pad_hor(ui_SliderBL, 10, LV_PART_MAIN);
lv_obj_set_style_bg_color(ui_SliderBL, lv_color_hex(M_CYAN), LV_PART_INDICATOR);
lv_obj_set_style_bg_opa(ui_SliderBL, LV_OPA_COVER, LV_PART_INDICATOR);
lv_obj_set_style_bg_color(ui_SliderBL, lv_color_hex(M_WHITE), LV_PART_KNOB);
lv_obj_set_style_border_width(ui_SliderBL, 2, LV_PART_KNOB);
lv_obj_set_style_border_color(ui_SliderBL, lv_color_hex(M_CYAN), LV_PART_KNOB);
lv_obj_set_style_bg_opa(ui_SliderBL, LV_OPA_COVER, LV_PART_KNOB);
lv_obj_set_style_pad_all(ui_SliderBL, -1, LV_PART_KNOB);
lv_obj_set_style_anim_duration(ui_SliderBL, 0, 0);
/* Keep native value/input handling; render three concentric capsule shapes. */
lv_obj_set_style_bg_opa(ui_SliderBL, LV_OPA_TRANSP, LV_PART_MAIN);
lv_obj_set_style_bg_opa(ui_SliderBL, LV_OPA_TRANSP, LV_PART_INDICATOR);
lv_obj_set_style_bg_opa(ui_SliderBL, LV_OPA_TRANSP, LV_PART_KNOB);
lv_obj_set_style_border_opa(ui_SliderBL, LV_OPA_TRANSP, LV_PART_KNOB);
lv_obj_add_event_cb(ui_SliderBL, brightness_draw, LV_EVENT_DRAW_MAIN, NULL);
lv_obj_t **switches[] = {&ui_SwitchKS, &ui_SwitchLang};
lv_obj_t *parents[] = {ui_PanelKS, ui_PanelLang};
lv_obj_t **labels[] = {&ui_LabelKS, &ui_LabelLang};
for (unsigned i = 0; i < 2; ++i) {
    *labels[i] = m_label(parents[i], "", 26, 0, 124, &ui_font_modern16, M_TEXT);
    lv_obj_align(*labels[i], LV_ALIGN_LEFT_MID, 26, 0);
    *switches[i] = lv_switch_create(parents[i]);
    lv_obj_remove_style_all(*switches[i]);
    lv_obj_set_size(*switches[i], 32, 18);
    lv_obj_align(*switches[i], LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(*switches[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(*switches[i], lv_color_hex(M_LINE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(*switches[i], LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(*switches[i], LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(*switches[i], lv_color_hex(M_CYAN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(*switches[i], LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_radius(*switches[i], LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_bg_color(*switches[i], lv_color_hex(M_WHITE), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(*switches[i], LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(*switches[i], -3, LV_PART_KNOB);
    lv_obj_set_style_anim_duration(*switches[i], 0, 0);
}
ui_LabelPPS = m_label(ui_PanelPPS, "", 26, 0, 148, &ui_font_modern16, M_TEXT);
lv_obj_align(ui_LabelPPS, LV_ALIGN_LEFT_MID, 26, 0);
lv_obj_t *power_arrow = m_label(ui_PanelPPS, LV_SYMBOL_RIGHT, 0, 0, 16,
                               &lv_font_montserrat_16, M_CYAN);
lv_obj_align(power_arrow, LV_ALIGN_RIGHT_MID, 0, 0);
ui_LabelTheme = m_label(ui_PanelTheme, m_lang("Theme", "主题"), 26, 0, 65,
                        ui_round_font18(), M_TEXT);
lv_obj_align(ui_LabelTheme, LV_ALIGN_LEFT_MID, 26, 0);
ui_ThemeValue = m_label(ui_PanelTheme, ui_palette_name(ui_palette_index()), 0, 0, 85,
                        ui_round_font18(), M_CYAN);
lv_obj_set_style_text_align(ui_ThemeValue, LV_TEXT_ALIGN_RIGHT, 0);
lv_obj_align(ui_ThemeValue, LV_ALIGN_RIGHT_MID, -14, 0);
lv_obj_t *theme_arrow = m_label(ui_PanelTheme, LV_SYMBOL_RIGHT, 0, 0, 10,
                               &lv_font_montserrat_14, M_CYAN);
lv_obj_align(theme_arrow, LV_ALIGN_RIGHT_MID, 0, 0);
lv_obj_t *header_bg = lv_obj_create(ui_SetPage);
m_box(header_bg, 0, 0, 240, 40, M_LIST_BG);
lv_obj_add_flag(header_bg, LV_OBJ_FLAG_FLOATING);
lv_obj_add_flag(ui_ModernSettingsTitle, LV_OBJ_FLAG_FLOATING);
lv_obj_move_foreground(ui_ModernSettingsTitle);
lv_obj_t *footer_margin = lv_obj_create(ui_SetPage);
m_box(footer_margin, 0, 228, 240, 12, M_LIST_BG);
lv_obj_add_flag(footer_margin, LV_OBJ_FLAG_FLOATING);
ui_LabelRotation = m_label(ui_PanelRotate, "", 26, 0, 91, &ui_font_modern16, M_TEXT);
lv_obj_align(ui_LabelRotation, LV_ALIGN_LEFT_MID, 26, 0);
ui_LabelRotNum = m_label(ui_PanelRotate, "", 115, 0, 77, &lv_font_montserrat_16, M_CYAN);
lv_obj_align(ui_LabelRotNum, LV_ALIGN_RIGHT_MID, 0, 0);
lv_obj_set_style_text_align(ui_LabelRotNum, LV_TEXT_ALIGN_RIGHT, 0);
ui_LabelRange = m_label(ui_PanelRange, "", 26, 0, 91, &ui_font_modern16, M_TEXT);
lv_obj_align(ui_LabelRange, LV_ALIGN_LEFT_MID, 26, 0);
ui_LabelRangeMode = m_label(ui_PanelRange, "", 118, 0, 74, &ui_font_modern16, M_CYAN);
lv_obj_align(ui_LabelRangeMode, LV_ALIGN_RIGHT_MID, 0, 0);
lv_obj_set_style_text_align(ui_LabelRangeMode, LV_TEXT_ALIGN_RIGHT, 0);
char str_buf[80];
snprintf(str_buf, sizeof(str_buf), "POWER PICO\nThe uA current meter\nV %d.%d.%d",
         pp_version_major(), pp_version_minor(), pp_version_patch());
ui_LabelAbout = m_label(ui_PanelAbout, str_buf, 0, 0, 192, &ui_font_modern14, M_MUTED);
refresh_language();
