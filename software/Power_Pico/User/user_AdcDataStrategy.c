#include "user_AdcDataStrategy.h"
#include "BL24C02.h"
#include "gate.h"
#include "tim.h"
#include <math.h>

// DMA 双缓冲原始数据
uint16_t adc_raw_buffer[ADC_TIMES * 2][ADC_CHANNELS];

// USB 发送缓冲区 (Ping-Pong 双缓冲防止发送冲突)
static USB_ADC_Packet_t usb_packet_buffer[2];

// 统计实例
static Data_Monitor_t g_data_monitor = {0};

// 用于监视数据更新的计数器
static uint32_t monitor_chunk_counter = 0;

// 物理切换防抖计数器（策略内部状态）
static uint8_t high_overload_cnt = 0;
static uint8_t low_underload_cnt = 0;
static volatile uint8_t transition_skip_chunks = 0;
static volatile uint8_t range_switch_cooldown_chunks = 0;

// 内部调用的数据更新函数，mV 和 10nA 累加
static void Data_Monitor_Update(uint16_t vol_adc, uint16_t cur_adc, uint16_t ref_adc, uint8_t range)
{
    // V = (ADC_Value / 4095) * 3.0V * (1000k + 100k) / 100k
    float voltage_mv = (float)vol_adc * (3000.0f / 4095.0f * 11.0f);

    // I_uA = (ADC差值 * scale * multiplier) + offset, I_10nA = I_uA * 100
    float current_ua = ADC_Convert_Current_uA(cur_adc, ref_adc, range);
    int64_t current_10na = (int64_t)(current_ua * 100.0f);

    // mV 除以 1.1MΩ，得到 10nA 分辨率下的分压误差电流
    float error_current_10na = voltage_mv / 11.0f;
    float final_current_10na = current_10na - error_current_10na;

    g_data_monitor.sum_vol_mv += (uint64_t)voltage_mv;
    g_data_monitor.sum_cur_resolution_10na += final_current_10na;
    g_data_monitor.count++;
}

// 内部调用的计算函数，计算平均值并重置累加器
static void Data_Monitor_Calculate_Average(void)
{
    __disable_irq();

    if (g_data_monitor.count > 0)
    {
        g_data_monitor.avg_vol_v = (float)g_data_monitor.sum_vol_mv / g_data_monitor.count / 1000.0f;
        g_data_monitor.avg_cur_ua = (float)g_data_monitor.sum_cur_resolution_10na / g_data_monitor.count / 100.0f;
    }
    else
    {
        g_data_monitor.avg_vol_v = 0.0f;
        g_data_monitor.avg_cur_ua = 0.0f;
    }

    g_data_monitor.sum_vol_mv = 0;
    g_data_monitor.sum_cur_resolution_10na = 0;
    g_data_monitor.count = 0;

    __enable_irq();
}

void Data_Monitor_Get_Values(float *out_vol_v, float *out_cur_ua)
{
    *out_vol_v = g_data_monitor.avg_vol_v;
    *out_cur_ua = g_data_monitor.avg_cur_ua;
}

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

// 内部使用的自动量程策略阈值
static ADC_AutoRangeCodeThreshold_t g_adc_autorange_code_threshold = {
    .low_overload_code = THRESH_HIGH,
    .mid_overload_code = THRESH_HIGH,
    .mid_underload_code = THRESH_LOW,
    .high_underload_code = THRESH_LOW,
    .mid_near_rail_margin_code = THRESH_LOW,
    .up_confirm_times = THRESH_UP_CONFIRM_TIMES,
    .down_confirm_times = THRESH_DOWN_CONFIRM_TIMES
};

