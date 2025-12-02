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

// message task
osThreadId_t MessageTaskHandle;
const osThreadAttr_t MessageTask_attributes = {
  .name = "MessageTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityNormal2,
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
  .stack_size = 128 * 2,
  .priority = (osPriority_t) osPriorityLow2,
};

// LVGL Handler task
osThreadId_t LvHandlerTaskHandle;
const osThreadAttr_t LvHandlerTask_attributes = {
  .name = "LvHandlerTask",
  .stack_size = 128 * 24,
  .priority = (osPriority_t) osPriorityLow,
};


/* Message queues ------------------------------------------------------------*/
//Key message
osMessageQueueId_t Key_MessageQueue;

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

	/* add threads, ... */
  HardwareInitTaskHandle  = osThreadNew(HardwareInitTask, NULL, &HardwareInitTask_attributes);
  // PDUFPTaskHandle          = osThreadNew(PDUFPTask, NULL, &PDUFPTask_attributes);
  KeyTaskHandle 			    = osThreadNew(KeyTask, NULL, &KeyTask_attributes);
  MessageTaskHandle       = osThreadNew(MessageTask, NULL, &MessageTask_attributes);
  LvHandlerTaskHandle     = osThreadNew(LvHandlerTask, NULL, &LvHandlerTask_attributes);

  /* add events, ... */

	/* add  others ... */


}


/**
  * @brief  FreeRTOS Tick Hook, to increase the LVGL tick
  * @param  None
  * @retval None
  */
void TaskTickHook(void)
{
	//to increase the LVGL tick
	lv_tick_inc(1);
}


/**
  * @brief  LVGL Handler task, to run the lvgl
  * @param  argument: Not used
  * @retval None
  */
void LvHandlerTask(void *argument)
{
  key_event_t key_event;
  while(1)
  {
    if(osMessageQueueGet(Key_MessageQueue, &key_event, NULL, 1)==osOK) {
      lv_lib_pm_handle_key_event(&key_event);
    }
		lv_task_handler();
    osDelay(1);
	}
}




