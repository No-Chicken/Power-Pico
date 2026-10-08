ui_PDFixedPage = m_screen();
m_header(ui_PDFixedPage, m_lang("Power control", "电源调节"), "PD");
m_meter(ui_PDFixedPage, &ui_PanelPDVol, &ui_LabelPDVol, &ui_LabelPDVolUnit, 12, "V", M_AMBER);
m_meter(ui_PDFixedPage, &ui_PanelPDCur, &ui_LabelPDCur, &ui_LabelPDCurUnit, 124, "A", M_GREEN);
ui_Spinner2 = NULL;
lv_obj_t **buttons[] = {&ui_BtnPDSet1, &ui_BtnPDSet2, &ui_BtnPDSet3, &ui_BtnPDSet4, &ui_BtnPDSet5};
lv_obj_t **labels[] = {&ui_LabelPDSet1, &ui_LabelPDSet2, &ui_LabelPDSet3, &ui_LabelPDSet4, &ui_LabelPDSet5};
const char *presets[] = {"5V  1A", "9V  1A", "12V  1A", "15V  1A", "20V  1A"};
for (unsigned i = 0; i < 5; ++i)
    *buttons[i] = m_button(ui_PDFixedPage, labels[i], presets[i],
                          12 + (i % 2) * 112, 101 + (i / 2) * 33, 104, 28);
lv_obj_set_style_border_width(ui_BtnPDSet1, 3, 0);
ui_BtnPDClose = m_button(ui_PDFixedPage, &ui_LabelPDClose,
                        LV_SYMBOL_POWER " PD", 12, 202, 216, 26);
lv_obj_set_style_text_color(ui_LabelPDClose, lv_color_hex(M_RED), 0);
