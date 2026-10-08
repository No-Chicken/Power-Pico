#ifndef PD_VOLTAGE_CHECK_H
#define PD_VOLTAGE_CHECK_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

typedef enum {
    PD_VOLTAGE_IDLE, PD_VOLTAGE_WAITING,
    PD_VOLTAGE_VALID, PD_VOLTAGE_MISMATCH
} pd_voltage_result_t;

typedef struct {
    uint32_t ready_at;
    bool tracking, confirmed;
} pd_voltage_check_t;

/* UI measurement catch-up only; this never changes the PD contract. */
static inline pd_voltage_result_t pd_voltage_check_target(
    pd_voltage_check_t *check, bool ready, float voltage, float target, uint32_t now)
{
    if (!ready) {
        *check = (pd_voltage_check_t){0};
        return PD_VOLTAGE_IDLE;
    }
    if (!check->tracking) {
        check->tracking = true;
        check->ready_at = now;
    }
    if (isfinite(voltage) && target >= 5.0f && target <= 20.0f &&
        fabsf(voltage - target) <= 0.5f) {
        check->confirmed = true;
        return PD_VOLTAGE_VALID;
    }
    /* Once confirmed, a later voltage loss must not get another grace period. */
    return !check->confirmed && (uint32_t)(now - check->ready_at) < 1000U ?
        PD_VOLTAGE_WAITING : PD_VOLTAGE_MISMATCH;
}

static inline pd_voltage_result_t pd_voltage_check(
    pd_voltage_check_t *check, bool ready, float voltage, uint32_t now)
{
    return pd_voltage_check_target(check, ready, voltage, 9.0f, now);
}

#endif
