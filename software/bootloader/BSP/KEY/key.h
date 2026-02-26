#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

//KEY1
#define KEYB_PORT	GPIOA
#define KEYB_PIN	GPIO_PIN_0
#define KEYB HAL_GPIO_ReadPin(KEYB_PORT,KEYB_PIN)

void Key_Port_Init(void);
uint8_t KeyScan(uint8_t mode);


#ifdef __cplusplus
}
#endif
#endif

