#include "user_CmdStrategy.h"

#include "BL24C02.h"
#include "gate.h"
#include "usb_device.h"
#include "user_AdcDataStrategy.h"
#include "user_TasksInit.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define CMD_PROTOCOL_MAX_PAYLOAD    48U
#define CMD_PROTOCOL_FRAME_OVERHEAD 10U
#define CMD_PROTOCOL_MAX_FRAME_SIZE (CMD_PROTOCOL_FRAME_OVERHEAD + CMD_PROTOCOL_MAX_PAYLOAD)
#define CMD_PROTOCOL_RX_BUFFER_SIZE (CMD_PROTOCOL_MAX_FRAME_SIZE * 2U)
#define CMD_RESPONSE_MASK           0x80U

typedef enum {
    CMD_STATUS_OK = 0,
    CMD_STATUS_UNSUPPORTED_VERSION = 1,
    CMD_STATUS_UNSUPPORTED_COMMAND = 2,
    CMD_STATUS_INVALID_LENGTH = 3,
    CMD_STATUS_INVALID_PARAMETER = 4,
    CMD_STATUS_EEPROM_ERROR = 5,
    CMD_STATUS_BUSY = 6,
    CMD_STATUS_CRC_ERROR = 7,
} CmdStatus_t;

typedef struct {
    uint8_t version;
    uint8_t type;
    uint16_t sequence;
    uint16_t payload_length;
    uint8_t payload[CMD_PROTOCOL_MAX_PAYLOAD];
} CmdFrame_t;

typedef struct {
    uint8_t buffer[CMD_PROTOCOL_RX_BUFFER_SIZE];
    uint16_t length;
} CmdStrategy_State_t;

static CmdStrategy_State_t cmd_state;

static uint16_t CmdStrategy_ReadU16(const uint8_t *source)
{
    return (uint16_t)source[0] | ((uint16_t)source[1] << 8);
}

static void CmdStrategy_WriteU16(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t)(value & 0xFFU);
    destination[1] = (uint8_t)(value >> 8);
}

static void CmdStrategy_WriteU32(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)(value & 0xFFU);
    destination[1] = (uint8_t)((value >> 8) & 0xFFU);
    destination[2] = (uint8_t)((value >> 16) & 0xFFU);
    destination[3] = (uint8_t)(value >> 24);
}

static uint32_t CmdStrategy_ReadU32(const uint8_t *source)
{
    return (uint32_t)source[0]
        | ((uint32_t)source[1] << 8)
        | ((uint32_t)source[2] << 16)
        | ((uint32_t)source[3] << 24);
}

static void CmdStrategy_WriteFloat(uint8_t *destination, float value)
{
    uint32_t bits;

    memcpy(&bits, &value, sizeof(bits));
    CmdStrategy_WriteU32(destination, bits);
}

