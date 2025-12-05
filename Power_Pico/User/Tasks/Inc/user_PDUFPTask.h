#ifndef __USER_PDUFPTASK_H__
#define __USER_PDUFPTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "user_TasksInit.h"

// 定义UI层发送给PD UFP Task的指令的信号 //
typedef enum {
    PD_CMD_START = 0,
    PD_CMD_STOP,
    PD_CMD_SET_PPS,
} PD_command_t;

// 定义PD UFP Task任务发送到UI层的处理信号 //
typedef enum {
    PD_EVT_PPS_READY = 0,
    PD_EVT_PPS_FAILED,
} PD_handle_event_t;

// UI层发送到PD UFP Task任务的命令信号结构体 //
typedef struct
{
    PD_command_t event;
    float set_voltage;
    float set_current;
} PD_command_msg_t;


void PDUFPTask(void *argument);


#ifdef __cplusplus
}
#endif

#endif

