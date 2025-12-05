#ifndef __USER_TASKSINIT_H__
#define __USER_TASKSINIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "cmsis_os.h"

extern osMessageQueueId_t Key_MessageQueue;
extern osMessageQueueId_t PD_UI_MessageQueue;
extern osMessageQueueId_t PD_Task_MessageQueue;

void User_Tasks_Init(void);
void TaskTickHook(void);

#ifdef __cplusplus
}
#endif

#endif

