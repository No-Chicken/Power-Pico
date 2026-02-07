/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* USER CODE BEGIN 0 */

#include "tim.h"
#include "usb_device.h"
#include "gate.h"

// DMA 双缓冲原始数据 [200行][5列]
uint16_t adc_raw_buffer[ADC_TIMES * 2][ADC_CHANNELS];
// USB 发送缓冲区 (Ping-Pong 双缓冲防止发送冲突)
USB_ADC_Packet_t usb_packet_buffer[2];
// 统计实例
Data_Monitor_t g_data_monitor = {0};
// 物理切换防抖计数器
static uint8_t high_overload_cnt = 0;
static uint8_t low_underload_cnt = 0;
static uint8_t is_transition_next = 0;  // 标记下一包是否为过渡数据
// 用于监视数据更新的计数器
static uint32_t monitor_chunk_counter = 0;

/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* ADC1 init function */
void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 5;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */
  __HAL_DMA_ENABLE_IT(&hdma_adc1, DMA_IT_TC); /*开启DMA传输完成中断*/
  /* USER CODE END ADC1_Init 2 */

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA5     ------> ADC1_IN5
    PA6     ------> ADC1_IN6
    PA7     ------> ADC1_IN7
    PB0     ------> ADC1_IN8
    PB1     ------> ADC1_IN9
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc1);

  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA5     ------> ADC1_IN5
    PA6     ------> ADC1_IN6
    PA7     ------> ADC1_IN7
    PB0     ------> ADC1_IN8
    PB1     ------> ADC1_IN9
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_1);

    /* ADC1 DMA DeInit */
    HAL_DMA_DeInit(adcHandle->DMA_Handle);
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

// 内部调用的数据更新函数，mV 和 10nA 累加
static void Data_Monitor_Update(uint16_t vol_adc, uint16_t cur_adc, uint16_t ref_adc, uint8_t range)
{
    // 1. 计算电压 (mV)
    // V = (ADC_Value / 4095) * 3.0V * (10k + 1k) / 1k
    // mV = ADC_Value * (3000.0 / 4095 * 11)
    float voltage_mv = (float)vol_adc * (3000.0f / 4095.0f * 11.0f);

    // 2. 计算电流 (单位: 10nA)
    // I_uA = ADC_Value * SCALE_XXX
    // I_10nA = I_uA * 100
    uint64_t current_10na = 0;
    switch (range) {
        case LOW_CUR:
            current_10na = (uint64_t)((float)(cur_adc-ref_adc) * SCALE_LOW * 100.0f);
            break;
        case MID_CUR:
            current_10na = (uint64_t)((float)(cur_adc-ref_adc) * SCALE_MID * 100.0f);
            break;
        case HIGH_CUR:
            current_10na = (uint64_t)((float)(cur_adc-ref_adc) * SCALE_HIGH * 100.0f);
            break;
    }

    // 3. 累加数据 (此函数在中断上下文中被调用，Data_Monitor_Get_Values会处理中断保护)
    g_data_monitor.sum_vol_mv += (uint64_t)voltage_mv;
    g_data_monitor.sum_cur_resolution_10na += current_10na;
    g_data_monitor.count++;
}

// 内部调用的计算函数，计算平均值并重置累加器
static void Data_Monitor_Calculate_Average(void)
{
    // 关中断保护，确保在计算和重置期间 g_data_monitor 不被修改
    __disable_irq();

    if (g_data_monitor.count > 0)
    {
        // 计算平均值并存储到结构体的新成员中
        g_data_monitor.avg_vol_v = (float)g_data_monitor.sum_vol_mv / g_data_monitor.count / 1000.0f;
        g_data_monitor.avg_cur_ua = (float)g_data_monitor.sum_cur_resolution_10na / g_data_monitor.count / 100.0f;
    }
    else
    {
        // 如果在此期间没有有效数据（例如，一直处于档位切换中）
        g_data_monitor.avg_vol_v = 0.0f;
        g_data_monitor.avg_cur_ua = 0.0f;
    }

    // 重置累加器，为下一个计算周期做准备
    g_data_monitor.sum_vol_mv = 0;
    g_data_monitor.sum_cur_resolution_10na = 0;
    g_data_monitor.count = 0;

    __enable_irq();
}

// 外部调用的获取函数，单位 V 和 uA
void Data_Monitor_Get_Values(float *out_vol_v, float *out_cur_ua)
{
    // 直接返回已经计算好的平均值，无需关中断，因为读取 float 是原子操作
    *out_vol_v = g_data_monitor.avg_vol_v;
    *out_cur_ua = g_data_monitor.avg_cur_ua;
}

// 外部调用的清除函数，重置所有统计数据
void Data_Monitor_Clear(void)
{
    __disable_irq();
    g_data_monitor.sum_vol_mv = 0;
    g_data_monitor.sum_cur_resolution_10na = 0;
    g_data_monitor.count = 0;
    g_data_monitor.avg_vol_v = 0.0f;
    g_data_monitor.avg_cur_ua = 0.0f;
    __enable_irq();
}

// 处理 ADC 数据块的核心函数，负责数据选择、切换决策和USB发送
static void Process_ADC_Chunk(uint16_t *chunk_ptr, uint8_t packet_idx)
{
    USB_ADC_Packet_t *pkg = &usb_packet_buffer[packet_idx];

    // 填充包头
    pkg->header[0] = PACKET_HEADER_0;
    pkg->header[1] = PACKET_HEADER_1;
    pkg->timestamp = GetMicrosecondCounter();
    pkg->data_count = ADC_TIMES;

    // 获取当前物理档位
    uint8_t current_hw_range = Gate_get_status();
    uint8_t req_switch_range = current_hw_range; // 初始化请求切换的档位

    // 如果上一包触发了切换，本包数据是在切换期间采集的“脏数据”，必须丢弃
    if (is_transition_next) {
        is_transition_next = 0; // 清除标志
        high_overload_cnt = 0;  // 清除计数器，为下一次干净数据做准备
        low_underload_cnt = 0;
        // 直接返回，不处理也不发送任何数据
        return;
    }

    int i = 0;
    // 遍历采样点
    for (i = 0; i < ADC_TIMES; i++)
    {
        uint16_t *sample_row = chunk_ptr + (i * ADC_CHANNELS);
        uint16_t raw_vol = sample_row[IDX_VOL];
        uint16_t raw_low = sample_row[IDX_LOW];
        uint16_t raw_mid = sample_row[IDX_MID];
        uint16_t raw_hig = sample_row[IDX_HIGH];
        uint16_t raw_ref = sample_row[IDX_REF];

        // --- 基于当前物理档位，进行独立的切换决策 ---
        switch (current_hw_range)
        {
            case LOW_CUR:
                // 在LOW档，只关心raw_low是否过载
                if (abs(raw_low-raw_ref) >= THRESH_HIGH) {
                    high_overload_cnt++;
                    if (high_overload_cnt >= THRESH_TIMES) {
                        req_switch_range = MID_CUR; // 请求升到MID档
                    }
                } else {
                    high_overload_cnt = 0;
                }
                // LOW档不存在欠载问题
                low_underload_cnt = 0;
                break;

            case MID_CUR:
                // 在MID档，判断raw_mid是否过载或欠载
                if (abs(raw_mid-raw_ref) >= THRESH_HIGH) { // 过载
                    high_overload_cnt++;
                    if (high_overload_cnt >= THRESH_TIMES) {
                        req_switch_range = HIGH_CUR; // 请求升到HIGH档
                    }
                } else {
                    high_overload_cnt = 0;
                }

                if (abs(raw_mid-raw_ref) < THRESH_LOW) { // 欠载
                    low_underload_cnt++;
                    if (low_underload_cnt >= THRESH_TIMES) {
                        req_switch_range = LOW_CUR; // 请求降到LOW档
                    }
                } else {
                    low_underload_cnt = 0;
                }
                break;

            case HIGH_CUR:
                // 在HIGH档，只关心raw_hig是否欠载
                if (abs(raw_hig-raw_ref) < THRESH_LOW) { // 注意：这里用raw_hig判断
                    low_underload_cnt++;
                    if (low_underload_cnt >= THRESH_TIMES) {
                        req_switch_range = MID_CUR; // 请求降到MID档
                    }
                } else {
                    low_underload_cnt = 0;
                }
                // HIGH档不存在过载问题
                high_overload_cnt = 0;
                break;
        }

        // 如果已经做出切换决定，立即跳出循环
        if (req_switch_range != current_hw_range) {
            break;
        }

        // --- 数据选择与填充 ---
        // 根据采集本数据块时的硬件档位(current_hw_range)来选择有效的电流ADC值
        uint16_t final_cur = 0;
        switch (current_hw_range) {
            case LOW_CUR:  final_cur = raw_low; break;
            case MID_CUR:  final_cur = raw_mid; break;
            case HIGH_CUR: final_cur = raw_hig; break;
        }

        // 填充USB数据包的当前采样点
        pkg->samples[i].range   = current_hw_range;
        pkg->samples[i].vol_adc = raw_vol;
        pkg->samples[i].cur_adc = final_cur;
        pkg->samples[i].ref_adc = raw_ref;

        // 更新用于屏幕显示的统计数据
        Data_Monitor_Update(raw_vol, final_cur, raw_ref, current_hw_range);
    }

    // 更新包中实际有效的数据点数量
    pkg->data_count = i;

    // --- 循环结束后，执行物理切换 ---
    if (req_switch_range != current_hw_range)
    {
        flow_route_selection(req_switch_range); // 执行物理切换
        is_transition_next = 1; // 标记下一包是过渡数据
        high_overload_cnt = 0;  // 切换后清零计数器
        low_underload_cnt = 0;
    }

    // USB 发送 (非阻塞)
    CDC_Transmit_FS((uint8_t*)pkg, sizeof(USB_ADC_Packet_t));

    //
    monitor_chunk_counter++;
    if (monitor_chunk_counter >= MONITOR_UPDATE_CHUNK_COUNT)
    {
        monitor_chunk_counter = 0; // 重置计数器
        Data_Monitor_Calculate_Average(); // 执行计算
    }
}

/* --- DMA 回调函数 --- */

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    // 处理前半段 (Buffer 行 0 ~ 99)
    // 传入 &adc_raw_buffer[0][0]
    Process_ADC_Chunk(&adc_raw_buffer[0][0], 0);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    // 处理后半段 (Buffer 行 100 ~ 199)
    // 传入 &adc_raw_buffer[ADC_TIMES][0]
    Process_ADC_Chunk(&adc_raw_buffer[ADC_TIMES][0], 1);
}

/* USER CODE END 1 */
