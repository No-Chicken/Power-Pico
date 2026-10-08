#include "fixed_voltage_control.h"
#include <string.h>
#include "pico_diag.h"

static void state(fixed_voltage_control_t *p, fixed_voltage_control_state_t value, uint32_t now)
{
    p->status.state = value;
    p->state_at = now;
    pico_diag_log(DIAG_STATE, value);
    if (value > fixed_voltage_control_READY) {
        p->status.last_error = value;
        /* Cached values only: status reads would acknowledge PHY interrupts. */
        pico_diag_log(DIAG_PROBE_PHY_STATUS,
            (uint32_t)p->phy->reg_status[0] |
            ((uint32_t)p->phy->reg_status[4] << 8) |
            ((uint32_t)p->phy->interrupta << 16) |
            ((uint32_t)p->phy->interruptb << 24));
    }
}

bool fixed_voltage_control_active(const fixed_voltage_control_t *p)
{
    return p->status.state >= fixed_voltage_control_ATTACH && p->status.state <= fixed_voltage_control_READY;
}

bool fixed_voltage_control_power_supported(const fixed_voltage_control_status_t *status,
                              uint16_t mv, uint16_t ma, bool pps)
{
    if (mv < 5000 || mv > 20000 || status->count > 7 ||
        (pps && (status->source_revision != 2 || !ma || ma > 3000 ||
                 mv % 20 || ma % 50))) return false;
    PD_protocol_t protocol = {0};
    protocol.power_data_obj_count = status->count;
    memcpy(protocol.power_data_obj, status->pdo, sizeof(protocol.power_data_obj));
    for (unsigned i = 0; i < protocol.power_data_obj_count; ++i) {
        PD_power_info_t info;
        if (!PD_protocol_get_power_info(&protocol, i, &info) || !info.max_i) continue;
        if (!pps && info.type == PD_PDO_TYPE_FIXED_SUPPLY && info.max_v * 50U == mv)
            return true;
        if (pps && info.type == PD_PDO_TYPE_AUGMENTED_PDO &&
            mv >= info.min_v * 50U && mv <= info.max_v * 50U && ma <= info.max_i * 10U)
            return true;
    }
    return false;
}

static bool can_request(const fixed_voltage_control_t *p)
{
    return !p->awaiting_crc && (p->status.state == fixed_voltage_control_READY ||
        (p->status.contract_mv && (p->status.state == fixed_voltage_control_REJECTED ||
         p->status.state == fixed_voltage_control_WAIT || p->status.state == fixed_voltage_control_TIMEOUT ||
         p->status.state == fixed_voltage_control_NO_9V)));
}

static bool send(fixed_voltage_control_t *p, uint16_t header, uint32_t *objects, uint32_t now)
{
    pico_diag_log(DIAG_TX, header);
    if (FUSB302_tx_sop(p->phy, header, objects) != FUSB302_SUCCESS) {
        state(p, fixed_voltage_control_IO_ERROR, now);
        return false;
    }
    p->awaiting_crc = true;
    return true;
}

void fixed_voltage_control_start(fixed_voltage_control_t *p, FUSB302_dev_t *phy, uint32_t now)
{
    memset(p, 0, sizeof(*p));
    p->phy = phy;
    p->started_at = p->retry_at = now;
    PD_protocol_init(&p->protocol);
    p->protocol.fixed_9v_probe = true;
    p->target_voltage = p->protocol.fixed_probe_voltage = PD_V(9.0);
    p->protocol.fixed_probe_revision = 1;
    state(p, fixed_voltage_control_ATTACH, now);
}

void fixed_voltage_control_start_5v(fixed_voltage_control_t *p, FUSB302_dev_t *phy, uint32_t now)
{
    fixed_voltage_control_start(p, phy, now);
    p->target_voltage = p->protocol.fixed_probe_voltage = PD_V(5.0);
    p->protocol.fixed_probe_revision = 2;
    pico_diag_log(DIAG_PD_TARGET, 5000);
}

