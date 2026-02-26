#include "./lv_lib_pm.h"
#include <string.h>

// 定义全局实例
PageManager_t PageManager;

/**
 * 初始化页面管理器
 */
void lv_lib_pm_Init(void) {
    memset(&PageManager, 0, sizeof(PageManager));
}

/**
 * load init screen
 */
void lv_lib_pm_load_init_screen(void) {
    if (PageManager.count == 0) return;

    Page_t* initial_page = PageManager.pages[0];
    if (initial_page->init) initial_page->init();
    lv_screen_load_anim(*initial_page->page_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
    PageManager.current_index = 0;
}

/**
 * 注册一个新页面
 * @param page 页面结构体指针
 */
void lv_lib_pm_register(Page_t* page) {
    if (PageManager.count >= MAX_PAGES) {
        LV_LOG_WARN("PageManager: page list is full!");
        return;
    }
    PageManager.pages[PageManager.count++] = page;
}

/**
 * 切换到数组中下一个页面（循环）
 */
void lv_lib_pm_next(void) {
    if (PageManager.count == 0) return;

    Page_t* prev = PageManager.pages[PageManager.current_index];
    Page_t* next = PageManager.pages[(PageManager.current_index + 1) % PageManager.count];

    // 初始化新页面并切换
    if (next->init) next->init();
    lv_screen_load_anim(*next->page_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, true);

    // 反初始化前页
    if (prev->deinit) prev->deinit();

    // 更新索引
    PageManager.current_index = (PageManager.current_index + 1) % PageManager.count;

}

/**
 * 切换到数组中上一个页面（循环）
 */
void lv_lib_pm_prev(void) {
    if (PageManager.count == 0) return;

    Page_t* current = PageManager.pages[PageManager.current_index];
    uint8_t prev_index = (PageManager.current_index == 0) ? PageManager.count - 1 : PageManager.current_index - 1;
    Page_t* prev = PageManager.pages[prev_index];

    if (prev->init) prev->init();
    lv_screen_load_anim(*prev->page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, true);

    if (current->deinit) current->deinit();

    PageManager.current_index = prev_index;

}

/**
 * 跳转到指定索引页面
 * @param page_name 页面名称，传入 NULL 则根据索引跳转
 * @param index 目标页面索引，如果 page_name 不为 NULL 则忽略该参数
 */
void lv_lib_pm_goto(const char* page_name, uint8_t index) {

    if (page_name != NULL) {
        bool found = false;
        // 根据页面名称查找索引
        for (uint8_t i = 0; i < PageManager.count; i++) {
            if (strcmp(PageManager.pages[i]->name, page_name) == 0) {
                index = i;
                found = true;
                break;
            }
        }
        // 如果根据名称查找但未找到，则直接返回，不执行任何跳转
        if (!found) {
            LV_LOG_WARN("Page '%s' not found.", page_name);
            return;
        }
    }

    if(index >= PageManager.count || PageManager.count == 0) return;

    Page_t* current = PageManager.pages[PageManager.current_index];
    Page_t* target = PageManager.pages[index];

    if (current != target) {
        if (current->deinit) current->deinit();
        PageManager.current_index = index;
        if (target->init) target->init();
        lv_screen_load_anim(*target->page_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
    }
}

/**
 * 跳转到首页面
 */
void lv_lib_pm_goto_first(void) {
    lv_lib_pm_goto(NULL, 0);
}

/**
 * 获取当前页面
 * @return 当前页面指针
 */
Page_t* lv_lib_pm_get_current_page(void) {
    if (PageManager.count == 0) return NULL;
    return PageManager.pages[PageManager.current_index];
}

/**
 * 处理按键事件，调用当前页面的按键事件处理函数
 * @param key_id 按键ID
 */
void lv_lib_pm_handle_key_event(void* key_event) {
    Page_t* current_page = lv_lib_pm_get_current_page();
    if (current_page && current_page->key_event_handler) {
        current_page->key_event_handler(key_event); // 调用当前页面的按键事件处理函数
    } else {
        LV_LOG_WARN("No key event handler for current page\n");
    }
}
