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
#include "ui.h"

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
  PowerData_t power_data;
  uint32_t _time = 1; // default delay time
  while(1)
  {
    // 按键事件处理
    if(osMessageQueueGet(Key_MessageQueue, &key_event, NULL, 0)==osOK) {
      lv_lib_pm_handle_key_event(&key_event);
    }
    // 数据更新处理
    if(osMessageQueueGet(PowerDataQueue, &power_data, NULL, 0)==osOK) {
      // 刷新UI层的电压电流数据变量
      ui_update_vol_cur_varables(power_data.voltage, power_data.current);
    }
		_time = lv_timer_handler();
    // 限制最大休眠时间，保证按键队列被及时读取
    if(_time > 30) {
        _time = 30;
    }
    osDelay(_time);
	}
}
