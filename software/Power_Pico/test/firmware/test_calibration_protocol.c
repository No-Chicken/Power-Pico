#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gate.h"
#include "user_AdcDataStrategy.h"
#include "user_CmdStrategy.h"
#include "user_TasksInit.h"

static int failures;
static ADC_Calibration_t current_calibration;
static bool eeprom_save_result = true;
static int eeprom_save_count;
static int update_write_count;
static int reset_count;
static uint8_t gate_mode = GATE_MODE_AUTO;
static uint8_t gate_range = HIGH_CUR;
static CmdTxFrame_t responses[16];
static int response_count;

osMessageQueueId_t CmdTxQueue = (void *)1;
osThreadId_t MessageSendTaskHandle = (void *)1;

#define CHECK_TRUE(expression)                                                   \
    do {                                                                         \
        if (!(expression)) {                                                     \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression);       \
            failures++;                                                         \
        }                                                                        \
    } while (0)

#define CHECK_EQ_U8(expected, actual) CHECK_TRUE((uint8_t)(expected) == (actual))
#define CHECK_EQ_U16(expected, actual) CHECK_TRUE((uint16_t)(expected) == (actual))
#define CHECK_NEAR(expected, actual, tolerance)                                  \
    CHECK_TRUE(fabsf((expected) - (actual)) <= (tolerance))

void Sys_Get_AdcCalibration(ADC_Calibration_t *calibration)
{
    *calibration = current_calibration;
}

bool Sys_Set_AdcCalibration(const ADC_Calibration_t *calibration)
{
    if (!ADC_Calibration_IsValid(calibration)) {
        return false;
    }
    current_calibration = *calibration;
    return true;
}

bool EEPROM_SysSetting_Save(void)
{
    eeprom_save_count++;
    return eeprom_save_result;
}

void EEPROM_UpdateCommand_Write(bool is_update)
{
    if (is_update) {
        update_write_count++;
    }
}

void HAL_Delay(uint32_t milliseconds)
{
    (void)milliseconds;
}

void USER_USB_DEVICE_DeInit(void)
{
}

void NVIC_SystemReset(void)
{
    reset_count++;
}

void flow_route_selection(uint8_t selection)
{
    gate_range = selection;
}

uint8_t Gate_get_status(void)
{
    return gate_range;
}

void Gate_Set_Mode(uint8_t mode)
{
    gate_mode = mode;
    if (mode != GATE_MODE_AUTO) {
        gate_range = mode;
    }
}

uint8_t Gate_Get_Mode(void)
{
    return gate_mode;
}

uint64_t GetMicrosecondCounter(void)
{
    return 0U;
}

osStatus_t osMessageQueuePut(osMessageQueueId_t queue,
                             const void *message,
                             uint8_t priority,
                             uint32_t timeout)
{
    (void)queue;
    (void)priority;
    (void)timeout;
    if (response_count < (int)(sizeof(responses) / sizeof(responses[0]))) {
        responses[response_count++] = *(const CmdTxFrame_t *)message;
    }
    return osOK;
}

uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags)
{
    (void)thread_id;
    return flags;
}

static uint16_t crc16(const uint8_t *data, uint16_t length)
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

static void write_u16(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8);
}

static void write_float(uint8_t *destination, float value)
{
    memcpy(destination, &value, sizeof(value));
}

