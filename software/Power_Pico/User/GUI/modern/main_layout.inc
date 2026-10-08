ui_HomeScreen = m_screen();
m_label(ui_HomeScreen, "Power Pico", 13, 11, 175, &ui_font_title17, M_TEXT);
lv_obj_t *dc = m_label(ui_HomeScreen, "DC", 194, 14, 33,
                       &lv_font_montserrat_14, M_MUTED);
lv_obj_set_style_text_align(dc, LV_TEXT_ALIGN_RIGHT, 0);
lv_obj_t **rows[] = {&ui_ButVal, &ui_ButCur, &ui_ButEnerge};
lv_obj_t **values[] = {&ui_LabelValt, &ui_LabelCur, &ui_LabelEnerge};
lv_obj_t **units[] = {&ui_LabelUnitVal, &ui_LabelUnitCur, &ui_LabelUnitEnerge};
const uint32_t colors[] = {0xF46B70, 0x43BD59, 0x507FEB};
const uint32_t edges[] = {0xE85E65, 0x36AD4D, 0x4470DC};
const char *unit_text[] = {"V", "uA", "uW"};
const char *captions[] = {m_lang("Volt", "电压"), m_lang("Curr", "电流"), m_lang("Power", "功率")};
for (unsigned i = 0; i < 3; ++i) {
    *rows[i] = lv_obj_create(ui_HomeScreen);
    m_box(*rows[i], 12, 42 + 57 * i, 216, 51, edges[i]);
    lv_obj_set_style_radius(*rows[i], M_CARD_RADIUS, 0);
    lv_obj_t *section = lv_obj_create(*rows[i]);
    m_box(section, 52, 0, 164, 51, colors[i]);
    lv_obj_set_style_radius(section, M_CARD_RADIUS, 0);
    /* Square the inner edge without an off-screen rounded clipping layer. */
    lv_obj_t *seam = lv_obj_create(*rows[i]);
    m_box(seam, 52, 0, M_CARD_RADIUS, 51, colors[i]);
    lv_obj_t *caption = m_label(*rows[i], captions[i], 4, 0, 44,
                                ui_get_language_select() ? &ui_font_home_label18 :
                                                           &ui_font_modern14, M_WHITE);
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(caption, LV_ALIGN_LEFT_MID, 4, 0);
    *values[i] = m_label(*rows[i], "0.000", 64, 0, 119,
                         &ui_font_instrument35, M_WHITE);
    lv_obj_set_style_text_align(*values[i], LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_long_mode(*values[i], LV_LABEL_LONG_CLIP);
    *units[i] = m_label(*rows[i], unit_text[i], 185, 0, 30,
                        &ui_font_instrument17, M_WHITE);
    lv_obj_set_style_text_align(*units[i], LV_TEXT_ALIGN_CENTER, 0);
    m_center_digits(*values[i], 25);
    m_center_digits(*units[i], 26);
}
lv_obj_t *divider = lv_obj_create(ui_HomeScreen);
m_box(divider, 13, 217, 214, 1, M_LINE);
ui_ButTime = NULL;
m_label(ui_HomeScreen, m_lang("TIME", "运行时间"), 13, 221, 90, &ui_font_modern14, M_MUTED);
ui_LabelTime = m_label(ui_HomeScreen, "00:00:00", 104, 219, 124,
                       &ui_font_instrument17, M_TEXT);
lv_obj_set_style_text_align(ui_LabelTime, LV_TEXT_ALIGN_RIGHT, 0);
m_center_digits(ui_LabelTime, 230);