static uint16_t adc_abs_diff_u16(uint16_t a, uint16_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

void ADC_Calibration_SetDefault(ADC_Calibration_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    cfg->low_scale_multiplier = 1.0f;
    cfg->mid_scale_multiplier = 1.0f;
    cfg->high_scale_multiplier = 1.0f;
    cfg->low_offset_ua = 0.0f;
    cfg->mid_offset_ua = 0.0f;
    cfg->high_offset_ua = 0.0f;
}

static bool ADC_Calibration_RangeIsValid(float hardware_scale_ua_per_lsb,
                                         float multiplier,
                                         float offset_ua)
{
    float offset_lsb;

    if (!isfinite(multiplier)
        || !isfinite(offset_ua)
        || multiplier < 0.75f
        || multiplier > 1.25f) {
        return false;
    }

    offset_lsb = offset_ua / (hardware_scale_ua_per_lsb * multiplier);
    return isfinite(offset_lsb) && fabsf(offset_lsb) <= 32.0f;
}

bool ADC_Calibration_IsValid(const ADC_Calibration_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }

    return ADC_Calibration_RangeIsValid(SCALE_LOW,
                                        cfg->low_scale_multiplier,
                                        cfg->low_offset_ua)
        && ADC_Calibration_RangeIsValid(SCALE_MID,
                                        cfg->mid_scale_multiplier,
                                        cfg->mid_offset_ua)
        && ADC_Calibration_RangeIsValid(SCALE_HIGH,
                                        cfg->high_scale_multiplier,
                                        cfg->high_offset_ua);
}

float ADC_Convert_Current_uA(uint16_t cur_adc, uint16_t ref_adc, uint8_t range)
{
    ADC_Calibration_t calibration;
    float delta_lsb = (float)((int32_t)cur_adc - (int32_t)ref_adc);

    Sys_Get_AdcCalibration(&calibration);

    switch (range) {
        case LOW_CUR:
            return delta_lsb
                * SCALE_LOW
                * calibration.low_scale_multiplier
                + calibration.low_offset_ua;
        case MID_CUR:
            return delta_lsb
                * SCALE_MID
                * calibration.mid_scale_multiplier
                + calibration.mid_offset_ua;
        case HIGH_CUR:
            return delta_lsb
                * SCALE_HIGH
                * calibration.high_scale_multiplier
                + calibration.high_offset_ua;
        default:
            return 0.0f;
    }
}

void ADC_Set_AutoRangeCodeThreshold(const ADC_AutoRangeCodeThreshold_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    __disable_irq();
    g_adc_autorange_code_threshold = *cfg;
    __enable_irq();
}

void ADC_Get_AutoRangeCodeThreshold(ADC_AutoRangeCodeThreshold_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    __disable_irq();
    *cfg = g_adc_autorange_code_threshold;
    __enable_irq();
}

static void ADC_RangeStrategy_ResetDecisionState(void)
{
    high_overload_cnt = 0;
    low_underload_cnt = 0;
}

