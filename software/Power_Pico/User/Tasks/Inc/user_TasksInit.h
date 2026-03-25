#ifndef __USER_TASKSINIT_H__
#define __USER_TASKSINIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "cmsis_os.h"

// 用于数据传输的结构体，包含电压和电流
// 流向为 MessageSendTask -> LVGLtask
typedef struct {
    float voltage;    // 电压 (V)
    float current;    // 电流 (uA)
} PowerData_t;

// 定义一个标志位，表示 ADC 数据已经准备好，在ADC DMA中断中被置位
// 在 MessageSendTask 中等待这个标志位，表示可以发送 ADC 数据了
#define FLAG_ADC_HALF_READY  0x0001U  // 0000 0001 ADC 半满
#define FLAG_ADC_FULL_READY  0x0002U  // 0000 0010 ADC 全满
#define FLAG_USB_UPDATE_REQ  0x0004U  // 0000 0100 USB 请求升级

extern osThreadId_t MessageSendTaskHandle;
extern osThreadId_t MessageReceiveTaskHandle;

extern osMessageQueueId_t Key_MessageQueue;
extern osMessageQueueId_t PD_cmd_MessageQueue;
extern osMessageQueueId_t PD_handle_event_MsgQueue;
extern osMessageQueueId_t PowerDataQueue;

void User_Tasks_Init(void);

#ifdef __cplusplus
}
#endif

#endif

