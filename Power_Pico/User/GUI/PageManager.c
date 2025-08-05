#include "PageManager.h"
#include <string.h>

// 定义全局实例
PageManager_t PageManager;

/**
 * 初始化页面管理器
 */
void PageManager_init(void) {
    memset(&PageManager, 0, sizeof(PageManager));
}

/**
 * 注册一个新页面
 * @param page 页面结构体指针
 */
void PageManager_register(Page_t* page) {
    if (PageManager.count >= MAX_PAGES) {
        LV_LOG_WARN("PageManager: page list is full!");
        return;
    }
    PageManager.pages[PageManager.count++] = page;
}

/**
 * 切换到下一个页面（循环）
 */
void PageManager_next(void) {
    if (PageManager.count == 0) return;

    Page_t* current = PageManager.pages[PageManager.current_index];
    Page_t* next = PageManager.pages[(PageManager.current_index + 1) % PageManager.count];

    // 反初始化当前页
    if (current->deinit) current->deinit();

    // 更新索引
    PageManager.current_index = (PageManager.current_index + 1) % PageManager.count;

    // 初始化新页面并切换
    if (next->init) next->init();
    lv_scr_load_anim(*next->page_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
}

/**
 * 切换到上一个页面（循环）
 */
void PageManager_prev(void) {
    if (PageManager.count == 0) return;

    Page_t* current = PageManager.pages[PageManager.current_index];
    uint8_t prev_index = (PageManager.current_index == 0) ?
                         PageManager.count - 1 : PageManager.current_index - 1;
    Page_t* prev = PageManager.pages[prev_index];

    if (current->deinit) current->deinit();

    PageManager.current_index = prev_index;

    if (prev->init) prev->init();
    lv_scr_load_anim(*prev->page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, true);
}

/**
 * 跳转到指定索引页面
 * @param index 目标页面索引
 */
void PageManager_goto(uint8_t index) {
    if (index >= PageManager.count || PageManager.count == 0) return;

    Page_t* current = PageManager.pages[PageManager.current_index];
    Page_t* target = PageManager.pages[index];

    if (current != target) {
        if (current->deinit) current->deinit();
        PageManager.current_index = index;
        if (target->init) target->init();
        lv_scr_load_anim(*target->page_obj, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, true);
    }
}

/**
 * 获取当前页面
 * @return 当前页面指针
 */
Page_t* PageManager_get_current_page(void) {
    if (PageManager.count == 0) return NULL;
    return PageManager.pages[PageManager.current_index];
}
