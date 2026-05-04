#ifndef __USER_ADCDATASTRATEGY_H__
#define __USER_ADCDATASTRATEGY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define ADC_TIMES 25 // 每次采样 25 次 (2.5ms)
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

// 自动量程切换阈值默认值（单位：uA）
#define THRESH_LOW_OVERLOAD_UA   550.0f // LOW档过载阈值，达到后尝试升到MID
#define THRESH_MID_OVERLOAD_UA   55000.0f // MID档过载阈值，达到后尝试升到HIGH
#define THRESH_MID_UNDERLOAD_UA  450.0f // MID档欠载阈值，低于后尝试降到LOW
#define THRESH_HIGH_UNDERLOAD_UA 45000.0f // HIGH档欠载阈值，低于后尝试降到MID
// 次数
#define THRESH_TIMES 8

// 监视器更新频率和对应的采样点数量
#define MONITOR_UPDATE_PERIOD_MS 250
#define MONITOR_UPDATE_CHUNK_COUNT (MONITOR_UPDATE_PERIOD_MS / 10)

/* --- USB 数据包结构 (总长 = 11 + 7*ADC_TIMES 字节) --- */
#pragma pack(push, 1) // 强制 1 字节对齐
typedef struct {
    uint8_t  header[2];      // 0x55, 0xAA
    uint64_t timestamp;      // 64位微秒级时间戳
    uint8_t  data_count;     // 本包中有效的数据点数量 (尝试过25和100)

    struct {
        uint8_t  range;      // 量程: 1=Low, 2=Mid, 3=High
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

// 运行时电流校准参数（单位：uA）
typedef struct {
    float low_scale_ua_per_lsb;
    float mid_scale_ua_per_lsb;
    float high_scale_ua_per_lsb;
    float low_offset_ua;
    float mid_offset_ua;
    float high_offset_ua;
} ADC_Calibration_t;

// 运行时自动量程阈值（单位：uA）
typedef struct {
    float low_overload_ua;
    float mid_overload_ua;
    float mid_underload_ua;
    float high_underload_ua;
} ADC_AutoRangeThreshold_t;


extern uint16_t adc_raw_buffer[ADC_TIMES * 2][ADC_CHANNELS];

float ADC_Convert_Current_uA(uint16_t cur_adc, uint16_t ref_adc, uint8_t range);
void Process_ADC_Chunk(uint16_t *chunk_ptr, uint8_t packet_idx);
void Data_Monitor_Get_Values(float *out_vol_v, float *out_cur_ua);
void Data_Monitor_Clear(void);
void ADC_Set_Calibration(const ADC_Calibration_t *cfg);
void ADC_Get_Calibration(ADC_Calibration_t *cfg);
void ADC_Set_AutoRangeThreshold(const ADC_AutoRangeThreshold_t *cfg);
void ADC_Get_AutoRangeThreshold(ADC_AutoRangeThreshold_t *cfg);
uint8_t ADC_RangeStrategy_Evaluate(uint8_t current_hw_range,
                                   uint16_t raw_low,
                                   uint16_t raw_mid,
                                   uint16_t raw_hig,
                                   uint16_t raw_ref);
void ADC_RangeStrategy_ResetDecisionState(void);

#ifdef __cplusplus
}
#endif

#endif
