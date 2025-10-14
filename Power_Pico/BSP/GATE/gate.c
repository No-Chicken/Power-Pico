#include "gate.h"
#include "data_queue.h"
#include "usart.h"
#include "math.h"

// 量程阈值
#define ADC_TRUST_MAX 4000
#define ADC_TRUST_MIN 10
#define THRESH_HIGH 1900
#define THRESH_LOW  18
// 次数
#define THRESH_TIMES 8

uint8_t gate_status = HIGH_CUR; // Default status

// 预先计算标度因子
static double SCALE_LOW  = 3.0f / 4096.0f / 50.0f / LOW_CUR_RES *  1000000.0; // uA
static double SCALE_MID  = 3.0f / 4096.0f / 50.0f / MID_CUR_RES *  1000000.0; // uA
static double SCALE_HIGH  = 3.0f / 4096.0f / 50.0f / HIGH_CUR_RES * 1000000.0; // uA

void Gate_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(EN1_PORT, EN1_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(EN2_PORT, EN2_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = EN1_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EN1_PORT, &GPIO_InitStruct);

	/*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = EN2_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EN2_PORT, &GPIO_InitStruct);
}

void flow_route_selection(uint8_t selection)
{
  if(selection > HIGH_CUR)
    selection = HIGH_CUR; // Invalid selection
  if(selection < LOW_CUR)
    selection = LOW_CUR;
	if(selection == HIGH_CUR)
  {
    HAL_GPIO_WritePin(EN1_PORT, EN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(EN2_PORT, EN2_PIN, GPIO_PIN_RESET);
  }
  else if(selection == MID_CUR)
  {
    HAL_GPIO_WritePin(EN1_PORT, EN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EN2_PORT, EN2_PIN, GPIO_PIN_SET);
  }
  else if(selection == LOW_CUR)
  {
    HAL_GPIO_WritePin(EN1_PORT, EN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EN2_PORT, EN2_PIN, GPIO_PIN_RESET);
  }
  gate_status = selection; // Update the status
}

uint8_t Gate_get_status(void)
{
  return gate_status;
}

// 在DMA中断中执行, 不能有阻塞
void Gate_Swich_and_UART_Send(ADC_Packet adc_packet)
{
  static uint8_t high_times = 0;
  static uint8_t low_times = 0;
  // low mid high
  uint16_t cur_adc = 0;
  switch(Gate_get_status()) {
    case LOW_CUR:
      cur_adc = adc_packet.data[0][1];
      break;
    case MID_CUR:
      cur_adc = adc_packet.data[0][2];
      break;
    case HIGH_CUR:
      cur_adc = adc_packet.data[0][3];
      break;
  }
  // 可信范围内
  float voltage;
  float current;
  if(cur_adc < ADC_TRUST_MAX && cur_adc > ADC_TRUST_MIN) {
    adc_packet.header[3] = Gate_get_status();
    ADC_Packet adc_packet_trans = adc_packet;
    HAL_UART_Transmit_DMA(&huart6, (uint8_t*)&adc_packet_trans, sizeof(adc_packet_trans));
    // 计算电压电流值
    voltage = adc_packet.data[0][0] * 3.0f / 4096.0f * 11.0f; // 11 = (100+10)/10
    // 计算电流值
    if(Gate_get_status() == LOW_CUR) {
      current = adc_packet.data[0][1];
      current = (current - 2048.0) * SCALE_LOW;
    }
    else if(Gate_get_status() == MID_CUR) {
      current = adc_packet.data[0][2];
      current = (current - 2048.0) * SCALE_MID;
    }
    else if(Gate_get_status() == HIGH_CUR) {
      current = adc_packet.data[0][3];
      current = (current - 2048.0) * SCALE_HIGH;
    }
    // 入队列
    queue_push(global_voltage_queue, voltage);
    queue_push(global_current_queue, current);
    // 判断是否需要切换档位
    // ADC码值超过量程
    uint16_t diff = abs((int)cur_adc - 2048);
    if(diff > THRESH_HIGH) {
      high_times++;
      low_times = 0;
      if(high_times >= THRESH_TIMES) {
        high_times = 0;
        flow_route_selection(Gate_get_status()+1);
      }
    }
    else if(diff < THRESH_LOW) {
      low_times++;
      high_times = 0;
      if(low_times >= THRESH_TIMES) {
        low_times = 0;
        flow_route_selection(Gate_get_status()-1);
      }
    }
    else {
      high_times = 0;
      low_times = 0;
    }
  }
  // 不可信范围内
  else {
    if(cur_adc >= ADC_TRUST_MAX) // 过大，切换到更大档位
      flow_route_selection(Gate_get_status()+1);
    else if(cur_adc <= ADC_TRUST_MIN) // 过小，切换到更小档位
      flow_route_selection(Gate_get_status()-1);
  }
}
