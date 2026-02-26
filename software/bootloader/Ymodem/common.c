/**
  ******************************************************************************
  * @file    STM32F4xx_IAP/src/common.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file provides all the common functions.
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

/** @addtogroup STM32F4xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "usbd_cdc_if.h"
#include "usbd_core.h" // ??????????????
#include "usb_device.h" // ?????? hUsbDeviceFS

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

// ?? usbd_cdc_if.c ????
extern uint8_t USB_RxBuffer[];
extern volatile uint16_t USB_RxHead;
extern volatile uint16_t USB_RxTail;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Convert an Integer to a string
  * @param  str: The string
  * @param  intnum: The integer to be converted
  * @retval None
  */
void Int2Str(uint8_t* str, int32_t intnum)
{
  uint32_t i, Div = 1000000000, j = 0, Status = 0;

  for (i = 0; i < 10; i++)
  {
    str[j++] = (intnum / Div) + 48;

    intnum = intnum % Div;
    Div /= 10;
    if ((str[j-1] == '0') & (Status == 0))
    {
      j = 0;
    }
    else
    {
      Status++;
    }
  }
}

/**
  * @brief  Convert a string to an integer
  * @param  inputstr: The string to be converted
  * @param  intnum: The integer value
  * @retval 1: Correct
  *         0: Error
  */
uint32_t Str2Int(uint8_t *inputstr, int32_t *intnum)
{
  uint32_t i = 0, res = 0;
  uint32_t val = 0;

  if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X'))
  {
    if (inputstr[2] == '\0')
    {
      return 0;
    }
    for (i = 2; i < 11; i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1; */
        res = 1;
        break;
      }
      if (ISVALIDHEX(inputstr[i]))
      {
        val = (val << 4) + CONVERTHEX(inputstr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 8 digit hex --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }
  else /* max 10-digit decimal input */
  {
    for (i = 0;i < 11;i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1 */
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0))
      {
        val = val << 10;
        *intnum = val;
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0))
      {
        val = val << 20;
        *intnum = val;
        res = 1;
        break;
      }
      else if (ISVALIDDEC(inputstr[i]))
      {
        val = val * 10 + CONVERTDEC(inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 10 digit decimal --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }

  return res;
}

/**
  * @brief  Get an integer from the USB VCP
  * @param  num: The integer
  * @retval 1: Correct
  *         0: Error
  */
uint32_t GetIntegerInput(int32_t * num)
{
  uint8_t inputstr[16];

  while (1)
  {
    GetInputString(inputstr);
    if (inputstr[0] == '\0') continue;
    if ((inputstr[0] == 'a' || inputstr[0] == 'A') && inputstr[1] == '\0')
    {
      USB_PutString("User Cancelled \r\n");
      return 0;
    }

    if (Str2Int(inputstr, num) == 0)
    {
      USB_PutString("Error, Input again: \r\n");
    }
    else
    {
      return 1;
    }
  }
}

/**
  * @brief  Test to see if a key has been pressed on the USB VCP
  * @param  key: The key pressed
  * @retval 1: A key was received
  *         0: No key was received
  */
uint32_t USB_KeyPressed(uint8_t *key)
{
  // Check if there is data in the USB receive ring buffer
  if (USB_RxHead != USB_RxTail)
  {
    *key = USB_RxBuffer[USB_RxTail];
    USB_RxTail = (USB_RxTail + 1) % 2048; // APP_RX_DATA_SIZE
    return 1;
  }
  else
  {
    return 0;
  }
}

/**
  * @brief  Get a key from the USB VCP
  * @param  None
  * @retval The Key Pressed
  */
uint8_t GetKey(void)
{
  uint8_t key = 0;

  /* Waiting for user input */
  while (1)
  {
    if (USB_KeyPressed((uint8_t*)&key)) break;
  }
  return key;

}


uint8_t USB_GetChar(uint8_t *c, uint32_t timeout)
{
    uint32_t tickstart = HAL_GetTick();

    while (1)
    {
        if (USB_RxHead != USB_RxTail)
        {
            *c = USB_RxBuffer[USB_RxTail];
            USB_RxTail = (USB_RxTail + 1) % RING_BUFFER_SIZE;
            return 0;
        }

        if ((HAL_GetTick() - tickstart) > timeout)
        {
            return 1;
        }
    }
}

/**
  * @brief  Send a char to the USB VCP
  * @param  c: The char to be sent
  * @retval None
  */
void USB_PutChar(uint8_t c)
{
    static uint8_t temp_buf;

    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return;

    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
    while (hcdc->TxState != 0) {}

    temp_buf = c;
    CDC_Transmit_FS(&temp_buf, 1);
}

void USB_PutString(char *s)
{
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
    {
        return;
    }

    uint32_t len = strlen(s);
    while (CDC_Transmit_FS((uint8_t*)s, len) == USBD_BUSY)
    {
        if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
        {
            return;
        }
    }
}

/**
  * @brief  Get Input string from the HyperTerminal
  * @param  buffP: The input string
  * @retval None
  */
void GetInputString (uint8_t * buffP)
{
  uint32_t bytes_read = 0;
  uint8_t c = 0;
  do
  {
    c = GetKey();
    if (c == '\r')
      break;
    if (c == '\b') /* Backspace */
    {
      if (bytes_read > 0)
      {
        USB_PutString("\b \b");
        bytes_read --;
      }
      continue;
    }
    if (bytes_read >= CMD_STRING_SIZE )
    {
      USB_PutString("Command string size overflow\r\n");
      bytes_read = 0;
      continue;
    }
    if (c >= 0x20 && c <= 0x7E)
    {
      buffP[bytes_read++] = c;
      USB_PutChar(c);
    }
  }
  while (1);
  USB_PutString(("\n\r"));
  buffP[bytes_read] = '\0';
}

/**
  * @brief  Calculates the CRC32 of a given memory area.
  * @param  start_address: Start address of the data
  * @param  size: Size of the data in bytes
  * @retval Calculated CRC32 value
  */
uint32_t Calculate_CRC32(uint32_t start_address, uint32_t size)
{
    // ??CRC????
    __HAL_RCC_CRC_CLK_ENABLE();
    CRC_HandleTypeDef CrcHandle;
    CrcHandle.Instance = CRC;
    HAL_CRC_Init(&CrcHandle);

    return HAL_CRC_Calculate(&CrcHandle, (uint32_t*)start_address, size / 4);
}


/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE******/
