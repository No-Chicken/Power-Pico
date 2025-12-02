/*
 *  key.c
 *
 *  规范化按键驱动：
 *  - 支持多键注册（GPIO，低/高有效）
 *  - 去抖、长按、连发
 *  - 事件队列（DOWN/UP/CLICK/LONG/REPEAT）
 *  - 掩码查询当前稳定按下状态，支持多键同时
 *
 *  Created on: 2025-12-02
 */

#include "key.h"
#include "sys.h"
#include "cmsis_os.h"   // 若无OS，可移除
#include <string.h>

/* 若有OS则用OS tick，否则用HAL tick */
#if OS_SUPPORT
    #define key_get_tick() ((uint32_t)osKernelGetTickCount())
#else
    #define key_get_tick() ((uint32_t)HAL_GetTick())
#endif
#define TICK_DIFF(now, then) ((uint32_t)((uint32_t)(now) - (uint32_t)(then)))

/* 按键槽结构体 */
typedef struct {
    GPIO_TypeDef* port; // gpio port
    uint16_t      pin;  // gpio pin
    uint8_t       active_high;      // 高电平有效
    uint8_t       registered;       // 是否已注册
    uint8_t       raw_pressed;      // 原始按下状态
    uint8_t       stable_pressed;   // 稳定按下状态
    uint32_t      debounce_tick;    // 消抖计时
    uint8_t       long_reported;    // 长按事件是否上报
    uint32_t      press_tick;       // 按下时刻
    uint32_t      last_repeat_tick; // 上次连发时刻
} key_slot_t;

/* 全局按键管理结构体 */
static struct {
    uint8_t      mode;
    key_timing_t timing;
    uint32_t     long_enable_mask;
    key_slot_t   slot[KEY_ID_MAX];
    uint32_t     raw_mask;
    uint32_t     stable_mask;
} g_key = {
    .mode = (KEY_MODE_REPORT_DOWN_UP | KEY_MODE_CLICK_ON_RELEASE | KEY_MODE_LONG_ONCE),
    .timing = { .debounce_ms = 5, .long_ms = 500, .repeat_ms = 150 },
    .long_enable_mask = 0,
    .raw_mask = 0,
    .stable_mask = 0,
};

/**
 * @brief 注册GPIO按键
 * @param id        按键ID
 * @param port      GPIO端口
 * @param pin      GPIO引脚
 * @param active_high 引脚高电平有效
*/
bool Key_RegisterGPIO(key_id_t id, GPIO_TypeDef* port, uint16_t pin, uint8_t active_high)
{
    if(id >= KEY_ID_MAX || port == NULL) return false;
    key_slot_t* k = &g_key.slot[id];
    k->port = port;
    k->pin = pin;
    k->active_high = active_high ? 1 : 0;
    k->registered = 1;
    k->raw_pressed = 0;
    k->stable_pressed = 0;
    k->debounce_tick = key_get_tick();
    k->long_reported = 0;
    k->press_tick = 0;
    k->last_repeat_tick = 0;
    return true;
}

/**
 * @brief 设置按键模式
 * @param mode_flags 模式标志位
 */
void Key_SetMode(uint8_t mode_flags) { g_key.mode = mode_flags; }
/**
 * @brief 获取按键模式
 * @return 当前模式标志位
 */
uint8_t Key_GetMode(void) { return g_key.mode; }
/**
 * @brief 设置允许长按/连发的按键掩码
 * @param mask 按键掩码
 */
void Key_SetLongEnableMask(uint32_t mask) { g_key.long_enable_mask = mask & KEY_ALL_MASK; }
/**
 * @brief 获取允许长按/连发的按键掩码
 * @return 按键掩码
 */
uint32_t Key_GetLongEnableMask(void) { return g_key.long_enable_mask; }
/**
 * @brief 获取当前稳定按下的按键掩码
 * @return 按键掩码
 */
uint32_t Key_GetPressedMask(void) { return g_key.stable_mask; }
/**
 * @brief 获取当前原始按下的按键掩码
 * @return 按键掩码
 */
uint32_t Key_GetRawMask(void) { return g_key.raw_mask; }
/**
 * @brief 设置时序参数
 * @param t 指向时序参数结构体的指针
 */
void Key_SetTiming(const key_timing_t* t)
{
    if(!t) return;
    if(t->debounce_ms) g_key.timing.debounce_ms = t->debounce_ms;
    if(t->repeat_ms)   g_key.timing.repeat_ms   = t->repeat_ms;
    if(t->long_ms)     g_key.timing.long_ms     = t->long_ms;
}
/**
 * @brief 获取按键按下状态（考虑高/低有效）
 * @param port      GPIO端口
 * @param pin       GPIO引脚
 * @param active_high 引脚高电平有效
 */
static inline uint8_t read_pressed(GPIO_TypeDef* port, uint16_t pin, uint8_t active_high)
{
    GPIO_PinState s = HAL_GPIO_ReadPin(port, pin);
    return active_high ? (s == GPIO_PIN_SET) : (s == GPIO_PIN_RESET);
}

/**
 * @brief 扫描一次，若有事件则通过 out_evt 返回
 * @param out_evt  指向事件对象的指针（非空）
 * @return true 有事件；false 无事件
 */