USB_ADC_Packet_t *Process_ADC_Chunk(uint16_t *chunk_ptr, uint8_t packet_idx)
{
    USB_ADC_Packet_t *pkg = &usb_packet_buffer[packet_idx];
    uint8_t gate_mode = Gate_Get_Mode();
    uint8_t current_hw_range = Gate_get_status();
    uint8_t req_switch_range = current_hw_range;

    if (transition_skip_chunks > 0U) {
        __disable_irq();
        if (transition_skip_chunks > 0U) {
            transition_skip_chunks--;
        }
        __enable_irq();
        return NULL;
    }

    if (gate_mode != GATE_MODE_AUTO) {
        if (current_hw_range != gate_mode) {
            flow_route_selection(gate_mode);
            current_hw_range = gate_mode;
        }
        req_switch_range = current_hw_range;
        ADC_RangeStrategy_ResetDecisionState();
    }

    pkg->header[0] = PACKET_HEADER_0;
    pkg->header[1] = PACKET_HEADER_1;
    pkg->timestamp = GetMicrosecondCounter();
    pkg->data_count = ADC_TIMES;

    int i = 0;
    for (i = 0; i < ADC_TIMES; i++)
    {
        uint16_t *sample_row = chunk_ptr + (i * ADC_CHANNELS);
        uint16_t raw_vol = sample_row[IDX_VOL];
        uint16_t raw_low = sample_row[IDX_LOW];
        uint16_t raw_mid = sample_row[IDX_MID];
        uint16_t raw_hig = sample_row[IDX_HIGH];
        uint16_t raw_ref = sample_row[IDX_REF];

        if (gate_mode == GATE_MODE_AUTO && range_switch_cooldown_chunks == 0U) {
            switch (current_hw_range)
            {
                case LOW_CUR:
                {
                    uint16_t low_delta = adc_abs_diff_u16(raw_low, raw_ref);
                    if (low_delta >= g_adc_autorange_code_threshold.low_overload_code) {
                        high_overload_cnt++;
                        if (high_overload_cnt >= g_adc_autorange_code_threshold.up_confirm_times) {
                            req_switch_range = MID_CUR;
                        }
                    } else {
                        high_overload_cnt = 0;
                    }
                    low_underload_cnt = 0;
                    break;
                }

                case MID_CUR:
                {
                    uint16_t mid_delta = adc_abs_diff_u16(raw_mid, raw_ref);
                    uint16_t margin = g_adc_autorange_code_threshold.mid_near_rail_margin_code;
                    uint8_t mid_near_rail = (raw_mid >= (uint16_t)(4095U - margin)) || (raw_mid <= margin);

                    if ((mid_delta >= g_adc_autorange_code_threshold.mid_overload_code) || mid_near_rail) {
                        high_overload_cnt++;
                        if (high_overload_cnt >= g_adc_autorange_code_threshold.up_confirm_times) {
                            req_switch_range = HIGH_CUR;
                        }
                    } else {
                        high_overload_cnt = 0;
                    }

                    if (mid_delta <= g_adc_autorange_code_threshold.mid_underload_code) {
                        low_underload_cnt++;
                        if (low_underload_cnt >= g_adc_autorange_code_threshold.down_confirm_times) {
                            req_switch_range = LOW_CUR;
                        }
                    } else {
                        low_underload_cnt = 0;
                    }
                    break;
                }

                case HIGH_CUR:
                {
                    uint16_t high_delta = adc_abs_diff_u16(raw_hig, raw_ref);
                    if (high_delta <= g_adc_autorange_code_threshold.high_underload_code) {
                        low_underload_cnt++;
                        if (low_underload_cnt >= g_adc_autorange_code_threshold.down_confirm_times) {
                            req_switch_range = MID_CUR;
                        }
                    } else {
                        low_underload_cnt = 0;
                    }
                    high_overload_cnt = 0;
                    break;
                }

                default:
                    ADC_RangeStrategy_ResetDecisionState();
                    break;
            }

            if (req_switch_range != current_hw_range) {
                break;
            }
        }

        uint16_t final_cur = 0;
        switch (current_hw_range) {
            case LOW_CUR:  final_cur = raw_low; break;
            case MID_CUR:  final_cur = raw_mid; break;
            case HIGH_CUR: final_cur = raw_hig; break;
            default:       final_cur = raw_hig; break;
        }

        pkg->samples[i].range = current_hw_range;
        pkg->samples[i].vol_adc = raw_vol;
        pkg->samples[i].cur_adc = final_cur;
        pkg->samples[i].ref_adc = raw_ref;

        Data_Monitor_Update(raw_vol, final_cur, raw_ref, current_hw_range);
    }

    pkg->data_count = (uint8_t)i;

    if (gate_mode == GATE_MODE_AUTO && req_switch_range != current_hw_range) {
        flow_route_selection(req_switch_range);
        transition_skip_chunks = 1;
        range_switch_cooldown_chunks = RANGE_SWITCH_COOLDOWN_CHUNKS;
        ADC_RangeStrategy_ResetDecisionState();
    } else if (range_switch_cooldown_chunks > 0U) {
        range_switch_cooldown_chunks--;
    }

    monitor_chunk_counter++;
    if (monitor_chunk_counter >= MONITOR_UPDATE_CHUNK_COUNT)
    {
        monitor_chunk_counter = 0;
        Data_Monitor_Calculate_Average();
    }

    return pkg;
}
