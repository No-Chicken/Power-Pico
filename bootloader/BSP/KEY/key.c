#include "key.h"
#include "delay.h"

void Key_Port_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin : PA0 */
	GPIO_InitStruct.Pin = KEYB_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KEYB_PORT, &GPIO_InitStruct);

}

uint8_t KeyScan(uint8_t mode)
{
	static uint8_t key_up = 1;
	uint8_t keyvalue=0;
	if(mode) key_up = 1;
	if( key_up && !KEYB)
	{
		delay_ms(3);//ensure the key is down
		if(!KEYB) keyvalue = 1;
		if(keyvalue) key_up = 0;
	}
	else
	{
		delay_ms(3);//ensure the key is up
		if(KEYB)
			key_up = 1;
	}
	return keyvalue;
}

