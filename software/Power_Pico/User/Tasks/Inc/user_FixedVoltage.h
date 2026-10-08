#ifndef USER_fixed_voltage_control_H
#define USER_fixed_voltage_control_H

#include "fixed_voltage_control.h"

void user_fixed_voltage_control_request(bool start);
void user_fixed_voltage_control_status(fixed_voltage_control_status_t *status);
void user_pd_boot_init(void);
void user_pd_power_status(fixed_voltage_control_status_t *status);
bool user_pd_power_request(uint16_t millivolts, uint16_t milliamps, bool pps);

#endif
