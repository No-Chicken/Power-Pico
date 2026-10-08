#ifndef PICO_DIAG_H
#define PICO_DIAG_H

#include <stdbool.h>
#include <stdint.h>

enum {
    DIAG_ARM = 1, DIAG_CC_OFF, DIAG_PHY_INIT, DIAG_CC_ON,
    DIAG_STATE, DIAG_TX, DIAG_RX, DIAG_EVENT, DIAG_RELEASE,
    DIAG_VOLTAGE, DIAG_FAULT, DIAG_DONE, DIAG_STORAGE_ERROR,
    DIAG_PROBE_RELEASE_ATTACHED, DIAG_PROBE_STANDBY,
    DIAG_PROBE_PHY_STATUS, DIAG_PROBE_RESYNC,
    DIAG_PD_TARGET, DIAG_PD_BOOT, DIAG_PD_HANDOFF,
    DIAG_PD_BOOT_PROGRESS,
    DIAG_PD_CC_LEVELS, DIAG_PD_PHY_CONFIG,
    DIAG_PD_VOLTAGE_CHECK,
    DIAG_PDO_BASE = 32
};

void pico_diag_early_init(void);
void pico_diag_storage_init(void);
void pico_diag_arm(void);
void pico_diag_arm_ram(void);
void pico_diag_log(uint32_t stage, uint32_t arg);
void pico_diag_fault(uint32_t exception);
void pico_diag_request(void);
void pico_diag_boot_request(void);
/* Called only by the ADC/USB sender; true reserves this iteration for a dump. */
bool pico_diag_usb_service(void);

#endif
