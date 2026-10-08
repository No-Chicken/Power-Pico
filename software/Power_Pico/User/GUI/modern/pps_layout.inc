ui_PPSPage = m_screen();
m_header(ui_PPSPage, m_lang("Power control", "电源调节"), "PPS");
m_meter(ui_PPSPage, &ui_PanelPPSVol, &ui_LabelPPSVol, &ui_LabelPPSVolUnit, 12, "V", M_AMBER);
m_meter(ui_PPSPage, &ui_PanelPPSCur, &ui_LabelPPSCur, &ui_LabelPPSCurUnit, 124, "A", M_GREEN);
ui_Spinner1 = NULL;
ui_PanelPPS = lv_obj_create(ui_PPSPage);
m_box(ui_PanelPPS, 120, 33, 480, 104, M_BG);
lv_obj_set_align(ui_PanelPPS, LV_ALIGN_CENTER);
lv_obj_set_y(ui_PanelPPS, 29);
lv_obj_t **buttons[] = {&ui_BtnSet1, &ui_BtnSet2, &ui_BtnSet3,
                       &ui_BtnSet4, &ui_BtnSet5, &ui_BtnSet6};
lv_obj_t **labels[] = {&ui_LabelSet1, &ui_LabelSet2, &ui_LabelSet3,
                      &ui_LabelSet4, &ui_LabelSet5, &ui_LabelSet6};
const char *presets[] = {"5V  1A", "9V  1A", "12V  1A", "15V  1A", "18V  1A", "20V  1A"};
for (unsigned i = 0; i < 6; ++i)
    *buttons[i] = m_button(ui_PanelPPS, labels[i], presets[i],
                          12 + (i % 2) * 112, (i / 2) * 33, 104, 28);
lv_obj_set_style_border_width(ui_BtnSet1, 3, 0);
ui_BtnValSet = lv_obj_create(ui_PanelPPS);
m_box(ui_BtnValSet, 252, 0, 216, 46, M_PANEL);
lv_obj_set_style_radius(ui_BtnValSet, 8, 0);
m_label(ui_BtnValSet, m_lang("Voltage", "设定电压"), 50, 2, 116, &ui_font_modern14, M_MUTED);
ui_LabelValSet = m_label(ui_BtnValSet, "5.00 V", 50, 20, 116, &lv_font_montserrat_20, M_TEXT);
lv_obj_set_style_text_align(ui_LabelValSet, LV_TEXT_ALIGN_CENTER, 0);
ui_BtnValDec = m_button(ui_BtnValSet, &ui_LabelValDec, "-", 8, 8, 30, 30);
ui_BtnValInc = m_button(ui_BtnValSet, &ui_LabelValInc, "+", 178, 8, 30, 30);
ui_BtnCurSet = lv_obj_create(ui_PanelPPS);
m_box(ui_BtnCurSet, 252, 54, 216, 46, M_PANEL);
lv_obj_set_style_radius(ui_BtnCurSet, 8, 0);
m_label(ui_BtnCurSet, m_lang("Current limit", "电流上限"), 50, 2, 116, &ui_font_modern14, M_MUTED);
ui_LabelCurSet = m_label(ui_BtnCurSet, "1.00 A", 50, 20, 116, &lv_font_montserrat_20, M_TEXT);
lv_obj_set_style_text_align(ui_LabelCurSet, LV_TEXT_ALIGN_CENTER, 0);
ui_BtnCurDec = m_button(ui_BtnCurSet, &ui_LabelCurDec, "-", 8, 8, 30, 30);
ui_BtnCurInc = m_button(ui_BtnCurSet, &ui_LabelCurInc, "+", 178, 8, 30, 30);
lv_obj_t *steppers[] = {ui_BtnValDec, ui_BtnValInc, ui_BtnCurDec, ui_BtnCurInc};
for (unsigned i = 0; i < 4; ++i) lv_obj_set_style_radius(steppers[i], LV_RADIUS_CIRCLE, 0);
ui_BtnSwitch = m_button(ui_PPSPage, &ui_LabelSwitch, LV_SYMBOL_SETTINGS, 12, 202, 104, 26);
ui_BtnClose = m_button(ui_PPSPage, &ui_LabelClose, LV_SYMBOL_POWER " PPS", 124, 202, 104, 26);
lv_obj_set_style_text_color(ui_LabelClose, lv_color_hex(M_RED), 0);
