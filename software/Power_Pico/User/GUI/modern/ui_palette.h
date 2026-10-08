#ifndef UI_PALETTE_H
#define UI_PALETTE_H
#include "../ui.h"
#if POWER_PICO_UI_MODERN
enum { UI_PALETTE_COUNT = 8 };
uint8_t ui_palette_index(void);
void ui_palette_select(uint8_t index);
uint32_t ui_palette_accent(void);
uint32_t ui_palette_edge(void);
const char *ui_palette_name(uint8_t index);
extern lv_obj_t *ui_ThemePage;
void ui_ThemePage_screen_init(void);
void ui_theme_page_key_handler(void *event);
#endif
#endif
