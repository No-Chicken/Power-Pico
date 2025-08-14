#ifndef __GATE_H__
#define __GATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// EN1
#define EN1_PORT    GPIOB
#define EN1_PIN     GPIO_PIN_3

// EN2
#define EN2_PORT    GPIOA
#define EN2_PIN     GPIO_PIN_15

// Flow route selection
#define HIGH_CUR 2
#define MID_CUR 1
#define LOW_CUR 0

// Resistance values in ohms
#define HIGH_CUR_RES 0.005    // 5 m ohm
#define MID_CUR_RES 0.5     // 500 m ohm
#define LOW_CUR_RES 50.0     // 50 ohm

void Gate_Port_Init(void);
void flow_route_selection(uint8_t selection);
uint8_t Gate_get_status(void);

#ifdef __cplusplus
}
#endif
#endif

