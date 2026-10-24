#ifndef TEST_GATE_H
#define TEST_GATE_H

#include <stdint.h>

#define LOW_CUR  1U
#define MID_CUR  2U
#define HIGH_CUR 3U

#define GATE_MODE_AUTO 0U
#define GATE_MODE_LOW  LOW_CUR
#define GATE_MODE_MID  MID_CUR
#define GATE_MODE_HIGH HIGH_CUR

void flow_route_selection(uint8_t selection);
uint8_t Gate_get_status(void);
void Gate_Set_Mode(uint8_t mode);
uint8_t Gate_Get_Mode(void);

#endif
