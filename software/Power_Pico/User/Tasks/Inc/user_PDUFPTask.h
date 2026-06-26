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
    PD_CMD_SET_PPS,       // PPS步进调节
    PD_CMD_SET_PD_FIXED   // PD fixed固定档位调节
} PD_command_t;

// 定义PD UFP Task任务发送到UI层的处理信号 //
typedef enum {
    PD_EVT_PPS_READY = 0, // PPS模式协商成功
    PD_EVT_FIXED_READY,   // PD固定档位成功
    PD_EVT_PPS_FAILED,
} PD_handle_event_t;

// 
enum {
    PD_FIXED_VOL_LEVEL_5V = 0,
    PD_FIXED_VOL_LEVEL_9V,
    PD_FIXED_VOL_LEVEL_12V,
    PD_FIXED_VOL_LEVEL_15V,
    PD_FIXED_VOL_LEVEL_20V
};
typedef uint8_t PD_FIXED_VOL_LEVEL;

// UI层发送到PD UFP Task任务的命令信号结构体 //
typedef struct
{
    PD_command_t event;
    float pps_set_voltage;
    float pps_set_current;
    PD_FIXED_VOL_LEVEL pd_fixed_level;
} PD_command_msg_t;


void PDUFPTask(void *argument);


#ifdef __cplusplus
}
#endif

#endif

