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

#define ADC_TIMES 100 // 每次采样 100 次 (10ms)
#define ADC_CHANNELS 5 // 5 通道：电压 + 3 档电流 + REF
#define PACKET_HEADER_0 0xAA
#define PACKET_HEADER_1 0x55

// Rank1=PA5, Rank2=PA6, Rank3=PA7, Rank4=PB0, Rank5=PB1
#define IDX_VOL  0
#define IDX_LOW  1
#define IDX_MID  2
#define IDX_HIGH 3
#define IDX_REF  4

// Resistance values in ohms
#define HIGH_CUR_RES 0.005f   // 5 m ohm
#define MID_CUR_RES 0.5f      // 500 m ohm
#define LOW_CUR_RES 50.0f     // 50 ohm

// 预先计算标度因子
#define SCALE_LOW  (3.0f / 4096.0f / 50.0f / LOW_CUR_RES *  1000000.0) // uA
#define SCALE_MID  (3.0f / 4096.0f / 50.0f / MID_CUR_RES *  1000000.0) // uA
#define SCALE_HIGH (3.0f / 4096.0f / 50.0f / HIGH_CUR_RES * 1000000.0) // uA

// 量程阈值
#define THRESH_HIGH 4000-2048 // 对应550uA 50mA
#define THRESH_LOW  15 // 对应大概450uA 45mA
// 次数
#define THRESH_TIMES 8

// 监视器更新频率和对应的采样点数量
#define MONITOR_UPDATE_PERIOD_MS 250
#define MONITOR_UPDATE_CHUNK_COUNT (MONITOR_UPDATE_PERIOD_MS / 10)

/* --- USB 数据包结构 (总长 711 字节) --- */
#pragma pack(push, 1) // 强制 1 字节对齐
typedef struct {
    uint8_t  header[2];      // 0x55, 0xAA
    uint64_t timestamp;      // 64位微秒级时间戳
    uint8_t  data_count;     // 固定 100

    struct {
        uint8_t  range;      // 量程: 0=Low, 1=Mid, 2=High
        uint16_t vol_adc;    // 电压
        uint16_t cur_adc;    // 电流 (选择后)
        uint16_t ref_adc;    // 参考电压
    } samples[ADC_TIMES];

} USB_ADC_Packet_t;
#pragma pack(pop)

/* --- 统计监视器结构体 --- */
typedef struct {
    /* * 累加和使用 64 位整数，防止溢出并保持精度
     * sum_cur_resolution_10na: 用户要求的 uA*100，即分辨率 0.01uA (10nA)
     * sum_vol_mv: 电压为了保持精度累加 mV，Get时再转 V
     */
    volatile int64_t sum_cur_resolution_10na;
    volatile int64_t sum_vol_mv;
    volatile uint32_t count;
    volatile float avg_vol_v;
    volatile float avg_cur_ua;
} Data_Monitor_t;

// 原始 DMA 缓冲区 [200][5]
extern uint16_t adc_raw_buffer[ADC_TIMES * 2][ADC_CHANNELS];

/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_ADC1_Init(void);

/* USER CODE BEGIN Prototypes */

void Data_Monitor_Get_Values(float *out_vol_v, float *out_cur_ua);
void Data_Monitor_Clear(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */

