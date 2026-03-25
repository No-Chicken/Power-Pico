/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
//sys
#include "sys.h"
#include "stdio.h"

//bsp
#include "key.h"

//gui
#include "lvgl.h"
#include "lv_lib_pm.h"

//tasks
#include "user_HardwareInitTask.h"
#include "user_LVGLTask.h"
#include "user_PDUFPTask.h"
#include "user_KeyTask.h"
#include "user_MessageTask.h"

/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Timers --------------------------------------------------------------------*/
osTimerId_t IdleTimerHandle;


/* Tasks ---------------------------------------------------------------------*/
// Hardwares initialization
osThreadId_t HardwareInitTaskHandle;
const osThreadAttr_t HardwareInitTask_attributes = {
  .name = "HardwareInitTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityHigh,
};

// message receive task
osThreadId_t MessageReceiveTaskHandle;
const osThreadAttr_t MessageReceiveTask_attributes = {
  .name = "MessageReceiveTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityNormal2,
};

// message send task
osThreadId_t MessageSendTaskHandle;
const osThreadAttr_t MessageSendTask_attributes = {
  .name = "MessageSendTask",
  .stack_size = 128 * 8,
  .priority = (osPriority_t) osPriorityHigh,
};

// Key task
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityNormal,
};

// PDUFP task
osThreadId_t PDUFPTaskHandle;
const osThreadAttr_t PDUFPTask_attributes = {
  .name = "PDUFPTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow2,
};

// LVGL Handler task
osThreadId_t LvHandlerTaskHandle;
const osThreadAttr_t LvHandlerTask_attributes = {
  .name = "LvHandlerTask",
  .stack_size = 128 * 24,
  .priority = (osPriority_t) osPriorityLow1,
};


/* Message queues ------------------------------------------------------------*/

// Key task 中发送出的按键信息的消息队列
// 流向为 KeyTask -> LVGLtask
osMessageQueueId_t Key_MessageQueue;

// UI layer发送给PDUFPTask任务的命令消息队列
// 流向为 LVGLtask -> PDUFPTask
osMessageQueueId_t PD_cmd_MessageQueue;

// PDUFPTask任务发送给UI层的通知处理情况的消息队列
// 流向为 PDUFPTask -> LVGLtask
osMessageQueueId_t PD_handle_event_MsgQueue;

// 数据处理Task任务发送给UI层的电压电流数据的消息队列
// 流向为 MessageSendTask -> LVGLtask
osMessageQueueId_t PowerDataQueue;

/* Private function prototypes -----------------------------------------------*/
void LvHandlerTask(void *argument);

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void User_Tasks_Init(void)
{
  /* add mutexes, ... */

  /* add semaphores, ... */

  /* start timers, add new ones, ... */

  /* add queues, ... */
	Key_MessageQueue  = osMessageQueueNew(4, sizeof(key_event_t), NULL);
  PD_cmd_MessageQueue = osMessageQueueNew(4, sizeof(PD_command_msg_t), NULL);
  PD_handle_event_MsgQueue = osMessageQueueNew(4, 1, NULL); // uint8_t message
  PowerDataQueue = osMessageQueueNew(8, sizeof(PowerData_t), NULL);

	/* add threads, ... */
  HardwareInitTaskHandle  = osThreadNew(HardwareInitTask, NULL, &HardwareInitTask_attributes);
  PDUFPTaskHandle          = osThreadNew(PDUFPTask, NULL, &PDUFPTask_attributes);
  KeyTaskHandle 			    = osThreadNew(KeyTask, NULL, &KeyTask_attributes);
  MessageReceiveTaskHandle  = osThreadNew(MessageReceiveTask, NULL, &MessageReceiveTask_attributes);
  MessageSendTaskHandle     = osThreadNew(MessageSendTask, NULL, &MessageSendTask_attributes);
  LvHandlerTaskHandle     = osThreadNew(LvHandlerTask, NULL, &LvHandlerTask_attributes);

  /* add events, ... */

	/* add  others ... */


}

