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

    // uart start
    HAL_UART_Receive_DMA(&huart6, (uint8_t*)uart_receive_str, USART_RX_BUFFER_SIZE);
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
    //
    UART6_TX_Send((uint8_t *)"power-pico\r\n", 13);
    // HAL_UART_Transmit_DMA(&huart6,(uint8_t *)"power-pico\r\n",13);

    // ADC
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_packet1.data, ADC_TIMES * ADC_CHANELS); /*启动ADC的DMA传输，配合定时器触发ADC转换  12位的ADC对应0-4095 */
    HAL_TIM_Base_Start(&htim2); /*开启定时器，用溢出时间来触发ADC*/

    adc_packet1.header[0] = 0x55;
    adc_packet1.header[1] = 0xAA;
    adc_packet1.header[2] = ADC_TIMES;
    adc_packet1.header[3] = ADC_CHANELS;

    adc_packet2.header[0] = 0x55;
    adc_packet2.header[1] = 0xAA;
    adc_packet1.header[2] = ADC_TIMES;
    adc_packet1.header[3] = ADC_CHANELS;

    // gate, for current flow route selection, high current by default
    Gate_Port_Init();
    flow_route_selection(HIGH_CUR);

    // PWM Start for LCD backlight
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);

    // beep
    // HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_4);

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


