#ifndef fixed_voltage_control_H
#define fixed_voltage_control_H

#include "FUSB302_UFP.h"
#include "PD_UFP_Protocol.h"

typedef enum {
    fixed_voltage_control_IDLE, fixed_voltage_control_ATTACH, fixed_voltage_control_CAPS, fixed_voltage_control_ACCEPT,
    fixed_voltage_control_POWER, fixed_voltage_control_READY, fixed_voltage_control_REJECTED, fixed_voltage_control_WAIT,
    fixed_voltage_control_TIMEOUT, fixed_voltage_control_DISCONNECTED, fixed_voltage_control_IO_ERROR,
    fixed_voltage_control_INVALID_CAPS, fixed_voltage_control_NO_9V, fixed_voltage_control_RESET, fixed_voltage_control_BUSY
} fixed_voltage_control_state_t;

typedef struct {
    fixed_voltage_control_state_t state;
    uint32_t pdo[7];
    uint16_t source_current_ma;
    uint16_t request_current_ma;
    uint8_t count;
    uint8_t cc;
    uint8_t source_revision;
    uint16_t target_mv, contract_mv;
    bool pps;
    bool contract_pps;
    fixed_voltage_control_state_t last_error;
} fixed_voltage_control_status_t;

typedef struct {
    PD_protocol_t protocol;
    fixed_voltage_control_status_t status;
    FUSB302_dev_t *phy;
    uint32_t started_at, state_at, retry_at;
    uint16_t last_rx;
    uint8_t retries;
    bool have_rx, awaiting_crc;
    bool soft_reset_tried, waiting_soft_reset;
    uint16_t target_voltage, contract_voltage;
    uint16_t pps_voltage;
    uint8_t pps_current;
    uint32_t contract_at;
    uint16_t contract_pps_voltage;
    uint8_t contract_pps_current;
} fixed_voltage_control_t;

void fixed_voltage_control_start(fixed_voltage_control_t *p, FUSB302_dev_t *phy, uint32_t now);
void fixed_voltage_control_start_5v(fixed_voltage_control_t *p, FUSB302_dev_t *phy, uint32_t now);
bool fixed_voltage_control_request_9v(fixed_voltage_control_t *p, uint32_t now);
bool fixed_voltage_control_request_fixed(fixed_voltage_control_t *p, uint16_t voltage, uint32_t now);
bool fixed_voltage_control_request_pps(fixed_voltage_control_t *p, uint16_t voltage, uint8_t current, uint32_t now);
bool fixed_voltage_control_power_supported(const fixed_voltage_control_status_t *status,
                              uint16_t mv, uint16_t ma, bool pps);
void fixed_voltage_control_receive(fixed_voltage_control_t *p, uint16_t header, uint32_t *objects, uint32_t now);
void fixed_voltage_control_events(fixed_voltage_control_t *p, FUSB302_event_t events, uint32_t now);
void fixed_voltage_control_tick(fixed_voltage_control_t *p, uint32_t now);
bool fixed_voltage_control_active(const fixed_voltage_control_t *p);

#endif
