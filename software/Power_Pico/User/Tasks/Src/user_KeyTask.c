/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "main.h"
#include "key.h"
#include "beep.h"
#include "BL24C02.h" // settings

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Key press check task
  * @param  argument: Not used
  * @retval None
  */
void KeyTask(void *argument)
{
	key_event_t key_event;
	while(1)
	{
		if(Key_Scan(&key_event))
		{
			osMessageQueuePut(Key_MessageQueue, &key_event, 0, 1);
			// beep
			if(Sys_Get_KeySoundEnable())
			{
				beep_open();
				osDelay(50);
				beep_close();
			}
		}
		osDelay(10);
	}
}
