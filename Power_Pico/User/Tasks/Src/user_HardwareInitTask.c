/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "usart.h"
#include "tim.h"
#include "stm32f4xx_it.h"
#include "adc.h"

// user
#include "user_TasksInit.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "lcd_init.h"
#include "gate.h"

// ui
#include "lvgl.h"
#include "lv_port_disp.h"
#include "ui.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* protocol frame
+ 0x55 0xAA header
+ 1byte direct & unit, 0x0? for positive, 0x1? for negative,
  0x?0 for uA (0-500.00uA), 0x?1 for uA (0.50-500.00mA), 0x?2 for mA (0.50-5.00A)
+ 4byte current (100 times)
+ 2byte voltage (100 times)
+ 0xAA 0x55 footer
*/
uint8_t protocol_frame[] = {0x55, 0xAA, 0x02, 0x00, 0x00, 0x01, 0x23, 0x01, 0xf4, 0xAA, 0x55};

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  hardwares init task
  * @param  argument: Not used
  * @retval None
  */
void HardwareInitTask(void *argument)
{
	while(1)
	{
    vTaskSuspendAll();

    // ADC
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, ADC_TIMES * ADC_CHANELS); /*启动ADC的DMA传输，配合定时器触发ADC转换  12位的ADC对应0-4095 */
    HAL_TIM_Base_Start(&htim2); /*开启定时器，用溢出时间来触发ADC*/

    // uart start
    HAL_UART_Receive_DMA(&huart6, (uint8_t*)uart_receive_str, USART_RX_BUFFER_SIZE);
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
    //
    UART6_TX_Send((uint8_t *)"power-pico\r\n", 13);
    //
    // UART6_TX_Send(protocol_frame, sizeof(protocol_frame));
    //

    // gate, for current flow route selection, high current by default
    Gate_Port_Init();
    flow_route_selection(HIGH_CUR);

    // PWM Start for LCD backlight
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);

    // key
    Key_Port_Init();

    // lcd
    // done in lvgl disp init

    // ui
    // LVGL and disp init
    lv_init();
    lv_port_disp_init();
    ui_init();

    xTaskResumeAll();
		vTaskDelete(NULL);
		osDelay(500);
	}
}


