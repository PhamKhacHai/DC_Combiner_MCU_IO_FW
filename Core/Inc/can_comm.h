#ifndef CAN_COMM_H
#define CAN_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "bsp_gpio.h"

bool CanComm_Init(uint32_t now_ms);
void CanComm_Task(uint32_t now_ms);
bool CanComm_SendStatus(uint32_t now_ms);

bsp_gpio_node_id_t CanComm_GetNodeId(void);
uint32_t CanComm_GetLastRxTimeMs(void);
bool CanComm_HasPendingCommand(void);

uint32_t CanComm_GetRxCount(void);
uint32_t CanComm_GetTxCount(void);
uint32_t CanComm_GetRxErrorCount(void);
uint32_t CanComm_GetTxErrorCount(void);
bool CanComm_IsStarted(void);
bool CanComm_IsFailSafeActive(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_COMM_H */