static bool select_voltage(fixed_voltage_control_t *p, uint32_t now)
{
    PD_power_info_t info;
    p->status.target_mv = p->pps_voltage ? p->pps_voltage * 20U : p->target_voltage * 50U;
    p->status.pps = p->pps_voltage != 0;
    for (uint8_t index = 0; index < p->protocol.power_data_obj_count; ++index) {
        if (p->pps_voltage) {
            if (p->status.source_revision != 2 ||
                !PD_protocol_get_power_info(&p->protocol, index, &info) ||
                info.type != PD_PDO_TYPE_AUGMENTED_PDO ||
                p->pps_voltage * 20U < info.min_v * 50U ||
                p->pps_voltage * 20U > info.max_v * 50U ||
                p->pps_current * 5U > info.max_i) continue;
            PD_protocol_select_power(&p->protocol, index);
            p->protocol.fixed_9v_probe = false;
            p->protocol.PPS_voltage = p->pps_voltage;
            p->protocol.PPS_current = p->pps_current;
            p->status.source_current_ma = info.max_i * 10U;
            p->status.request_current_ma = p->pps_current * 50U;
            state(p, fixed_voltage_control_ACCEPT, now);
            return true;
        }
        if (PD_protocol_get_power_info(&p->protocol, index, &info) &&
            info.type == PD_PDO_TYPE_FIXED_SUPPLY && info.max_v == p->target_voltage &&
            info.max_i > 0) {
            PD_protocol_select_power(&p->protocol, index);
            p->status.source_current_ma = info.max_i * 10U;
            p->status.request_current_ma = info.max_i < 100 ? info.max_i * 10U : 1000;
            state(p, fixed_voltage_control_ACCEPT, now);
            return true;
        }
    }
    p->status.source_current_ma = p->status.request_current_ma = 0;
    state(p, fixed_voltage_control_NO_9V, now);
    return false;
}

bool fixed_voltage_control_request_9v(fixed_voltage_control_t *p, uint32_t now)
{
    return fixed_voltage_control_request_fixed(p, PD_V(9.0), now);
}

bool fixed_voltage_control_request_fixed(fixed_voltage_control_t *p, uint16_t voltage, uint32_t now)
{
    if (!can_request(p)) return false;
    if (voltage < PD_V(5.0) || voltage > PD_V(20.0)) return false;
    p->pps_voltage = p->pps_current = 0;
    p->status.last_error = fixed_voltage_control_IDLE;
    p->protocol.fixed_9v_probe = true;
    p->protocol.PPS_voltage = p->protocol.PPS_current = 0;
    p->target_voltage = p->protocol.fixed_probe_voltage = voltage;
    p->started_at = now;
    p->soft_reset_tried = p->waiting_soft_reset = false;
    pico_diag_log(DIAG_PD_TARGET, voltage * 50U);
    if (select_voltage(p, now)) {
        uint16_t header = 0;
        uint32_t objects[7] = {0};
        PD_protocol_create_request(&p->protocol, &header, objects);
        send(p, header, objects, now);
    }
    return true;
}

bool fixed_voltage_control_request_pps(fixed_voltage_control_t *p, uint16_t voltage, uint8_t current, uint32_t now)
{
    if (!can_request(p) ||
        voltage < PPS_V(5.0) || voltage > PPS_V(20.0) ||
        !current || current > PPS_A(3.0)) return false;
    if (!fixed_voltage_control_power_supported(&p->status, voltage * 20U, current * 50U, true))
        return false;
    p->status.last_error = fixed_voltage_control_IDLE;
    p->pps_voltage = voltage;
    p->pps_current = current;
    p->started_at = now;
    p->soft_reset_tried = p->waiting_soft_reset = false;
    pico_diag_log(DIAG_PD_TARGET, voltage * 20U);
    if (select_voltage(p, now)) {
        uint16_t header = 0;
        uint32_t objects[7] = {0};
        PD_protocol_create_request(&p->protocol, &header, objects);
        send(p, header, objects, now);
    }
    return true;
}

static void resync(fixed_voltage_control_t *p, uint32_t now)
{
    p->soft_reset_tried = true;
    pico_diag_log(DIAG_PROBE_RESYNC, 0);
    if (FUSB302_pd_reset(p->phy) != FUSB302_SUCCESS) {
        state(p, fixed_voltage_control_IO_ERROR, now);
        return;
    }
    uint16_t header;
    PD_protocol_create_soft_reset(&p->protocol, &header);
    p->have_rx = p->awaiting_crc = false;
    p->waiting_soft_reset = true;
    p->retry_at = now;
    send(p, header, NULL, now);
}

