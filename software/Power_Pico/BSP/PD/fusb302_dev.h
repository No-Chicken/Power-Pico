#ifndef __FUSB302_DEV_H__
#define __FUSB302_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "FUSB302_UFP.h"
#include "PD_UFP_Protocol.h"

typedef uint8_t status_power_t;

enum {
    PD_UFP_VOLTAGE_LED_OFF      = 0,
    PD_UFP_VOLTAGE_LED_5V       = 1,
    PD_UFP_VOLTAGE_LED_9V       = 2,
    PD_UFP_VOLTAGE_LED_12V      = 3,
    PD_UFP_VOLTAGE_LED_15V      = 4,
    PD_UFP_VOLTAGE_LED_20V      = 5,
    PD_UFP_VOLTAGE_LED_AUTO     = 6
};
typedef uint8_t PD_UFP_VOLTAGE_LED_t;

enum {
    PD_UFP_CURRENT_LED_OFF      = 0,
    PD_UFP_CURRENT_LED_LE_1V    = 1,
    PD_UFP_CURRENT_LED_LE_3V    = 2,
    PD_UFP_CURRENT_LED_GT_3V    = 3,
    PD_UFP_CURRENT_LED_AUTO     = 4
};
typedef uint8_t PD_UFP_CURRENT_LED_t;

enum {
    STATUS_POWER_NA = 0,
    STATUS_POWER_TYP,
    STATUS_POWER_PPS
};
typedef uint8_t status_power_t;

enum {
    PD_SYS_STATE_INIT = 0,
    PD_SYS_STATE_READY,
    PD_SYS_STATE_ERROR
};
typedef uint8_t PD_user_state_t;

typedef enum {
    PD_STATE_IDLE,              // 空闲状态
    PD_STATE_NEGOTIATING,       // 正在协商中 (已发送请求，等待PS_RDY)
    PD_STATE_SUCCESS_PPS,       // 成功协商到PPS模式
    PD_STATE_SUCCESS_FIXED,     // 成功协商到固定电压
    PD_STATE_FAILED,            // 协商失败 (收到Reject或超时)
    PD_STATE_GET_CAPS           // 刚刚收到能力列表，准备发起第一次请求
} App_PD_State_t;

typedef struct
{

    App_PD_State_t state;

    PD_protocol_t protocol;

    // Power ready power
    uint16_t ready_voltage;
    uint16_t ready_current;

    // PPS setup
    uint16_t PPS_voltage_next;
    uint8_t PPS_current_next;
    // Status
    uint8_t status_initialized;
    uint8_t status_src_cap_received;
    status_power_t status_power;
    // Timer and counter for PD Policy
    uint32_t time_polling;
    uint32_t time_wait_src_cap;
    uint32_t time_wait_ps_rdy;
    uint32_t time_PPS_request;
    uint8_t get_src_cap_retry_count;
    uint8_t wait_src_cap;
    uint8_t wait_ps_rdy;
    uint8_t send_request;
    uint8_t cc;
    char power_list[7][24];
    uint8_t power_list_num;
    uint8_t power_index;
    float power_v;
    float power_i;
}App_PD_t;

extern FUSB302_dev_t fusb302_dev;
extern App_PD_t app_pd;

uint8_t fusb302_dev_init(void);
void handle_FUSB302_event(FUSB302_event_t events);
bool is_power_ready(void);
bool is_PPS_ready(void);
void send_power_request(void);
bool fusb302_timer(void);

#ifdef __cplusplus
}
#endif
#endif
