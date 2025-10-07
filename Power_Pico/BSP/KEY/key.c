#include "key.h"
#include "cmsis_os.h"

void Key_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = KEYB_PIN | KEYY_PIN | KEYL_PIN | KEYR_PIN | KEYN_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYB_PORT, &GPIO_InitStruct);
}

// mode: 0 - sigle press, 1 - long press
// return: 0 - no key press, 1 - KEYB, 2 - KEYY, 3 - KEYL, 4 - KEYR, 5 - KEYN
uint8_t KeyScan(uint8_t mode)
{
	static uint8_t key_up = 1;
	static uint8_t key_down = 0;
	uint8_t keyvalue = 0;

	if(mode)
	{
		key_up = 1;
		key_down = 0;
	}

	if( key_up && (!KEYB || !KEYY || !KEYL || !KEYR || !KEYN) )
	{
		osDelay(3); // ensure the key pressed
		if(!KEYB)
			key_down = KEYB_NUM;
		else if(!KEYY)
			key_down = KEYY_NUM;
		else if(!KEYL)
			key_down = KEYL_NUM;
		else if(!KEYR)
			key_down = KEYR_NUM;
		else if(!KEYN)
			key_down = KEYN_NUM;
		if(key_down)
			key_up = 0;
	}

	// 松开才有效
	if ( key_down && KEYB && KEYY && KEYL && KEYR && KEYN )
	{
		osDelay(3);//ensure the key
		if(KEYB && KEYY && KEYL && KEYR && KEYN)
		{
			key_up = 1;
			keyvalue = key_down;
			key_down = 0;
		}
	}

	return keyvalue;
}