void fixed_voltage_control_receive(fixed_voltage_control_t *p, uint16_t header, uint32_t *objects, uint32_t now)
{
    pico_diag_log(DIAG_RX, header);
    unsigned type = header & 31U;
    unsigned count = (header >> 12) & 7U;
    unsigned revision = (header >> 6) & 3U;
    if ((header & 0x8000U) || revision == 0 || revision == 3) return;
    PD_protocol_event_t ignored = 0;
    if (count == 0 && type == 1) {
        /* A delayed/repeated GoodCRC must not advance a new session's TX ID. */
        if (p->awaiting_crc &&
            ((header >> 9) & 7U) == p->protocol.message_id) {
            PD_protocol_handle_msg(&p->protocol, header, objects, &ignored);
            p->awaiting_crc = false;
        }
        return;
    }
    if (!fixed_voltage_control_active(p) || p->status.state == fixed_voltage_control_ATTACH) return;
    /* Do not treat Soft_Reset's Accept as acceptance of a 9V request, or
     * accept stale capabilities before the reset handshake completes. */
    if (p->waiting_soft_reset) {
        if (count != 0) return;
        if (type == 3) {
            p->waiting_soft_reset = false;
            p->awaiting_crc = false;
            p->protocol.message_id = 1;
            p->have_rx = true;
            p->last_rx = header;
            p->retry_at = now;
            p->retries = 0;
            pico_diag_log(DIAG_PROBE_RESYNC, 1);
            state(p, fixed_voltage_control_CAPS, now);
            return;
        }
        if (type == 4 || type == 12) {
            state(p, type == 4 ? fixed_voltage_control_REJECTED : fixed_voltage_control_WAIT, now);
            return;
        }
        if (type != 13) return;
    }
    if (!(count == 0 && type == 13) && p->have_rx &&
        ((header >> 9) & 7U) == ((p->last_rx >> 9) & 7U)) return;
    p->have_rx = true;
    p->last_rx = header;
    if (count == 0) {
        if (type == 4 || type == 12) {
            if (p->status.state == fixed_voltage_control_ACCEPT || p->status.state == fixed_voltage_control_POWER)
                state(p, type == 4 ? fixed_voltage_control_REJECTED : fixed_voltage_control_WAIT, now);
            return;
        }
        if (type == 3) {
            if (p->status.state == fixed_voltage_control_ACCEPT) state(p, fixed_voltage_control_POWER, now);
            return;
        }
        if (type == 6) {
            if (p->status.state == fixed_voltage_control_POWER) {
                p->contract_voltage = p->target_voltage;
                p->status.contract_mv = p->status.target_mv;
                p->status.contract_pps = p->pps_voltage != 0;
                p->contract_pps_voltage = p->pps_voltage;
                p->contract_pps_current = p->pps_current;
                p->contract_at = now;
                state(p, fixed_voltage_control_READY, now);
            }
            return;
        }
        if (type == 13) {
            p->started_at = now;
            PD_protocol_reset(&p->protocol);
            p->have_rx = p->awaiting_crc = false;
            p->waiting_soft_reset = false;
            p->contract_voltage = 0;
            p->status.contract_mv = 0;
            p->status.contract_pps = false;
            p->contract_pps_voltage = p->contract_pps_current = 0;
            p->status.count = 0;
            p->status.source_current_ma = p->status.request_current_ma = 0;
            p->retry_at = now;
            p->retries = 0;
            state(p, fixed_voltage_control_CAPS, now);
        } else if (type != 8 && type != 9 && type != 11) {
            /* Neither power-control page enters a role swap. */
            return;
        }
    } else if (type != 1) {
        return;
    }
    PD_protocol_handle_msg(&p->protocol, header, objects, &ignored);
    if (count && type == 1) {
        p->started_at = now;
        p->status.count = (uint8_t)count;
        p->status.source_revision = (uint8_t)revision;
        if (p->target_voltage == PD_V(5.0)) {
            p->protocol.fixed_probe_revision = (uint8_t)revision;
        }
        memset(p->status.pdo, 0, sizeof(p->status.pdo));
        memcpy(p->status.pdo, objects, count * sizeof(*objects));
        for (unsigned i = 0; i < count; ++i)
            pico_diag_log(DIAG_PDO_BASE + i, objects[i]);
        PD_power_info_t info;
        /* vSafe5V must be the first fixed PDO; never use a fallback selection. */
        if (!PD_protocol_get_power_info(&p->protocol, 0, &info) ||
            info.type != PD_PDO_TYPE_FIXED_SUPPLY || info.max_v != PD_V(5.0) ||
            info.max_i == 0) {
            state(p, fixed_voltage_control_INVALID_CAPS, now);
            return;
        }
        if (!select_voltage(p, now)) return;
    }
    uint16_t response;
    uint32_t payload[7] = {0};
    if (PD_protocol_respond(&p->protocol, &response, payload)) {
        send(p, response, payload, now);
    }
}