static float read_float(const uint8_t *source)
{
    float value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static uint16_t make_frame(uint8_t type,
                           uint16_t sequence,
                           const uint8_t *payload,
                           uint16_t payload_length,
                           uint8_t *output)
{
    uint16_t crc;

    output[0] = 0xA5U;
    output[1] = 0x5AU;
    output[2] = CMD_PROTOCOL_VERSION;
    output[3] = type;
    write_u16(&output[4], sequence);
    write_u16(&output[6], payload_length);
    if (payload_length != 0U) {
        memcpy(&output[8], payload, payload_length);
    }
    crc = crc16(&output[2], (uint16_t)(6U + payload_length));
    write_u16(&output[8U + payload_length], crc);
    return (uint16_t)(10U + payload_length);
}

static void reset_fixture(void)
{
    ADC_Calibration_SetDefault(&current_calibration);
    eeprom_save_result = true;
    eeprom_save_count = 0;
    update_write_count = 0;
    reset_count = 0;
    gate_mode = GATE_MODE_AUTO;
    gate_range = HIGH_CUR;
    response_count = 0;
    CmdStrategy_Init();
}

static void test_literal_get_and_fragmentation(void)
{
    static const uint8_t expected[] = {
        0xA5U, 0x5AU, 0x01U, 0x10U, 0x34U,
        0x12U, 0x00U, 0x00U, 0xF1U, 0x84U,
    };
    uint8_t request[58];
    uint16_t length;

    reset_fixture();
    length = make_frame(CMD_CAL_GET, 0x1234U, NULL, 0U, request);
    CHECK_EQ_U16(sizeof(expected), length);
    CHECK_TRUE(memcmp(expected, request, sizeof(expected)) == 0);

    CmdStrategy_ProcessRx(request, 3U);
    CHECK_TRUE(response_count == 0);
    CmdStrategy_ProcessRx(&request[3], (uint16_t)(length - 3U));
    CHECK_TRUE(response_count == 1);
    CHECK_EQ_U16(35U, responses[0].length);
    CHECK_EQ_U8(CMD_CAL_GET | 0x80U, responses[0].data[3]);
    CHECK_EQ_U16(25U, (uint16_t)responses[0].data[6]
                           | ((uint16_t)responses[0].data[7] << 8));
    CHECK_EQ_U8(0U, responses[0].data[8]);
    CHECK_NEAR(1.0f, read_float(&responses[0].data[9]), 1e-6f);
}

static void test_concatenation_and_crc_recovery(void)
{
    uint8_t first[58];
    uint8_t second[58];
    uint8_t stream[116];
    uint16_t first_length;
    uint16_t second_length;

    reset_fixture();
    first_length = make_frame(CMD_RANGE_GET, 1U, NULL, 0U, first);
    second_length = make_frame(CMD_CAL_GET, 2U, NULL, 0U, second);
    first[first_length - 1U] ^= 0xFFU;
    memcpy(stream, first, first_length);
    memcpy(&stream[first_length], second, second_length);
    CmdStrategy_ProcessRx(stream, (uint16_t)(first_length + second_length));

    CHECK_TRUE(response_count == 1);
    CHECK_EQ_U8(CMD_CAL_GET | 0x80U, responses[0].data[3]);

    reset_fixture();
    first_length = make_frame(CMD_RANGE_GET, 3U, NULL, 0U, first);
    memcpy(stream, first, first_length);
    memcpy(&stream[first_length], second, second_length);
    CmdStrategy_ProcessRx(stream, (uint16_t)(first_length + second_length));
    CHECK_TRUE(response_count == 2);
}

static void test_range_get_reads_current_state_for_repeated_frame(void)
{
    uint8_t request[58];
    uint16_t length;

    reset_fixture();
    length = make_frame(CMD_RANGE_GET, 7U, NULL, 0U, request);

    CmdStrategy_ProcessRx(request, length);
    CHECK_TRUE(response_count == 1);
    CHECK_EQ_U8(GATE_MODE_AUTO, responses[0].data[9]);
    CHECK_EQ_U8(HIGH_CUR, responses[0].data[10]);

    Gate_Set_Mode(GATE_MODE_HIGH);
    CmdStrategy_ProcessRx(request, length);
    CHECK_TRUE(response_count == 2);
    CHECK_EQ_U8(GATE_MODE_HIGH, responses[1].data[9]);
    CHECK_EQ_U8(HIGH_CUR, responses[1].data[10]);
}

static void test_set_all_repeated_request_and_rollback(void)
{
    uint8_t payload[24];
    uint8_t request[58];
    uint16_t length;
    ADC_Calibration_t previous;

    reset_fixture();
    write_float(&payload[0], 1.1f);
    write_float(&payload[4], -1.0f);
    write_float(&payload[8], 1.2f);
    write_float(&payload[12], -2.0f);
    write_float(&payload[16], 0.9f);
    write_float(&payload[20], 3.0f);
    length = make_frame(CMD_CAL_SET, 10U, payload, sizeof(payload), request);

    CmdStrategy_ProcessRx(request, length);
    CHECK_TRUE(eeprom_save_count == 1);
    CHECK_NEAR(1.2f, current_calibration.mid_scale_multiplier, 1e-6f);
    CHECK_NEAR(-2.0f, current_calibration.mid_offset_ua, 1e-6f);
    CHECK_EQ_U8(0U, responses[0].data[8]);

    CmdStrategy_ProcessRx(request, length);
    CHECK_TRUE(eeprom_save_count == 2);
    CHECK_TRUE(response_count == 2);

    previous = current_calibration;
    eeprom_save_result = false;
    write_float(&payload[0], 1.15f);
    length = make_frame(CMD_CAL_SET, 11U, payload, sizeof(payload), request);
    CmdStrategy_ProcessRx(request, length);
    CHECK_EQ_U8(5U, responses[2].data[8]);
    CHECK_NEAR(previous.low_scale_multiplier,
               current_calibration.low_scale_multiplier,
               1e-6f);
}

static void test_firmware_update_frame_triggers_reset_without_response(void)
{
    static const uint8_t guard[] = {0xC3U, 0x3CU};
    uint8_t request[58];
    uint16_t length;

    reset_fixture();
    length = make_frame(0x15U, 20U, guard, sizeof(guard), request);
    CmdStrategy_ProcessRx(request, length);

    CHECK_TRUE(response_count == 0);
    CHECK_TRUE(update_write_count == 1);
    CHECK_TRUE(reset_count == 1);
}

static void test_firmware_update_rejects_invalid_length(void)
{
    uint8_t request[58];
    uint16_t length;

    reset_fixture();
    length = make_frame(0x15U, 21U, NULL, 0U, request);
    CmdStrategy_ProcessRx(request, length);

    CHECK_TRUE(response_count == 1);
    CHECK_EQ_U8(3U, responses[0].data[8]);
    CHECK_TRUE(update_write_count == 0);
    CHECK_TRUE(reset_count == 0);
}

static void test_firmware_update_rejects_invalid_guard(void)
{
    static const uint8_t invalid_guard[] = {0xC3U, 0x00U};
    uint8_t request[58];
    uint16_t length;

    reset_fixture();
    length = make_frame(0x15U,
                        22U,
                        invalid_guard,
                        sizeof(invalid_guard),
                        request);
    CmdStrategy_ProcessRx(request, length);

    CHECK_TRUE(response_count == 1);
    CHECK_EQ_U8(4U, responses[0].data[8]);
    CHECK_TRUE(update_write_count == 0);
    CHECK_TRUE(reset_count == 0);
}

static void test_legacy_ascii_update_is_ignored(void)
{
    static const uint8_t update_command[] = "update\r\n";

    reset_fixture();
    CmdStrategy_ProcessRx(update_command,
                          (uint16_t)(sizeof(update_command) - 1U));

    CHECK_TRUE(response_count == 0);
    CHECK_TRUE(update_write_count == 0);
    CHECK_TRUE(reset_count == 0);
}

int main(void)
{
    test_literal_get_and_fragmentation();
    test_concatenation_and_crc_recovery();
    test_range_get_reads_current_state_for_repeated_frame();
    test_set_all_repeated_request_and_rollback();
    test_firmware_update_frame_triggers_reset_without_response();
    test_firmware_update_rejects_invalid_length();
    test_firmware_update_rejects_invalid_guard();
    test_legacy_ascii_update_is_ignored();

    if (failures != 0) {
        printf("%d command strategy test(s) failed\n", failures);
        return 1;
    }
    puts("Command strategy tests passed");
    return 0;
}
