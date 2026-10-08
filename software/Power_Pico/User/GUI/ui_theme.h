#ifndef POWER_PICO_UI_THEME_H
#define POWER_PICO_UI_THEME_H

/* Non-CMake projects keep the Classic UI unless explicitly enabled. */
#ifndef POWER_PICO_UI_MODERN
#define POWER_PICO_UI_MODERN 0
#endif

#if POWER_PICO_UI_MODERN
#define UI_THEME_ANIM LV_ANIM_OFF
#else
#define UI_THEME_ANIM LV_ANIM_ON
#endif

#endif
