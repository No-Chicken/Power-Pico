/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "menu.h"
#include "common.h"
#include "delay.h"
#include "lcd.h"
#include "lcd_init.h"
#include "key.h"
#include "BL24C02.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

extern pFunction Jump_To_Application;
extern uint32_t JumpAddress;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Bootloader_Main(void)
{
  // Bootloader 循环等待上位机指令
  while (1)
  {
      // 1. 发送握手信号
      USB_PutString("##PICO_BOOT##\r\n");
      // 2. 在延时期间(比如500ms)不断检查有没有收到数据
      // 这样既实现了延时，又不会阻塞接收
      uint8_t key = 0;
      for (int i = 0; i < 50; i++)
      {
          // USB_KeyPressed 应该是一个非阻塞检查函数
          // 如果收到数据返回 1，否则返回 0
          if (USB_KeyPressed(&key))
          {
              if (key == '0')
              {
                  // 收到 '0'，进入菜单
                  Main_Menu();
              }
          }
          HAL_Delay(10); // 每次只延时 10ms，检查 50 次 = 500ms
      }
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	SCB->VTOR = FLASH_BASE;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_USB_DEVICE_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

	//sys delay
	delay_init();

  //key_boot
	Key_Port_Init();

	// PWM Start for LCD backlight
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

	//lcd
	LCD_Init();
	LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
	delay_ms(10);
	LCD_Set_Light(50);

	// extern eeprom BL24C02
	uint8_t update_flag = 0;
	if(!EEPROM_Init_Check()) {
		update_flag = EEPROM_UpdateCommand_Check();
		if(EEPROM_UpdateCommand_Check()) {
			USB_PutString("enter firmware update mode\r\n");
			EEPROM_UpdateCommand_Write(false);
		}
	}

  // 开机启动时如果按下KEY, 或EEPROM有升级标志, 进入boot中IAP升级模式
  if(HAL_GPIO_ReadPin(KEYB_PORT, KEYB_PIN) == 0 || update_flag)
  {
		// 延时判断是否真的按下
    delay_ms(500);
    if(HAL_GPIO_ReadPin(KEYB_PORT, KEYB_PIN) == 0 || update_flag)
    {
      LCD_ShowString(72, LCD_H/3, (uint8_t*)"Bootload", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
      FLASH_If_Init();
      Bootloader_Main();
    }
  }

  // 如果没按下KEY1, 且有APP程序, 则运行APP, 没有APP则
  else
  {
    UpgradeInfo_t app_info;

    // 1. 从独立的校验区(Sector 3)读取升级信息结构体
    memcpy(&app_info, (void*)UPGRADE_INFO_ADDRESS, sizeof(UpgradeInfo_t));

    // 2. 检查标志是否有效 (魔法字和完成状态)
    if (app_info.magic_word == MAGIC_WORD && app_info.upgrade_status == UPGRADE_STATUS_COMPLETE)
    {
        // 3. 校验APP固件的完整性
        uint32_t current_crc = Calculate_CRC32(APPLICATION_ADDRESS, app_info.new_app_size);

        if (current_crc == app_info.new_app_crc32)
        {
            // 4. 校验成功，执行跳转
            USB_PutString("Valid APP found, jumping to application...\r\n");
            jump_to_app();
        }
    }
		// no legal APP
		else
		{
      USB_PutString("No App found, enter firmware update mode\r\n");
			LCD_ShowString(74, LCD_H/3, (uint8_t*)"No App!", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
      LCD_ShowString(24, LCD_H/3+48, (uint8_t*)"Plz Download APP", WHITE, BLACK, 24, 0);
			HAL_Delay(2000);
			LCD_ShowString(72, LCD_H/3, (uint8_t*)"Bootload", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
      FLASH_If_Init();
      Bootloader_Main();
		}
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		HAL_Delay(500);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: Serial_PutString("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
