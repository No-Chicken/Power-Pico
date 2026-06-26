#ifndef __USER_ADCDATASTRATEGY_H__
#define __USER_ADCDATASTRATEGY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define ADC_TIMES 10 // 每次采样 10 次 (1ms @10kHz)
#define ADC_CHANNELS 5 // 5 通道：电压 + 3 档电流 + REF
#define PACKET_HEADER_0 0xAA
#define PACKET_HEADER_1 0x55
#define ADC_SAMPLE_RATE_HZ 10000U

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

// 预先计算标度因子 默认值
#define SCALE_LOW  (3.0f / 4096.0f / 50.0f / LOW_CUR_RES *  1000000.0) // uA
#define SCALE_MID  (3.0f / 4096.0f / 50.0f / MID_CUR_RES *  1000000.0) // uA
#define SCALE_HIGH (3.0f / 4096.0f / 50.0f / HIGH_CUR_RES * 1000000.0) // uA

// 量程阈值 默认值
#define THRESH_HIGH 4000-2048 // 对应550uA 50mA
#define THRESH_LOW  15 // 对应大概450uA 45mA

// 次数
#define THRESH_UP_CONFIRM_TIMES    4U
#define THRESH_DOWN_CONFIRM_TIMES  ADC_TIMES
#define RANGE_SWITCH_COOLDOWN_MS   ADC_SAMPLE_RATE_HZ/1000/ADC_TIMES*5U // 5个采样块的冷却时间

// 监视器更新频率和对应的采样点数量
#define MONITOR_UPDATE_PERIOD_MS 250
#define ADC_CHUNK_PERIOD_MS (((ADC_TIMES) * 1000U + ADC_SAMPLE_RATE_HZ - 1U) / ADC_SAMPLE_RATE_HZ)
#define MONITOR_UPDATE_CHUNK_COUNT ((MONITOR_UPDATE_PERIOD_MS + ADC_CHUNK_PERIOD_MS - 1U) / ADC_CHUNK_PERIOD_MS)
#define RANGE_SWITCH_COOLDOWN_CHUNKS ((RANGE_SWITCH_COOLDOWN_MS + ADC_CHUNK_PERIOD_MS - 1U) / ADC_CHUNK_PERIOD_MS)

/* --- USB 数据包结构 (总长 = 11 + 7*ADC_TIMES 字节) --- */
#pragma pack(push, 1) // 强制 1 字节对齐
typedef struct {
    uint8_t  header[2];      // 0xAA, 0x55
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

// 快速码值判据参数（单位：ADC LSB）
typedef struct {
    uint16_t low_overload_code;
    uint16_t mid_overload_code;
    uint16_t mid_underload_code;
    uint16_t high_underload_code;
    uint16_t mid_near_rail_margin_code;
    uint8_t up_confirm_times;
    uint8_t down_confirm_times;
} ADC_AutoRangeCodeThreshold_t;

extern uint16_t adc_raw_buffer[ADC_TIMES * 2][ADC_CHANNELS];

void Process_ADC_Chunk(uint16_t *chunk_ptr, uint8_t packet_idx);

float ADC_Convert_Current_uA(uint16_t cur_adc, uint16_t ref_adc, uint8_t range);
void Data_Monitor_Get_Values(float *out_vol_v, float *out_cur_ua);
void Data_Monitor_Clear(void);
void ADC_Set_Calibration(const ADC_Calibration_t *cfg);
void ADC_Get_Calibration(ADC_Calibration_t *cfg);
void ADC_Set_AutoRangeCodeThreshold(const ADC_AutoRangeCodeThreshold_t *cfg);
void ADC_Get_AutoRangeCodeThreshold(ADC_AutoRangeCodeThreshold_t *cfg);

#ifdef __cplusplus
}
#endif

#endif
