#ifndef TEST_USER_TASKS_INIT_H
#define TEST_USER_TASKS_INIT_H

#include <stdint.h>

#define CMD_TX_FRAME_SIZE 58U
#define FLAG_CMD_TX_READY 0x0004U
#define osWaitForever     0xFFFFFFFFU
#define osOK              0

typedef void *osMessageQueueId_t;
typedef void *osThreadId_t;
typedef int32_t osStatus_t;

typedef struct {
    uint16_t length;
    uint8_t data[CMD_TX_FRAME_SIZE];
} CmdTxFrame_t;

extern osMessageQueueId_t CmdTxQueue;
extern osThreadId_t MessageSendTaskHandle;

osStatus_t osMessageQueuePut(osMessageQueueId_t queue,
                             const void *message,
                             uint8_t priority,
                             uint32_t timeout);
uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags);

#endif
