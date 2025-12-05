/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_PDUFPTask.h"
#include "main.h"
#include "fusb302_dev.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

uint8_t _pd_working_status = 0;
osTimerId_t PD_UFP_Task_timer_id;

/* Private function prototypes -----------------------------------------------*/

static void _timer_callback(void *argument)
{
    PD_handle_event_t ready;
    static uint8_t time_count = 0;
    time_count++;
    if(is_PPS_ready()) {
        ready = PD_EVT_PPS_READY;
        osMessageQueuePut(PD_handle_event_MsgQueue, &ready, 0, 1);
        time_count = 0;
        osTimerStop(PD_UFP_Task_timer_id);
    } else if(time_count >= 10) { // 5秒超时
        ready = PD_EVT_PPS_FAILED;
        osMessageQueuePut(PD_handle_event_MsgQueue, &ready, 0, 1);
        time_count = 0;
        osTimerStop(PD_UFP_Task_timer_id);
    }
}

/**
  * @brief  task for PD_UFP
  * @param  argument: Not used
  * @retval None
  */
void PDUFPTask(void *argument)
{
  // 快充诱骗时需要另外一个端口也供电，因为快充口会有可能断电
  // 可以检测status_power来判断是否有PPS供电
  PD_command_msg_t ui_msg;
  // 创建定时器
  PD_UFP_Task_timer_id = osTimerNew(_timer_callback, osTimerPeriodic, NULL, NULL);
	while(1)
	{
    // 非阻塞获取消息
    if(osMessageQueueGet(PD_cmd_MessageQueue, &ui_msg, NULL, 0)==osOK) {
      if(!_pd_working_status && ui_msg.event == PD_CMD_START) {
        // FUSB CC pin connect
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        PD_protocol_init(&app_pd.protocol);
        PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_5V);
        PD_protocol_set_PPS(&app_pd.protocol, PPS_V(5.0), PPS_A(1.0), false); // 初始PPS 5V 1A
        send_power_request();
        if(osTimerIsRunning(PD_UFP_Task_timer_id)) {
            osTimerStop(PD_UFP_Task_timer_id);
        }
        osTimerStart(PD_UFP_Task_timer_id, 1000); // 1s
        _pd_working_status = 1;

      } else if(_pd_working_status && ui_msg.event == PD_CMD_STOP) {



      } else if(_pd_working_status && ui_msg.event == PD_CMD_SET_PPS) {
        // 设置PPS电压电流
        if(ui_msg.set_voltage > 0 && ui_msg.set_current > 0 && ui_msg.set_voltage <= 20.0 && ui_msg.set_current <= 5.0) {
          PD_protocol_set_PPS(&app_pd.protocol, PPS_V(ui_msg.set_voltage), PPS_A(ui_msg.set_current), false);
          send_power_request();
        }
      }
    }

    if(_pd_working_status)
    {
      if (fusb302_timer() || HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET)
      {
          FUSB302_event_t FUSB302_events = 0;
          for (uint8_t i = 0; i < 3 && FUSB302_alert(&fusb302_dev, &FUSB302_events) != FUSB302_SUCCESS; i++) {}
          if (FUSB302_events) {
              handle_FUSB302_event(FUSB302_events);
          }
      }
    }
		osDelay(1);
	}
}
