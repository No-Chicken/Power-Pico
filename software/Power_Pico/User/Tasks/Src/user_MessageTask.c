/* Private includes -----------------------------------------------------------*/
//includes
#include "adc.h"
#include "user_AdcDataStrategy.h"
#include "user_CmdStrategy.h"
#include "user_TasksInit.h"
#include "user_MessageTask.h"
#include "usb_device.h"
#include "BL24C02.h"
#include "usbd_cdc_if.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* UI 刷新周期（按时间推导分频，避免依赖固定采样块大小） */
#define UI_UPDATE_PERIOD_MS 25U
#define UI_UPDATE_DIV ((UI_UPDATE_PERIOD_MS + ADC_CHUNK_PERIOD_MS - 1U) / ADC_CHUNK_PERIOD_MS)

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  task for receiving messages, such as USB update requests.
  * @param  argument: Not used
  * @retval None
  */
void MessageReceiveTask(void *argument)
{
    while (!user_hardware_ready) osDelay(1);
	uint32_t flags;
    CmdRxChunk_t chunk;
	(void)argument;
	CmdStrategy_Init();
	while(1)
	{
    flags = osThreadFlagsWait(FLAG_USB_UPDATE_REQ | FLAG_CMD_RX_READY,
                              osFlagsWaitAny,
                              5U);

    if ((flags & 0x80000000U) == 0U && (flags & FLAG_USB_UPDATE_REQ) != 0U) {
      EEPROM_UpdateCommand_Write(true);
      HAL_Delay(100);
      USER_USB_DEVICE_DeInit();
      // 给予PC足够的时间来识别设备断开
      HAL_Delay(500);
      NVIC_SystemReset();
    }
    if (CmdRxQueue != NULL) {
      /* Bound each batch so continuous host traffic cannot starve LVGL. */
      uint8_t drained = 0U;
      while (drained < CMD_RX_QUEUE_DEPTH &&
             osMessageQueueGet(CmdRxQueue, &chunk, NULL, 0U) == osOK) {
        CDC_ResumeReceive_FS();
        CmdStrategy_ProcessRx(chunk.data, chunk.length);
        drained++;
      }
    }
    CDC_ResumeReceive_FS();
    osDelay(5);
	}
}

/**
 * @brief  send adc data to PC via USB,
 * @param  argument: Not used
 */
void MessageSendTask(void *argument)
{
  while (!user_hardware_ready) osDelay(1);
  uint32_t flags;
  uint32_t ui_div_cnt = 0;
  CmdTxFrame_t response;
  USB_ADC_Packet_t pending_adc;
  bool pending_adc_valid = false;
  bool pending_cmd_valid = false;
  while (1)
  {
    // Wake for ADC chunks or a queued command response.
    flags = osThreadFlagsWait(FLAG_ADC_HALF_READY | FLAG_ADC_FULL_READY |
                              FLAG_CMD_TX_READY,
                              osFlagsWaitAny,
                              osWaitForever);

    if (flags & 0x80000000) {
        continue;
    }

    if (flags & FLAG_ADC_HALF_READY)
    {
      USB_ADC_Packet_t *packet = Process_ADC_Chunk(&adc_raw_buffer[0][0], 0);
      if (packet != NULL) {
        memcpy(&pending_adc, packet, sizeof(pending_adc));
        pending_adc_valid = true;
      }
    }

    if (flags & FLAG_ADC_FULL_READY)
    {
      USB_ADC_Packet_t *packet = Process_ADC_Chunk(&adc_raw_buffer[ADC_TIMES][0], 1);
      if (packet != NULL) {
        memcpy(&pending_adc, packet, sizeof(pending_adc));
        pending_adc_valid = true;
      }
    }

    /* Keep at most one pending frame of each kind.  Command replies have
     * priority; a measurement packet may be replaced by a newer one. */
    if (!pending_cmd_valid && CmdTxQueue != NULL &&
        osMessageQueueGet(CmdTxQueue, &response, NULL, 0U) == osOK) {
        pending_cmd_valid = true;
    }
    if (pending_cmd_valid) {
      if (CDC_Transmit_FS(response.data, response.length) == USBD_OK) {
        pending_cmd_valid = false;
      }
    } else if (pending_adc_valid) {
      if (CDC_Transmit_FS((uint8_t *)&pending_adc, sizeof(pending_adc)) == USBD_OK) {
        pending_adc_valid = false;
      }
    }

    // Publish UI data after UI_UPDATE_DIV task wakeups.
    if (++ui_div_cnt >= UI_UPDATE_DIV)
    {
        ui_div_cnt = 0;

        PowerData_t newData;
        Data_Monitor_Get_Values(&newData.voltage, &newData.current);

        osStatus_t status = osMessageQueuePut(PowerDataQueue, &newData, 0, 0);
        if (status == osErrorResource) {
            PowerData_t dummy;
            // 队列满：丢弃旧数据，写入新数据
            osMessageQueueGet(PowerDataQueue, &dummy, NULL, 0);
            osMessageQueuePut(PowerDataQueue, &newData, 0, 0);
        }
    }
  }
}
