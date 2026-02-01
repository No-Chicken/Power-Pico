/**
  ******************************************************************************
  * @file    STM32F4xx_IAP/src/menu.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file,
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded
  *             - disabling the write protection of the Flash sectors where the
  *               user loads his binary file.
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
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"
#include "string.h"
#include "usb_device.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t JumpAddress;
__IO uint32_t FlashProtection = 0;
uint8_t tab_1024[1024] =
  {
    0
  };
uint8_t FileName[FILE_NAME_LENGTH];

/* Private function prototypes -----------------------------------------------*/
void USB_Download(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Writes the UpgradeInfo structure to the dedicated Flash sector.
  * @param  info: pointer to the UpgradeInfo_t structure
  * @retval None
  */
static void Write_Upgrade_Info(UpgradeInfo_t* info)
{
    // 1. 擦除专门用于校验的 Sector 3
    FLASH_If_Erase_One_Sector(UPGRADE_INFO_SECTOR_NUM);
    // 2. 写入升级信息
    uint32_t flash_address = UPGRADE_INFO_ADDRESS;
    FLASH_If_Write((__IO uint32_t*)&flash_address, (uint32_t*)info, sizeof(UpgradeInfo_t) / 4);
}

/**
  * @brief  Reads the UpgradeInfo structure from the dedicated Flash sector.
  * @param  info: pointer to the UpgradeInfo_t structure to be filled
  * @retval None
  */
static void Read_Upgrade_Info(UpgradeInfo_t* info)
{
    memcpy(info, (void*)UPGRADE_INFO_ADDRESS, sizeof(UpgradeInfo_t));
}

// Int-to-Hex函数
static void Int2Hex(uint8_t* str, uint32_t intnum)
{
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++)
    {
        str[7 - i] = hex_chars[intnum & 0x0F];
        intnum >>= 4;
    }
    str[8] = '\0';
}

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
void USB_Download(void)
{
  UpgradeInfo_t received_info;
  uint8_t Number[10] = "          ";
  int32_t file_size = 0;

  // user operation
  // 1. 接收元数据
  USB_PutString("Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  // 2. 擦除整个APP区域
  USB_PutString("Erasing Application Area...\r\n");
  if (FLASH_If_Erase(APPLICATION_ADDRESS) != 0)
  {
      USB_PutString("Error: Failed to erase application area!\r\n");
      return;
  }
  USB_PutString("Erase Complete.\r\n");
  /* Waiting for file sent */
  USB_PutString("Starting Ymodem Receive\n\r");
  file_size = Ymodem_Receive(&tab_1024[0]);
  if (file_size > 0)
  {

    USB_PutString("\n\n\r File reception complete. Verifying...\n\r");
    // 4. 最终校验
    // 理想情况下，这里应该有一个从上位机获取的CRC32值进行对比, 此处不对比
    uint32_t calculated_crc = Calculate_CRC32(APPLICATION_ADDRESS, file_size);

    // 5. 写入校验信息到Flash末尾
    received_info.magic_word = MAGIC_WORD;
    received_info.upgrade_status = UPGRADE_STATUS_COMPLETE; // 标记为升级完成
    received_info.new_app_size = file_size;
    received_info.new_app_crc32 = calculated_crc;
    Write_Upgrade_Info(&received_info);
    USB_PutString("Verification and Flag Set successful!\r\n");
    USB_PutString("--------------------------------\r\n Name: ");
    USB_PutString(FileName);
    Int2Str(Number, file_size);
    USB_PutString("\n\r Size: ");
    USB_PutString(Number);
    USB_PutString(" Bytes\r\n");
    USB_PutString(" CRC32: 0x");
    uint8_t calculated_crc_str[9];
    Int2Hex(calculated_crc_str, calculated_crc);
    USB_PutString(calculated_crc_str);
    USB_PutString("\r\n-------------------\n");
    USB_PutString("Please reboot the device.\r\n");
  }
  else /* An error occurred while writing to Flash memory */
  {
    FLASH_If_Erase_One_Sector(UPGRADE_INFO_SECTOR_NUM); // 擦除升级校验区，避免误跳转
    USB_PutString("\n\rUpgrade failed!\r\n");
    if (file_size == -1)
    {
      USB_PutString("\n\n\rThe image size is higher than the allowed space memory!\n\r");
    }
    else if (file_size == -2)
    {
      USB_PutString("\n\n\rVerification failed!\n\r");
    }
    else if (file_size == -3)
    {
      USB_PutString("\r\n\nAborted by user.\n\r");
    }
    else
    {
      USB_PutString("\n\rFailed to receive the file!\n\r");
    }

  }
}

/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
  uint8_t key = 0;

  USB_PutString("\r\n======================================================================");
  USB_PutString("\r\n=              (C) COPYRIGHT 2011 STMicroelectronics                 =");
  USB_PutString("\r\n=                                                                    =");
  USB_PutString("\r\n=  STM32F4xx In-Application Programming Application  (Version 1.0.0) =");
  USB_PutString("\r\n=                                                                    =");
  USB_PutString("\r\n=                                   By MCD Application Team          =");
  USB_PutString("\r\n======================================================================");
  USB_PutString("\r\n\r\n");

  /* Test if any sector of Flash memory where user application will be loaded is write protected */
  if (FLASH_If_GetWriteProtectionStatus() == 0)
  {
    FlashProtection = 1;
  }
  else
  {
    FlashProtection = 0;
  }

  while (1)
  {
    USB_PutString("\r\n================== Main Menu ============================\r\n\n");
    USB_PutString("  Download Image To the STM32F4xx Internal Flash ------- 1\r\n\n");
    USB_PutString("  Upload Image From the STM32F4xx Internal Flash ------- 2\r\n\n");
    USB_PutString("  Execute The New Program ------------------------------ 3\r\n\n");

    if(FlashProtection != 0)
    {
      USB_PutString("  Disable the write protection ------------------------- 4\r\n\n");
    }

    USB_PutString("==========================================================\r\n\n");

    /* Receive key */
    key = GetKey();

    if (key == 0x31)
    {
      /* Download user application in the Flash */
      USB_Download();
    }
    else if (key == 0x32)
    {
      /* Upload user application from the Flash */
      // USB_Upload();
      USB_PutString("This function is disabled! Please Use 1\r");
    }
    else if (key == 0x33) /* execute the new program */
    {
      USB_PutString("Start to execute the user application...\r\n");
      HAL_Delay(1000);
      USER_USB_DEVICE_DeInit();
      HAL_RCC_DeInit();
      HAL_DeInit();
      //user code here
      SysTick->CTRL = 0X00;//禁止SysTick
      SysTick->LOAD = 0;
      SysTick->VAL = 0;
      __disable_irq();

      //set JumpAddress
      JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
      /* Jump to user application */
      Jump_To_Application = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
      Jump_To_Application();
    }
    else if ((key == 0x34) && (FlashProtection == 1))
    {
      /* Disable the write protection */
      switch (FLASH_If_DisableWriteProtection())
      {
        case 1:
        {
          USB_PutString("Write Protection disabled...\r\n");
          FlashProtection = 0;
          break;
        }
        case 2:
        {
          USB_PutString("Error: Flash write unprotection failed...\r\n");
          break;
        }
        default:
        {
        }
      }
    }
    else
    {
      if (FlashProtection == 0)
      {
        USB_PutString("Invalid Number ! ==> The number should be either 1, 2 or 3\r");
      }
      else
      {
        USB_PutString("Invalid Number ! ==> The number should be either 1, 2, 3 or 4\r");
      }
    }
  }
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE******/
