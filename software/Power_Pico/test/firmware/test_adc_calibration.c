#include <math.h>
#include <stdio.h>

#include "gate.h"
#include "user_AdcDataStrategy.h"

static int failures;
static ADC_Calibration_t current_calibration;
static uint8_t gate_mode = GATE_MODE_AUTO;
static uint8_t gate_range = HIGH_CUR;

#define CHECK_TRUE(expression)                                                   \
    do {                                                                         \
        if (!(expression)) {                                                     \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression);       \
            failures++;                                                         \
        }                                                                        \
    } while (0)

#define CHECK_FALSE(expression) CHECK_TRUE(!(expression))
#define CHECK_NEAR(expected, actual, tolerance)                                  \
    CHECK_TRUE(fabsf((expected) - (actual)) <= (tolerance))

void Sys_Get_AdcCalibration(ADC_Calibration_t *calibration)
{
    *calibration = current_calibration;
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

static void test_defaults_and_boundaries(void)
{
    ADC_Calibration_t calibration;

    ADC_Calibration_SetDefault(&calibration);
    CHECK_TRUE(sizeof(ADC_Calibration_t) == 6U * sizeof(float));
    CHECK_NEAR(1.0f, calibration.low_scale_multiplier, 1e-7f);
    CHECK_NEAR(1.0f, calibration.mid_scale_multiplier, 1e-7f);
    CHECK_NEAR(1.0f, calibration.high_scale_multiplier, 1e-7f);
    CHECK_NEAR(0.0f, calibration.low_offset_ua, 1e-7f);
    CHECK_NEAR(0.0f, calibration.mid_offset_ua, 1e-7f);
    CHECK_NEAR(0.0f, calibration.high_offset_ua, 1e-7f);
    CHECK_TRUE(ADC_Calibration_IsValid(&calibration));

    calibration.low_scale_multiplier = 0.75f;
    calibration.mid_scale_multiplier = 1.25f;
    calibration.high_offset_ua = 32.0f
        * SCALE_HIGH
        * calibration.high_scale_multiplier;
    CHECK_TRUE(ADC_Calibration_IsValid(&calibration));

    calibration.high_offset_ua += SCALE_HIGH;
    CHECK_FALSE(ADC_Calibration_IsValid(&calibration));
}

static void test_invalid_floats_and_gain(void)
{
    ADC_Calibration_t calibration;

    ADC_Calibration_SetDefault(&calibration);
    calibration.low_scale_multiplier = 0.749f;
    CHECK_FALSE(ADC_Calibration_IsValid(&calibration));

    ADC_Calibration_SetDefault(&calibration);
    calibration.mid_scale_multiplier = 1.251f;
    CHECK_FALSE(ADC_Calibration_IsValid(&calibration));

    ADC_Calibration_SetDefault(&calibration);
    calibration.high_offset_ua = NAN;
    CHECK_FALSE(ADC_Calibration_IsValid(&calibration));

    ADC_Calibration_SetDefault(&calibration);
    calibration.low_scale_multiplier = INFINITY;
    CHECK_FALSE(ADC_Calibration_IsValid(&calibration));
}

static void test_current_conversion_uses_sys_settings(void)
{
    ADC_Calibration_SetDefault(&current_calibration);
    current_calibration.low_scale_multiplier = 1.20f;
    current_calibration.mid_scale_multiplier = 0.80f;
    current_calibration.high_scale_multiplier = 1.10f;
    current_calibration.low_offset_ua = 3.0f;
    current_calibration.mid_offset_ua = -4.0f;
    current_calibration.high_offset_ua = 5.0f;

    CHECK_NEAR(2.0f * SCALE_LOW * 1.20f + 3.0f,
               ADC_Convert_Current_uA(2050U, 2048U, LOW_CUR),
               1e-5f);
    CHECK_NEAR(-3.0f * SCALE_MID * 0.80f - 4.0f,
               ADC_Convert_Current_uA(2045U, 2048U, MID_CUR),
               1e-4f);
    CHECK_NEAR(4.0f * SCALE_HIGH * 1.10f + 5.0f,
               ADC_Convert_Current_uA(2052U, 2048U, HIGH_CUR),
               1e-3f);
}

int main(void)
{
    test_defaults_and_boundaries();
    test_invalid_floats_and_gain();
    test_current_conversion_uses_sys_settings();

    if (failures != 0) {
        printf("%d ADC calibration test(s) failed\n", failures);
        return 1;
    }
    puts("ADC calibration tests passed");
    return 0;
}