void fixed_voltage_control_events(fixed_voltage_control_t *p, FUSB302_event_t events, uint32_t now)
{
    if (events) pico_diag_log(DIAG_EVENT, events);
    if (events & FUSB302_EVENT_DETACHED) {
        p->contract_pps_voltage = p->contract_pps_current = 0;
        p->status.contract_mv = p->contract_voltage = 0;
        p->status.contract_pps = false;
        state(p, fixed_voltage_control_DISCONNECTED, now);
        return;
    }
    if (events & FUSB302_EVENT_HARD_RESET) {
        p->contract_pps_voltage = p->contract_pps_current = 0;
        p->status.contract_mv = p->contract_voltage = 0;
        p->status.contract_pps = false;
        state(p, fixed_voltage_control_RESET, now);
        return;
    }
    if (events & FUSB302_EVENT_ATTACHED) {
        pico_diag_log(DIAG_PD_CC_LEVELS,
            (uint32_t)p->phy->cc1 | ((uint32_t)p->phy->cc2 << 8));
        pico_diag_log(DIAG_PD_PHY_CONFIG,
            (uint32_t)p->phy->reg_control[1] |
            ((uint32_t)p->phy->reg_control[2] << 8) |
            ((uint32_t)p->phy->reg_control[10] << 16));
        if ((p->phy->cc1 != 0) == (p->phy->cc2 != 0)) return;
        p->status.cc = p->phy->cc1 ? 1 : 2;
        p->retry_at = now;
        state(p, fixed_voltage_control_CAPS, now);
    }
    if (events & FUSB302_EVENT_RX_SOP) {
        uint16_t header;
        uint32_t objects[7] = {0};
        if (FUSB302_get_message(p->phy, &header, objects) != FUSB302_SUCCESS) {
            state(p, fixed_voltage_control_IO_ERROR, now);
            return;
        }
        fixed_voltage_control_receive(p, header, objects, now);
    }
}

void fixed_voltage_control_tick(fixed_voltage_control_t *p, uint32_t now)
{
    if (p->contract_pps_voltage && can_request(p) &&
        now - p->contract_at >= 5000U) {
        fixed_voltage_control_state_t error = p->status.last_error;
        p->contract_at = now;
        fixed_voltage_control_request_pps(p, p->contract_pps_voltage, p->contract_pps_current, now);
        if (error != fixed_voltage_control_IDLE) p->status.last_error = error;
        return;
    }
    if (!fixed_voltage_control_active(p)) return;
    if (p->status.state == fixed_voltage_control_READY) return;
    if (now - p->started_at >= 5000U ||
        ((p->status.state == fixed_voltage_control_ACCEPT || p->status.state == fixed_voltage_control_POWER) &&
         now - p->state_at >= 600U)) {
        state(p, fixed_voltage_control_TIMEOUT, now);
        return;
    }
    if (p->waiting_soft_reset) {
        if (now - p->retry_at >= 600U) state(p, fixed_voltage_control_TIMEOUT, now);
        return;
    }
    if (p->status.state == fixed_voltage_control_CAPS && now - p->retry_at >= 500U) {
        if (p->awaiting_crc || p->retries >= 3) {
            /* One bounded protocol resync, never a CC detach or hard reset. */
            if (!p->soft_reset_tried) resync(p, now);
            else state(p, fixed_voltage_control_TIMEOUT, now);
            return;
        }
        uint16_t header;
        PD_protocol_create_get_src_cap(&p->protocol, &header);
        send(p, header, NULL, now);
        ++p->retries;
        p->retry_at = now;
    }
}
