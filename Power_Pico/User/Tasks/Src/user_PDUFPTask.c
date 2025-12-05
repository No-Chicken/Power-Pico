/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_PDUFPTask.h"
#include "main.h"
#include "fusb302_dev.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/**
  * @brief  task for PD_UFP
  * @param  argument: Not used
  * @retval None
  */
void PDUFPTask(void *argument)
{
  // 快充诱骗时需要另外一个端口也供电，因为快充口会有可能断电
  // 可以检测status_power来判断是否有PPS供电
  PD_UI_MSG_t ui_msg;
  uint8_t _working_status = 0;
  // set
  PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_20V);
	while(1)
	{
    if(osMessageQueueGet(PD_UI_MessageQueue, &ui_msg, NULL, 1)==osOK) {
      if(ui_msg.event == PD_UI_EVT_START) {
        // FUSB CC pin connect
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        PD_protocol_init(&app_pd.protocol);
        PD_protocol_set_PPS(&app_pd.protocol, PPS_V(5.0), PPS_A(1.0), true); // 初始PPS 5V 1A
        send_power_request();
        _working_status = 1;
      } else if(ui_msg.event == PD_UI_EVT_STOP) {

      } else if(ui_msg.event == PD_UI_EVT_SET_PPS) {
        // 设置PPS电压电流
        if(ui_msg.set_voltage > 0 && ui_msg.set_current > 0 && ui_msg.set_voltage <= 20.0 && ui_msg.set_current <= 5.0) {
          PD_protocol_set_PPS(&app_pd.protocol, PPS_V(ui_msg.set_voltage), PPS_A(ui_msg.set_current), true);
          send_power_request();
        }
      }
    }
    if(_working_status)
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
