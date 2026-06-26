#include "diagnostic.h"

#include <stddef.h>

static uint8_t heartbeat_sequence;

void Diagnostic_Init(void)
{
  heartbeat_sequence = 0U;
}

void Diagnostic_BuildHeartbeatPayload(const diagnostic_heartbeat_input_t *input,
                                      uint8_t data[CONFIG_CAN_HEARTBEAT_DLC])
{
  uint8_t index;

  if (data == NULL)
  {
    return;
  }

  for (index = 0U; index < CONFIG_CAN_HEARTBEAT_DLC; index++)
  {
    data[index] = 0U;
  }

  if (input == NULL)
  {
    return;
  }

  data[0] = input->fw_major;
  data[1] = input->fw_minor;
  data[2] = input->fw_patch;
  data[3] = input->node_id;
  data[4] = input->global_status;
  data[5] = (uint8_t)(input->valid_command_rx_count & 0xFFU);
  data[6] = (uint8_t)(input->can_tx_fail_count & 0xFFU);
  data[7] = heartbeat_sequence;
}

void Diagnostic_OnHeartbeatTxSuccess(void)
{
  heartbeat_sequence++;
}

uint8_t Diagnostic_GetHeartbeatSequence(void)
{
  return heartbeat_sequence;
}