static float CmdStrategy_ReadFloat(const uint8_t *source)
{
    uint32_t bits = CmdStrategy_ReadU32(source);
    float value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint16_t CmdStrategy_Crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    uint16_t index;

    for (index = 0U; index < length; index++) {
        uint8_t bit;

        crc ^= (uint16_t)data[index] << 8;
        for (bit = 0U; bit < 8U; bit++) {
            crc = (crc & 0x8000U) != 0U
                ? (uint16_t)((crc << 1) ^ 0x1021U)
                : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static void CmdStrategy_RemovePrefix(uint16_t length)
{
    if (length > cmd_state.length) {
        length = cmd_state.length;
    }
    cmd_state.length = (uint16_t)(cmd_state.length - length);
    if (cmd_state.length != 0U) {
        memmove(cmd_state.buffer, &cmd_state.buffer[length], cmd_state.length);
    }
}

static void CmdStrategy_Feed(const uint8_t *data, uint16_t length)
{
    while (length != 0U) {
        uint16_t copy_length;

        if (cmd_state.length == sizeof(cmd_state.buffer)) {
            CmdStrategy_RemovePrefix(1U);
        }
        copy_length = (uint16_t)(sizeof(cmd_state.buffer) - cmd_state.length);
        if (copy_length > length) {
            copy_length = length;
        }
        memcpy(&cmd_state.buffer[cmd_state.length], data, copy_length);
        cmd_state.length = (uint16_t)(cmd_state.length + copy_length);
        data += copy_length;
        length = (uint16_t)(length - copy_length);
    }
}

static uint16_t CmdStrategy_FindMagic(void)
{
    uint16_t index;

    for (index = 0U; index + 1U < cmd_state.length; index++) {
        if (cmd_state.buffer[index] == 0xA5U && cmd_state.buffer[index + 1U] == 0x5AU) {
            return index;
        }
    }
    return cmd_state.length;
}

static bool CmdStrategy_TryGetFrame(CmdFrame_t *frame)
{
    while (true) {
        uint16_t magic_index = CmdStrategy_FindMagic();
        uint16_t payload_length;
        uint16_t frame_length;
        uint16_t expected_crc;
        uint16_t actual_crc;

        if (magic_index == cmd_state.length) {
            uint16_t keep = cmd_state.length != 0U
                && cmd_state.buffer[cmd_state.length - 1U] == 0xA5U
                ? 1U
                : 0U;
            CmdStrategy_RemovePrefix((uint16_t)(cmd_state.length - keep));
            return false;
        }
        if (magic_index != 0U) {
            CmdStrategy_RemovePrefix(magic_index);
        }
        if (cmd_state.length < 8U) {
            return false;
        }

        payload_length = CmdStrategy_ReadU16(&cmd_state.buffer[6]);
        if (payload_length > CMD_PROTOCOL_MAX_PAYLOAD) {
            CmdStrategy_RemovePrefix(1U);
            continue;
        }
        frame_length = (uint16_t)(CMD_PROTOCOL_FRAME_OVERHEAD + payload_length);
        if (cmd_state.length < frame_length) {
            return false;
        }

        expected_crc = CmdStrategy_ReadU16(&cmd_state.buffer[frame_length - 2U]);
        actual_crc = CmdStrategy_Crc16(&cmd_state.buffer[2],
                                       (uint16_t)(6U + payload_length));
        if (expected_crc != actual_crc) {
            CmdStrategy_RemovePrefix(1U);
            continue;
        }

        frame->version = cmd_state.buffer[2];
        frame->type = cmd_state.buffer[3];
        frame->sequence = CmdStrategy_ReadU16(&cmd_state.buffer[4]);
        frame->payload_length = payload_length;
        if (payload_length != 0U) {
            memcpy(frame->payload, &cmd_state.buffer[8], payload_length);
        }
        CmdStrategy_RemovePrefix(frame_length);
        return true;
    }
}

static uint16_t CmdStrategy_EncodeFrame(uint8_t type,
                                        uint16_t sequence,
                                        const uint8_t *payload,
                                        uint16_t payload_length,
                                        uint8_t *output)
{
    uint16_t crc;

    if (payload_length > CMD_PROTOCOL_MAX_PAYLOAD
        || (payload == NULL && payload_length != 0U)) {
        return 0U;
    }

    output[0] = 0xA5U;
    output[1] = 0x5AU;
    output[2] = CMD_PROTOCOL_VERSION;
    output[3] = type;
    CmdStrategy_WriteU16(&output[4], sequence);
    CmdStrategy_WriteU16(&output[6], payload_length);
    if (payload_length != 0U) {
        memcpy(&output[8], payload, payload_length);
    }
    crc = CmdStrategy_Crc16(&output[2], (uint16_t)(6U + payload_length));
    CmdStrategy_WriteU16(&output[8U + payload_length], crc);
    return (uint16_t)(CMD_PROTOCOL_FRAME_OVERHEAD + payload_length);
}

static uint16_t CmdStrategy_EncodeResponse(const CmdFrame_t *request,
                                           CmdStatus_t status,
                                           const uint8_t *payload,
                                           uint16_t payload_length,
                                           uint8_t *output)
{
    uint8_t response_payload[CMD_PROTOCOL_MAX_PAYLOAD];

    if (payload_length >= CMD_PROTOCOL_MAX_PAYLOAD
        || (payload == NULL && payload_length != 0U)) {
        return 0U;
    }
    response_payload[0] = (uint8_t)status;
    if (payload_length != 0U) {
        memcpy(&response_payload[1], payload, payload_length);
    }
    return CmdStrategy_EncodeFrame((uint8_t)(request->type | CMD_RESPONSE_MASK),
                                   request->sequence,
                                   response_payload,
                                   (uint16_t)(payload_length + 1U),
                                   output);
}

static void CmdStrategy_WriteCalibrationPayload(uint8_t *payload,
                                                const ADC_Calibration_t *calibration)
{
    CmdStrategy_WriteFloat(&payload[0], calibration->low_scale_multiplier);
    CmdStrategy_WriteFloat(&payload[4], calibration->low_offset_ua);
    CmdStrategy_WriteFloat(&payload[8], calibration->mid_scale_multiplier);
    CmdStrategy_WriteFloat(&payload[12], calibration->mid_offset_ua);
    CmdStrategy_WriteFloat(&payload[16], calibration->high_scale_multiplier);
    CmdStrategy_WriteFloat(&payload[20], calibration->high_offset_ua);
}

static void CmdStrategy_ReadCalibrationPayload(ADC_Calibration_t *calibration,
                                               const uint8_t *payload)
{
    calibration->low_scale_multiplier = CmdStrategy_ReadFloat(&payload[0]);
    calibration->low_offset_ua = CmdStrategy_ReadFloat(&payload[4]);
    calibration->mid_scale_multiplier = CmdStrategy_ReadFloat(&payload[8]);
    calibration->mid_offset_ua = CmdStrategy_ReadFloat(&payload[12]);
    calibration->high_scale_multiplier = CmdStrategy_ReadFloat(&payload[16]);
    calibration->high_offset_ua = CmdStrategy_ReadFloat(&payload[20]);
}

static CmdStatus_t CmdStrategy_SaveCalibration(const ADC_Calibration_t *calibration)
{
    ADC_Calibration_t previous;

    Sys_Get_AdcCalibration(&previous);
    if (!Sys_Set_AdcCalibration(calibration)) {
        return CMD_STATUS_INVALID_PARAMETER;
    }
    if (!EEPROM_SysSetting_Save()) {
        Sys_Set_AdcCalibration(&previous);
        return CMD_STATUS_EEPROM_ERROR;
    }
    return CMD_STATUS_OK;
}

static bool CmdStrategy_Execute(const CmdFrame_t *request, CmdTxFrame_t *response)
{
    CmdStatus_t status = CMD_STATUS_OK;
    ADC_Calibration_t calibration;
    uint8_t payload[CMD_PROTOCOL_MAX_PAYLOAD - 1U];
    uint16_t payload_length = 0U;
    bool update_requested = false;

    if (request->version != CMD_PROTOCOL_VERSION) {
        status = CMD_STATUS_UNSUPPORTED_VERSION;
    } else {
        switch (request->type) {
            case CMD_CAL_GET:
                if (request->payload_length != 0U) {
                    status = CMD_STATUS_INVALID_LENGTH;
                } else {
                    Sys_Get_AdcCalibration(&calibration);
                    CmdStrategy_WriteCalibrationPayload(payload, &calibration);
                    payload_length = 24U;
                }
                break;

            case CMD_RANGE_GET:
                if (request->payload_length != 0U) {
                    status = CMD_STATUS_INVALID_LENGTH;
                } else {
                    payload[0] = Gate_Get_Mode();
                    payload[1] = Gate_get_status();
                    payload_length = 2U;
                }
                break;

            case CMD_RANGE_SET:
                if (request->payload_length != 1U) {
                    status = CMD_STATUS_INVALID_LENGTH;
                } else if (request->payload[0] > GATE_MODE_HIGH) {
                    status = CMD_STATUS_INVALID_PARAMETER;
                } else {
                    Gate_Set_Mode(request->payload[0]);
                    payload[0] = Gate_Get_Mode();
                    payload[1] = Gate_get_status();
                    payload_length = 2U;
                }
                break;

            case CMD_CAL_SET:
                if (request->payload_length != 24U) {
                    status = CMD_STATUS_INVALID_LENGTH;
                } else {
                    Sys_Get_AdcCalibration(&calibration);
                    CmdStrategy_ReadCalibrationPayload(&calibration, request->payload);
                    status = CmdStrategy_SaveCalibration(&calibration);
                    if (status == CMD_STATUS_OK) {
                        Sys_Get_AdcCalibration(&calibration);
                        CmdStrategy_WriteCalibrationPayload(payload, &calibration);
                        payload_length = 24U;
                    }
                }
                break;

            case CMD_CAL_RESET:
                if (request->payload_length != 2U) {
                    status = CMD_STATUS_INVALID_LENGTH;
                } else if (request->payload[0] != 0xC3U
                           || request->payload[1] != 0x3CU) {
                    status = CMD_STATUS_INVALID_PARAMETER;
                } else {
                    ADC_Calibration_SetDefault(&calibration);
                    status = CmdStrategy_SaveCalibration(&calibration);
                    if (status == CMD_STATUS_OK) {
                        Sys_Get_AdcCalibration(&calibration);
                        CmdStrategy_WriteCalibrationPayload(payload, &calibration);
                        payload_length = 24U;
                    }
                }
                break;

            case CMD_FW_UPDATE:
                if (request->payload_length != 2U) {
                    status = CMD_STATUS_INVALID_LENGTH;
                } else if (request->payload[0] != 0xC3U
                           || request->payload[1] != 0x3CU) {
                    status = CMD_STATUS_INVALID_PARAMETER;
                } else {
                    update_requested = true;
                }
                break;

            default:
                status = CMD_STATUS_UNSUPPORTED_COMMAND;
                break;
        }
    }

    if (update_requested) {
        response->length = 0U;
        return true;
    }
    response->length = CmdStrategy_EncodeResponse(request,
                                                  status,
                                                  payload,
                                                  payload_length,
                                                  response->data);
    return false;
}

static bool CmdStrategy_QueueResponse(const CmdTxFrame_t *response)
{
    if (response == NULL
        || response->length == 0U
        || CmdTxQueue == NULL
        || MessageSendTaskHandle == NULL) {
        return false;
    }
    if (osMessageQueuePut(CmdTxQueue, response, 0U, osWaitForever) != osOK) {
        return false;
    }
    osThreadFlagsSet(MessageSendTaskHandle, FLAG_CMD_TX_READY);
    return true;
}

static void CmdStrategy_EnterFirmwareUpdate(void)
{
    EEPROM_UpdateCommand_Write(true);
    HAL_Delay(100U);
    USER_USB_DEVICE_DeInit();
    HAL_Delay(500U);
    NVIC_SystemReset();
}

void CmdStrategy_Init(void)
{
    memset(&cmd_state, 0, sizeof(cmd_state));
}

void CmdStrategy_ProcessRx(const uint8_t *data, uint16_t length)
{
    CmdFrame_t request;
    CmdTxFrame_t response;

    if (data == NULL || length == 0U) {
        return;
    }

    CmdStrategy_Feed(data, length);
    while (CmdStrategy_TryGetFrame(&request)) {
        bool update_requested = CmdStrategy_Execute(&request, &response);

        if (response.length != 0U) {
            CmdStrategy_QueueResponse(&response);
        }
        if (update_requested) {
            CmdStrategy_EnterFirmwareUpdate();
            return;
        }
    }
}
