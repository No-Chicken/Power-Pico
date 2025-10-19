/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_MessageTask.h"
#include "usart.h"

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
    if(uart_receive_flag == 1) {

      // clear the flag
      uart_receive_flag = 0;
      // check the received message is "update\r\n" ?
      if(strcmp((const char *)uart_receive_buf, "update\r\n") == 0) {
        // send response
        printf("start update\r\n");
        // jump to bootloader
      } else {
        // do nothing
      }

    }
		osDelay(500);
	}
}
