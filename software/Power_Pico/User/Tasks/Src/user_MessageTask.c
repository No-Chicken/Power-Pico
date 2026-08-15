#include "adc.h"
#include "usbd_cdc_if.h"
#include "user_AdcDataStrategy.h"
#include "user_CmdStrategy.h"
#include "user_MessageTask.h"
#include "user_TasksInit.h"

#include <stdbool.h>
#include <string.h>

#define UI_UPDATE_PERIOD_MS 25U
#define UI_UPDATE_DIV ((UI_UPDATE_PERIOD_MS + ADC_CHUNK_PERIOD_MS - 1U) / ADC_CHUNK_PERIOD_MS)

void MessageReceiveTask(void *argument)
{
    CmdRxChunk_t chunk;

    (void)argument;
    CmdStrategy_Init();

    while (1) {
        if (CmdRxQueue == NULL || osMessageQueueGet(CmdRxQueue, &chunk, NULL, osWaitForever) != osOK) {
            osDelay(100U);
            continue;
        }
        CmdStrategy_ProcessRx(chunk.data, chunk.length);
    }
}

void MessageSendTask(void *argument)
{
    uint32_t flags;
    uint32_t ui_div_count = 0U;
    CmdTxFrame_t pending_command;
    uint8_t pending_adc[sizeof(USB_ADC_Packet_t)];
    uint8_t active_tx[sizeof(USB_ADC_Packet_t)];
    uint16_t pending_adc_length = 0U;
    bool command_pending = false;
    bool adc_pending = false;
    bool tx_active = false;

    (void)argument;
    while (1) {
        const uint8_t *next_data = NULL;
        uint16_t next_length = 0U;
        bool next_is_command = false;
        USB_ADC_Packet_t *packet;
        uint32_t wait_time = (!tx_active && (command_pending || adc_pending))
            ? 1U
            : osWaitForever;

        flags = osThreadFlagsWait(FLAG_ADC_HALF_READY
                                      | FLAG_ADC_FULL_READY
                                      | FLAG_CMD_TX_READY
                                      | FLAG_USB_TX_COMPLETE,
                                  osFlagsWaitAny,
                                  wait_time);
        if ((flags & 0x80000000U) != 0U) {
            flags = 0U;
        }
        if ((flags & FLAG_USB_TX_COMPLETE) != 0U) {
            tx_active = false;
        }

        if ((flags & FLAG_ADC_HALF_READY) != 0U) {
            packet = Process_ADC_Chunk(&adc_raw_buffer[0][0], 0U);
            if (packet != NULL) {
                memcpy(pending_adc, packet, sizeof(*packet));
                pending_adc_length = sizeof(*packet);
                adc_pending = true;
            }
            if (++ui_div_count >= UI_UPDATE_DIV) {
                PowerData_t new_data;

                ui_div_count = 0U;
                Data_Monitor_Get_Values(&new_data.voltage, &new_data.current);
                if (osMessageQueuePut(PowerDataQueue, &new_data, 0U, 0U) == osErrorResource) {
                    PowerData_t discarded;

                    osMessageQueueGet(PowerDataQueue, &discarded, NULL, 0U);
                    osMessageQueuePut(PowerDataQueue, &new_data, 0U, 0U);
                }
            }
        }
        if ((flags & FLAG_ADC_FULL_READY) != 0U) {
            packet = Process_ADC_Chunk(&adc_raw_buffer[ADC_TIMES][0], 1U);
            if (packet != NULL) {
                memcpy(pending_adc, packet, sizeof(*packet));
                pending_adc_length = sizeof(*packet);
                adc_pending = true;
            }
            if (++ui_div_count >= UI_UPDATE_DIV) {
                PowerData_t new_data;

                ui_div_count = 0U;
                Data_Monitor_Get_Values(&new_data.voltage, &new_data.current);
                if (osMessageQueuePut(PowerDataQueue, &new_data, 0U, 0U) == osErrorResource) {
                    PowerData_t discarded;

                    osMessageQueueGet(PowerDataQueue, &discarded, NULL, 0U);
                    osMessageQueuePut(PowerDataQueue, &new_data, 0U, 0U);
                }
            }
        }

        if (!command_pending
            && CmdTxQueue != NULL
            && osMessageQueueGet(CmdTxQueue, &pending_command, NULL, 0U) == osOK) {
            command_pending = true;
        }
        if (tx_active) {
            continue;
        }

        if (command_pending) {
            next_data = pending_command.data;
            next_length = pending_command.length;
            next_is_command = true;
        } else if (adc_pending) {
            next_data = pending_adc;
            next_length = pending_adc_length;
        }

        if (next_data != NULL && next_length <= sizeof(active_tx)) {
            memcpy(active_tx, next_data, next_length);
            if (CDC_Transmit_FS(active_tx, next_length) == USBD_OK) {
                tx_active = true;
                if (next_is_command) {
                    command_pending = false;
                } else {
                    adc_pending = false;
                }
            }
        }
    }
}
