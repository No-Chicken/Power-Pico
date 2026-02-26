/* Private includes -----------------------------------------------------------*/
//includes

//bsp
#include "key.h"

// tasks
#include "user_TasksInit.h"
#include "user_LVGLTask.h"

//gui
#include "lvgl.h"
#include "lv_lib_pm.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


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
