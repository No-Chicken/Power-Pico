#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "lvgl.h"

// 最大支持页面数量
#define MAX_PAGES 5

// 页面结构体
typedef struct {
    void (*init)(void);
    void (*deinit)(void);
    lv_obj_t** page_obj;  // 指向页面对象指针的指针
    void (*key_event_handler)(uint8_t key_id); // 按键事件处理函数
    const char* name;     // 可选：用于调试
} Page_t;

// 页面管理器结构体
typedef struct {
    Page_t* pages[MAX_PAGES];
    uint8_t count;
    uint8_t current_index;
} PageManager_t;

// 全局管理器实例（声明）
extern PageManager_t PageManager;

// API 函数声明
void PageManager_init(void);
void PageManager_register(Page_t* page);
void PageManager_load_init_screen(void);
void PageManager_next(void);
void PageManager_prev(void);
void PageManager_goto(uint8_t index);
Page_t* PageManager_get_current_page(void);
void PageManager_handle_key_event(uint8_t key_id);

#endif // PAGE_MANAGER_H