bool Key_Scan(key_event_t* out_evt)
{
    if(!out_evt) return false;
    uint32_t now = key_get_tick();

    /* 采样原始状态 */
    uint32_t new_raw_mask = 0;
    for(uint8_t i=0; i<KEY_ID_MAX; ++i){
        key_slot_t* k = &g_key.slot[i];
        if(!k->registered) continue;
        uint8_t raw = read_pressed(k->port, k->pin, k->active_high);
        if(raw != k->raw_pressed){
            k->raw_pressed = raw;
            k->debounce_tick = now;
        }
        if(raw) new_raw_mask |= KEY_BIT(i);
    }
    g_key.raw_mask = new_raw_mask;

    /* 边沿与事件（优先返回最先发现的一个事件） */
    for(uint8_t i=0; i<KEY_ID_MAX; ++i){
        key_slot_t* k = &g_key.slot[i];
        if(!k->registered) continue;

        /* 去抖窗口内不改变稳定态 */
        if(TICK_DIFF(now, k->debounce_tick) < g_key.timing.debounce_ms){
            continue;
        }

        uint8_t stable_now = k->raw_pressed;
        if(stable_now != k->stable_pressed){
            /* 稳定边沿 */
            k->stable_pressed = stable_now;
            if(stable_now){
                /* DOWN */
                g_key.stable_mask |= KEY_BIT(i);
                k->press_tick = now;
                k->last_repeat_tick = now;
                k->long_reported = 0;
                if(g_key.mode & KEY_MODE_REPORT_DOWN_UP){
                    out_evt->id = (key_id_t)i;
                    out_evt->type = KEY_EVT_DOWN;
                    return true;
                }
            }else{
                /* UP */
                g_key.stable_mask &= ~KEY_BIT(i);

                if(g_key.mode & KEY_MODE_REPORT_DOWN_UP){
                    out_evt->id = (key_id_t)i;
                    out_evt->type = KEY_EVT_UP;
                    return true;
                }
                if((g_key.mode & KEY_MODE_CLICK_ON_RELEASE) && !k->long_reported){
                    out_evt->id = (key_id_t)i;
                    out_evt->type = KEY_EVT_CLICK;
                    /* 复位长按标记 */
                    k->long_reported = 0;
                    return true;
                }
                /* 复位长按标记 */
                k->long_reported = 0;
            }
        }
    }

    /* 长按/连发（按下且允许时） */
    for(uint8_t i=0; i<KEY_ID_MAX; ++i){
        key_slot_t* k = &g_key.slot[i];
        if(!k->registered || !k->stable_pressed) continue;

        uint32_t bit = KEY_BIT(i);
        bool long_allowed = (g_key.long_enable_mask & bit) != 0;
        if(!long_allowed) continue;

        uint32_t held = TICK_DIFF(now, k->press_tick);

        /* 首次长按 */
        if(!k->long_reported && (held >= g_key.timing.long_ms)){
            k->long_reported = 1;
            if(g_key.mode & (KEY_MODE_LONG_ONCE | KEY_MODE_REPEAT)){
                out_evt->id = (key_id_t)i;
                out_evt->type = KEY_EVT_LONG;
                k->last_repeat_tick = now;
                return true;
            }
        }

        /* 连发 */
        if(k->long_reported && (g_key.mode & KEY_MODE_REPEAT)){
            if(TICK_DIFF(now, k->last_repeat_tick) >= g_key.timing.repeat_ms){
                k->last_repeat_tick = now;
                out_evt->id = (key_id_t)i;
                out_evt->type = KEY_EVT_REPEAT;
                return true;
            }
        }
    }

    return false;
}

/* 初始化GPIO */
static void Key_Port_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin : PA0 */
	GPIO_InitStruct.Pin = KEYB_PIN | KEYY_PIN | KEYL_PIN | KEYR_PIN | KEYN_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KEYB_PORT, &GPIO_InitStruct);
}

/* 注册默认按键 */
static bool Key_RegisterDefaultKeys(void)
{
    bool ok = true;
    ok &= Key_RegisterGPIO(KEY_ID_B, KEYB_PORT, KEYB_PIN, 0);
    ok &= Key_RegisterGPIO(KEY_ID_Y, KEYY_PORT, KEYY_PIN, 0);
    ok &= Key_RegisterGPIO(KEY_ID_L, KEYL_PORT, KEYL_PIN, 0);
    ok &= Key_RegisterGPIO(KEY_ID_R, KEYR_PORT, KEYR_PIN, 0);
    ok &= Key_RegisterGPIO(KEY_ID_N, KEYN_PORT, KEYN_PIN, 0);
    return ok;
}

/* 初始化按键 */
void Key_Init(void)
{
    memset(&g_key.slot[0], 0, sizeof(g_key.slot));
    g_key.raw_mask = 0;
    g_key.stable_mask = 0;
    Key_Port_Init();
    Key_RegisterDefaultKeys();
    Key_SetMode(KEY_MODE_CLICK_ON_RELEASE | KEY_MODE_LONG_ONCE | KEY_MODE_REPEAT);
    Key_SetLongEnableMask(KEY_BIT(KEY_ID_Y) | KEY_BIT(KEY_ID_N));
}
