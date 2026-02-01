/**
  ******************************************************************************
  * @file    STM32F4xx_IAP/inc/common.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file provides all the headers of the common functions.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __COMMON_H
#define __COMMON_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Constants used by Serial Command Line Mode */
#define CMD_STRING_SIZE       128

/* Common routines */
#define IS_AF(c)  ((c >= 'A') && (c <= 'F'))
#define IS_af(c)  ((c >= 'a') && (c <= 'f'))
#define IS_09(c)  ((c >= '0') && (c <= '9'))
#define ISVALIDHEX(c)  IS_AF(c) || IS_af(c) || IS_09(c)
#define ISVALIDDEC(c)  IS_09(c)
#define CONVERTDEC(c)  (c - '0')

#define CONVERTHEX_alpha(c)  (IS_AF(c) ? (c - 'A'+10) : (c - 'a'+10))
#define CONVERTHEX(c)   (IS_09(c) ? (c - '0') : CONVERTHEX_alpha(c))

#define USBPutString(x) USB_PutString((uint8_t*)(x))

// 定义升级信息结构体
typedef struct {
    uint32_t magic_word;      // 魔法字，如 0xDEADBEEF，用于标识此结构体有效
    uint32_t upgrade_status;  // 升级状态标志
    uint32_t new_app_size;    // 新APP的固件大小
    uint32_t new_app_crc32;   // 新APP的CRC32校验和
} UpgradeInfo_t;

// 定义升级状态
#define UPGRADE_STATUS_NONE       0xFFFFFFFF
#define UPGRADE_STATUS_START      0xABCD1234
#define UPGRADE_STATUS_COMPLETE   0x5A5A5A5A
#define MAGIC_WORD                0xDEADBEEF

/* Exported functions ------------------------------------------------------- */
void Int2Str(uint8_t* str,int32_t intnum);
uint32_t Str2Int(uint8_t *inputstr,int32_t *intnum);
uint32_t GetIntegerInput(int32_t * num);
uint32_t USB_KeyPressed(uint8_t *key);
uint8_t GetKey(void);
void USB_PutChar(uint8_t c);
void USB_PutString(char *s);
void GetInputString(uint8_t * buffP);
uint32_t Calculate_CRC32(uint32_t start_address, uint32_t size);

#endif  /* __COMMON_H */

/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE******/
