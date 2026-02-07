/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "tim.h"
#include "stm32f4xx_it.h"
#include "adc.h"
#include "i2c.h"

// user
#include "user_TasksInit.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "lcd_init.h"
#include "gate.h"
#include "fusb302_dev.h"
#include "BL24C02.h" // settings

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

    // gate, for current flow route selection, high current by default
    Gate_Port_Init();
    flow_route_selection(HIGH_CUR);

    // ADC sample start
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_raw_buffer, ADC_TIMES*2 * ADC_CHANNELS); /*启动ADC的DMA传输，配合定时器触发ADC转换  12位的ADC对应0-4095 */
    HAL_TIM_Base_Start(&htim2); /*开启定时器，用溢出时间来触发ADC*/

    // PWM Start for LCD backlight
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);

    // beep
    // HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_4);

    // key
    Key_Init();

    // system settings from eeprom
    if(!EEPROM_Init_Check()) {
      EEPROM_SysSetting_Get();
    }

    // FUSB CC pin dis connect
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);

    // FUSB302 init
    fusb302_dev_init();

    // lcd
    // done in lvgl disp init

    // tim5 for elapsed time (us)
    HAL_TIM_Base_Start(&htim5);

    // ui
    // LVGL and disp init
    lv_init();
    lv_port_disp_init();
    ui_init();

    xTaskResumeAll();
		vTaskDelete(NULL);
	}
}


