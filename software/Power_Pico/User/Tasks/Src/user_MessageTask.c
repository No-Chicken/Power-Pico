/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_MessageTask.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"
#include "BL24C02.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  task for check the usart message
  * @param  argument: Not used
  * @retval None
  */
void MessageTask(void *argument)
{
	while(1)
	{
    if(get_update_request()) {

      // clear the flag
      set_update_request(false);
      // set the EEPROM flag
      EEPROM_UpdateCommand_Write(true);
      HAL_Delay(100);
      USER_USB_DEVICE_DeInit();
      // 给予PC足够的时间来识别设备断开
      HAL_Delay(500);
      // reset
      NVIC_SystemReset();
    }
		osDelay(500);
	}
}
