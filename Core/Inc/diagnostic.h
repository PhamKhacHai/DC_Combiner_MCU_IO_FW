#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "bsp_gpio.h"
#include "config.h"

typedef struct
{
  uint8_t fw_major;
  uint8_t fw_minor;
  uint8_t fw_patch;
  bsp_gpio_node_id_t node_id;
  uint8_t global_status;
  uint32_t valid_command_rx_count;
  uint32_t can_tx_fail_count;
} diagnostic_heartbeat_input_t;

void Diagnostic_Init(void);
void Diagnostic_BuildHeartbeatPayload(const diagnostic_heartbeat_input_t *input,
                                      uint8_t data[CONFIG_CAN_HEARTBEAT_DLC]);
void Diagnostic_OnHeartbeatTxSuccess(void);
uint8_t Diagnostic_GetHeartbeatSequence(void);

void Diagnostic_UpdateFeedbackState(uint8_t on_mask, uint8_t off_mask, uint8_t mismatch_mask);
void Diagnostic_RecordGroupFault(bsp_gpio_group_t group, uint8_t fault_code);
void Diagnostic_RecordFaultClear(bsp_gpio_group_t group);
void Diagnostic_RecordCanCommandRx(void);
void Diagnostic_RecordCanTxSuccess(void);
void Diagnostic_RecordCanRxError(void);
void Diagnostic_RecordCanTxError(void);
void Diagnostic_RecordCanTimeout(void);
void Diagnostic_RecordFailSafeEnter(void);
void Diagnostic_RecordDiagRequest(void);
void Diagnostic_RecordDiagError(void);

bool Diagnostic_GetCounter(uint8_t counter_id, uint8_t group_index, uint32_t *value);
void Diagnostic_ResetAllCounters(void);

void Diagnostic_BuildReadCounterResponse(uint8_t counter_id,
                                         uint8_t group_index,
                                         uint8_t data[CONFIG_CAN_DIAG_RESPONSE_DLC]);
void Diagnostic_BuildResetAllCountersResponse(uint8_t status,
                                              uint8_t data[CONFIG_CAN_DIAG_RESPONSE_DLC]);
void Diagnostic_BuildErrorResponse(uint8_t command,
                                   uint8_t counter_id,
                                   uint8_t group_index,
                                   uint8_t status,
                                   uint8_t data[CONFIG_CAN_DIAG_RESPONSE_DLC]);

#ifdef __cplusplus
}
#endif

#endif /* DIAGNOSTIC_H */
