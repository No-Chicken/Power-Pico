#ifndef __USER_PDUFPTASK_H__
#define __USER_PDUFPTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "user_TasksInit.h"

// Commands sent from the UI layer to the PD UFP task.
typedef enum {
    PD_CMD_START = 0,
    PD_CMD_STOP,
    PD_CMD_SET_PPS,       // PPS adjustment
    PD_CMD_SET_PD_FIXED   // Fixed-PD voltage selection
} PD_command_t;

// Events sent from the PD UFP task to the UI layer.
typedef enum {
    PD_EVT_PPS_READY = 0, // PPS negotiation succeeded
    PD_EVT_FIXED_READY,   // Fixed-PD negotiation succeeded
    PD_EVT_PPS_FAILED,
} PD_handle_event_t;

enum {
    PD_FIXED_VOL_LEVEL_5V = 0,
    PD_FIXED_VOL_LEVEL_9V,
    PD_FIXED_VOL_LEVEL_12V,
    PD_FIXED_VOL_LEVEL_15V,
    PD_FIXED_VOL_LEVEL_20V
};
typedef uint8_t PD_FIXED_VOL_LEVEL;

// Command payload sent from the UI layer to the PD UFP task.
typedef struct
{
    PD_command_t event;
    float pps_set_voltage;
    float pps_set_current;
    PD_FIXED_VOL_LEVEL pd_fixed_level;
} PD_command_msg_t;


void PDUFPTask(void *argument);


#ifdef __cplusplus
}
#endif

#endif

