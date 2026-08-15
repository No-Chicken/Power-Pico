#ifndef __USER_CMDSTRATEGY_H__
#define __USER_CMDSTRATEGY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CMD_PROTOCOL_VERSION     1U
#define CMD_CAL_GET              0x10U
#define CMD_RANGE_GET            0x11U
#define CMD_RANGE_SET            0x12U
#define CMD_CAL_SET              0x13U
#define CMD_CAL_RESET            0x14U
#define CMD_FW_UPDATE            0x15U

void CmdStrategy_Init(void);
void CmdStrategy_ProcessRx(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
