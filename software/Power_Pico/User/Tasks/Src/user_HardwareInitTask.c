/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "tim.h"
#include "adc.h"
#include "user_AdcDataStrategy.h"
#include "i2c.h"
#include "usb_device.h"
#include "user_FixedVoltage.h"

// user
#include "user_TasksInit.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "lcd_init.h"
#include "gate.h"
#include "pico_diag.h"
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

    // usb init
    MX_USB_DEVICE_Init();

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
    Gate_Set_Mode(Sys_Get_CurrentRangeMode());
    pico_diag_storage_init();

    // Keep CC attached; background PD may request only the fixed 5V PDO.
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);

    // FUSB302 init
    user_pd_boot_init();
    /* PD must run during the LCD reset/sleep delays. Other consumers wait
     * for user_hardware_ready and cannot enter partially initialized UI. */
    xTaskResumeAll();
    pico_diag_log(DIAG_PD_BOOT_PROGRESS, 1);

    // lcd
    // done in lvgl disp init

    // tim5 for elapsed time (us)
    HAL_TIM_Base_Start(&htim5);

    // ui
    // LVGL and disp init
    lv_init();
    lv_port_disp_init();
    ui_init();

    #if defined(POWER_PICO_RESTORE_EEPROM_BASELINE)
    if(EEPROM_RestoreLegacyBaseline()) {
        EEPROM_UpdateCommand_Write(true);
        HAL_Delay(100);
        USER_USB_DEVICE_DeInit();
        HAL_Delay(500);
        NVIC_SystemReset();
    }
    #else
    /* Load calibration and perform pending EEPROM migration after UI setup. */
    Sys_AdcCalibration_Init();
    #endif

    user_hardware_ready = true;
    pico_diag_log(DIAG_PD_BOOT_PROGRESS, 2);
		vTaskDelete(NULL);
	}
}


