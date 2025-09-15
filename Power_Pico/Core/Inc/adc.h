/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.h
  * @brief   This file contains all the function prototypes for
  *          the adc.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#define ADC_TIMES 1
#define ADC_CHANELS 4

typedef struct {
  uint8_t  header[4]; //2的倍数
  // 0x55, 0xAA header
  // 0x0A adc times, 0x05 adc channels (so data lenth is times * channels * 2 bytes)
  // 0x?0 for LOW, 0x?1 for MID, 0x?2 for HIGH.
  uint16_t data[ADC_TIMES][ADC_CHANELS];
} __attribute__((packed)) ADC_Packet;

extern ADC_Packet adc_packet;

#define UNIT_UA 0
#define UNIT_MA 1

// 量程阈值
#define ADC_TRUST_MAX 4000
#define THRESH_HIGH 1900
#define THRESH_LOW  18

// 次数
#define THRESH_TIMES 10

extern uint8_t global_cur_unit;
extern float global_valtage;
extern float global_current;

/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_ADC1_Init(void);

/* USER CODE BEGIN Prototypes */

void get_global_val_cur(float *valtage, float *current, uint8_t *cur_unit);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */

