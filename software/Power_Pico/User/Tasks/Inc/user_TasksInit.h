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

#define CMD_RX_CHUNK_SIZE  64U
#define CMD_RX_QUEUE_DEPTH 8U
#define CMD_TX_QUEUE_DEPTH 4U
#define CMD_TX_FRAME_SIZE  58U

typedef struct {
    uint16_t length;
    uint8_t data[CMD_RX_CHUNK_SIZE];
} CmdRxChunk_t;

typedef struct {
    uint16_t length;
    uint8_t data[CMD_TX_FRAME_SIZE];
} CmdTxFrame_t;

// 定义一个标志位，表示 ADC 数据已经准备好，在ADC DMA中断中被置位
// 在 MessageSendTask 中等待这个标志位，表示可以发送 ADC 数据了
#define FLAG_ADC_HALF_READY  0x0001U  // 0000 0001 ADC 半满
#define FLAG_ADC_FULL_READY  0x0002U  // 0000 0010 ADC 全满
#define FLAG_CMD_TX_READY     0x0004U
#define FLAG_USB_TX_COMPLETE  0x0008U

extern osThreadId_t MessageSendTaskHandle;
extern osThreadId_t MessageReceiveTaskHandle;

extern osMessageQueueId_t Key_MessageQueue;
extern osMessageQueueId_t PD_cmd_MessageQueue;
extern osMessageQueueId_t PD_handle_event_MsgQueue;
extern osMessageQueueId_t PowerDataQueue;
extern osMessageQueueId_t CmdRxQueue;
extern osMessageQueueId_t CmdTxQueue;
extern volatile uint32_t CmdRxOverflowCount;

void User_Tasks_Init(void);

#ifdef __cplusplus
}
#endif

#endif

