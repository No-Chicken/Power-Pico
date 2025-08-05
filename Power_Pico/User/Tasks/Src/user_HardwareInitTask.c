/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "usart.h"
#include "tim.h"
#include "stm32f4xx_it.h"

// user
#include "user_TasksInit.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "lcd_init.h"

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

    // usart start

    // PWM Start for LCD backlight
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);

    // key
    Key_Port_Init();

    // lcd
    // done in lvgl disp init
    LCD_Init();
    LCD_Fill(0,0, LCD_W, LCD_H, BLACK);
    LCD_Set_Light(50);
    LCD_ShowString(72,LCD_H/2,(uint8_t*)"Welcome!", WHITE, BLACK, 24, 0);

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


