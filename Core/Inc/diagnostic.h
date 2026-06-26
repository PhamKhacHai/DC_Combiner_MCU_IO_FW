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

#ifdef __cplusplus
}
#endif

#endif /* DIAGNOSTIC_H */
