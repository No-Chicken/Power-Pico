#ifndef __USER_TASKSINIT_H__
#define __USER_TASKSINIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "cmsis_os.h"

extern osMessageQueueId_t Key_MessageQueue;
extern osMessageQueueId_t PD_cmd_MessageQueue;
extern osMessageQueueId_t PD_handle_event_MsgQueue;

void User_Tasks_Init(void);
void TaskTickHook(void);

#ifdef __cplusplus
}
#endif

#endif

