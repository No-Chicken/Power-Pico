#include "gate.h"
#include "math.h"

uint8_t gate_status = HIGH_CUR; // Default status

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

