/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>

// lcd
#include "lcd.h"
#include "lcd_init.h"

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    240
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_VER_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    240
#endif

#define BYTE_PER_PIXEL 2 /*will be 2 for RGB565 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/* Track one outstanding async flush to avoid stray callback releasing LVGL twice. */
static volatile lv_display_t *g_pending_flush_disp = NULL;
static volatile bool g_flush_in_progress = false;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_flush_cb(disp, disp_flush);
#define BUFFER_METHOD 1
#if BUFFER_METHOD == 1
    /* Example 1
     * One buffer for partial rendering*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_1_1[MY_DISP_HOR_RES * MY_DISP_VER_RES / 2 * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
#elif BUFFER_METHOD == 2

    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_1[MY_DISP_HOR_RES * MY_DISP_VER_RES / 8 * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_2[MY_DISP_HOR_RES * MY_DISP_VER_RES / 8 * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

#elif BUFFER_METHOD == 3
    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);
#endif

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

void lcd_flush_ready_callback(void)
{
    if (g_flush_in_progress && g_pending_flush_disp != NULL) {
        lv_display_t *done_disp = (lv_display_t *)g_pending_flush_disp;
        g_pending_flush_disp = NULL;
        g_flush_in_progress = false;
        lv_display_flush_ready(done_disp);
    }
}

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    LCD_Init();
    LCD_Fill(0,0, LCD_W, LCD_H, BLACK);
    LCD_Close_Light();
    LCD_Set_Flush_Complete_Callback(lcd_flush_ready_callback);
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    static lv_display_rotation_t last_rotation = LV_DISPLAY_ROTATION_0;
    lv_display_rotation_t current_rotation = lv_display_get_rotation(disp_drv);

    if (!disp_flush_enabled) {
        lv_display_flush_ready(disp_drv);
        return;
    }

    if (area->x2 < area->x1 || area->y2 < area->y1) {
        lv_display_flush_ready(disp_drv);
        return;
    }

    if (current_rotation != last_rotation) {
        LCD_SetRotation(current_rotation * 90); // 仅在旋转状态改变时调用
        last_rotation = current_rotation;      // 更新上次状态
    }

    g_pending_flush_disp = disp_drv;
    g_flush_in_progress = true;
    LCD_Color_Render(area->x1,area->y1,area->x2,area->y2, (uint16_t *)px_map);
    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
