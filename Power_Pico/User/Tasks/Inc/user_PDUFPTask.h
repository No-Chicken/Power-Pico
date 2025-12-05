#ifndef __USER_PDUFPTASK_H__
#define __USER_PDUFPTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "user_TasksInit.h"

// 定义UI层发送给PDUFPTask的PD诱骗系列的信号 //
typedef enum {
    PD_UI_EVT_START = 0,
    PD_UI_EVT_STOP,
    PD_UI_EVT_SET_PPS,
} PD_UI_EVENT_t;

// 定义PDUFPTask任务发送给UI的信号 //
typedef enum {
    PD_TASK_EVT_PPS_READY = 0,
    PD_TASK_EVT_PPS_FAILED,
} PD_TASK_EVENT_t;

// UI层发送过来PDUFPTask任务消费的信号结构体 //
typedef struct
{
    PD_UI_EVENT_t event;
    float set_voltage;
    float set_current;
} PD_UI_MSG_t;


void PDUFPTask(void *argument);


#ifdef __cplusplus
}
#endif

#endif

