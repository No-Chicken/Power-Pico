/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "main.h"
#include "adc.h"
#include "gate.h"
#include "usart.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

// 定义档位阈值
#define LOW_TO_MID_THRESHOLD 55000 // 550uA
#define MID_TO_LOW_THRESHOLD 45000 // 450uA (滞回)
#define MID_TO_HIGH_THRESHOLD 5000000 // 50mA
#define HIGH_TO_MID_THRESHOLD 4500 // 45mA (滞回)

// 定义连续超过阈值的次数
#define SWITCH_COUNT_THRESHOLD 2

// 切换计数器
uint8_t low_to_mid_count = 0;
uint8_t mid_to_low_count = 0;
uint8_t mid_to_high_count = 0;
uint8_t high_to_mid_count = 0;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

// 检查当前电流并根据阈值切换流路
void check_and_switch_flow_route(uint8_t unit, uint32_t cur) {
  if (unit == LOW_CUR) {
      if (cur > LOW_TO_MID_THRESHOLD) {
          low_to_mid_count++;
          if (low_to_mid_count >= SWITCH_COUNT_THRESHOLD) {
              flow_route_selection(MID_CUR); // 切换到中档位
              low_to_mid_count = 0; // 重置计数器
          }
      } else {
          low_to_mid_count = 0; // 清零计数器
      }
  } else if (unit == MID_CUR) {
      if (cur > MID_TO_HIGH_THRESHOLD) {
          mid_to_high_count++;
          if (mid_to_high_count >= SWITCH_COUNT_THRESHOLD) {
              flow_route_selection(HIGH_CUR); // 切换到高档位
              mid_to_high_count = 0; // 重置计数器
          }
      } else if (cur < MID_TO_LOW_THRESHOLD) {
          mid_to_low_count++;
          if (mid_to_low_count >= SWITCH_COUNT_THRESHOLD) {
              flow_route_selection(LOW_CUR); // 切换到低档位
              mid_to_low_count = 0; // 重置计数器
          }
      } else {
          mid_to_high_count = 0; // 清零计数器
          mid_to_low_count = 0; // 清零计数器
      }
  } else if (unit == HIGH_CUR) {
      if (cur < HIGH_TO_MID_THRESHOLD) {
          high_to_mid_count++;
          if (high_to_mid_count >= SWITCH_COUNT_THRESHOLD) {
              flow_route_selection(MID_CUR); // 切换到中档位
              high_to_mid_count = 0; // 重置计数器
          }
      } else {
          high_to_mid_count = 0; // 清零计数器
      }
  }
}

/**
  * @brief  task for send messages or data
  * @param  argument: Not used
  * @retval None
  */
void UartSendTask(void *argument)
{
  uint8_t keystr = 0;
  uint16_t cur_bias = 4096 / 2;
	while(1)
	{
    if(osMessageQueueGet(Key_MessageQueue, &keystr, NULL, 0)==osOK)
		{
      // UART6_TX_Send((uint8_t *)"key pressed\r\n", 13);
    }
    // send protocol frame
    for(int i = 0 ; i < ADC_TIMES ; i++)
    {
      // direct & unit
      uint8_t dir_unit = 0x00; // 0x00 means: positive, (A) unit
      uint8_t unit = Gate_get_status(); // 0 - low current(500uA), 1 - mid current(50mA), 2 - high current(5A)
      dir_unit = (unit & 0x0F); // unit
      // current
      uint32_t cur;
      uint64_t cur_adc; // 64位避免溢出
      // voltage = VADC/4096 * 3.0Vref * 11( this means: 1MR + 100kR)
      uint16_t volt = adc_buf[i][3] * 3 * 11 * 100 / 4096; // 100times
      // cur direction and value
      if(adc_buf[i][2-unit] > cur_bias) {
        dir_unit |= 0x10; // negative
        cur_adc = adc_buf[i][2-unit] - cur_bias;
      }
      else {
        cur_adc = cur_bias - adc_buf[i][2-unit];
      }
      // cur = (IADC/4096 * 3.0Vref - 1.5Vref) / 50(ina199) / R
      if(unit == HIGH_CUR) {
        cur = cur_adc * 3 * 100 * 1000 / HIGH_CUR_RES / 50 / 4096; // 100times. (mA)
      }
      else if(unit == MID_CUR) {
        cur = cur_adc * 3 * 100 * 1000000 / MID_CUR_RES / 50 / 4096; // 100times. (uA)
      }
      else if(unit == LOW_CUR) {
        cur = cur_adc * 3 * 100 * 1000000 / LOW_CUR_RES / 50 / 4096; // 100times. (uA)
      }

      check_and_switch_flow_route(unit, cur); // 检查并切换流路

      // protocol frame
      uint8_t protocol_frame[11] = {0x55, 0xAA, dir_unit, (cur>>24 & 0xff), (cur>>16 & 0xff), (cur>>8 & 0xff), (cur & 0xff), (volt>>8 & 0xff), (volt & 0xff), 0xAA, 0x55};
      UART6_TX_Send(protocol_frame, sizeof(protocol_frame));
    }
		osDelay(1);
	}
}
