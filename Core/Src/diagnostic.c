#include "diagnostic.h"

#include "contactor.h"

#include <stddef.h>

typedef enum
{
  DIAGNOSTIC_FEEDBACK_UNKNOWN = 0,
  DIAGNOSTIC_FEEDBACK_OFF,
  DIAGNOSTIC_FEEDBACK_ON,
  DIAGNOSTIC_FEEDBACK_MISMATCH
} diagnostic_feedback_state_t;

typedef struct
{
  uint32_t contactor_close_count;
  uint32_t contactor_open_count;
  uint32_t feedback_mismatch_count;
  uint32_t group_fault_count;
  uint32_t on_timeout_fault_count;
  uint32_t unexpected_off_fault_count;
  uint32_t fault_clear_count;
  diagnostic_feedback_state_t last_feedback_state;
} diagnostic_group_counters_t;

typedef struct
{
  uint32_t can_command_rx_count;
  uint32_t can_tx_count;
  uint32_t can_rx_error_count;
  uint32_t can_tx_error_count;
  uint32_t can_timeout_count;
  uint32_t failsafe_enter_count;
  uint32_t diag_request_count;
  uint32_t diag_error_count;
} diagnostic_global_counters_t;

static diagnostic_group_counters_t group_counters[CONFIG_GROUP_COUNT];
static diagnostic_global_counters_t global_counters;
static uint8_t heartbeat_sequence;

static void Diagnostic_ClearData(uint8_t *data, uint8_t length)
{
  uint8_t index;

  if (data == NULL)
  {
    return;
  }

  for (index = 0U; index < length; index++)
  {
    data[index] = 0U;
  }
}

static void Diagnostic_IncrementSaturating(uint32_t *counter)
{
  if ((counter != NULL) && (*counter < UINT32_MAX))
  {
    (*counter)++;
  }
}

static bool Diagnostic_IsValidGroup(uint8_t group)
{
  return group < CONFIG_GROUP_COUNT;
}

static bool Diagnostic_IsGlobalGroup(uint8_t group)
{
  return group == CONFIG_CAN_DIAG_GROUP_GLOBAL;
}

static diagnostic_feedback_state_t Diagnostic_DecodeFeedbackState(uint8_t on_mask,
                                                                  uint8_t off_mask,
                                                                  uint8_t mismatch_mask,
                                                                  uint8_t group)
{
  uint8_t group_bit = (uint8_t)(1U << group);

  if ((on_mask & group_bit) != 0U)
  {
    return DIAGNOSTIC_FEEDBACK_ON;
  }

  if ((off_mask & group_bit) != 0U)
  {
    return DIAGNOSTIC_FEEDBACK_OFF;
  }

  if ((mismatch_mask & group_bit) != 0U)
  {
    return DIAGNOSTIC_FEEDBACK_MISMATCH;
  }

  return DIAGNOSTIC_FEEDBACK_UNKNOWN;
}

static void Diagnostic_PutU32Le(uint8_t *data, uint8_t index, uint32_t value)
{
  if (data == NULL)
  {
    return;
  }

  data[index] = (uint8_t)(value & 0xFFU);
  data[index + 1U] = (uint8_t)((value >> 8) & 0xFFU);
  data[index + 2U] = (uint8_t)((value >> 16) & 0xFFU);
  data[index + 3U] = (uint8_t)((value >> 24) & 0xFFU);
}

void Diagnostic_Init(void)
{
  heartbeat_sequence = 0U;
  Diagnostic_ResetAllCounters();
}

void Diagnostic_BuildHeartbeatPayload(const diagnostic_heartbeat_input_t *input,
                                      uint8_t data[CONFIG_CAN_HEARTBEAT_DLC])
{
  Diagnostic_ClearData(data, CONFIG_CAN_HEARTBEAT_DLC);

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

void Diagnostic_UpdateFeedbackState(uint8_t on_mask, uint8_t off_mask, uint8_t mismatch_mask)
{
  uint8_t group;

  for (group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    diagnostic_feedback_state_t current_state;
    diagnostic_feedback_state_t previous_state;

    current_state = Diagnostic_DecodeFeedbackState(on_mask, off_mask, mismatch_mask, group);
    previous_state = group_counters[group].last_feedback_state;

    if (previous_state == DIAGNOSTIC_FEEDBACK_UNKNOWN)
    {
      group_counters[group].last_feedback_state = current_state;
      continue;
    }

    if ((current_state == DIAGNOSTIC_FEEDBACK_ON) &&
        (previous_state != DIAGNOSTIC_FEEDBACK_ON))
    {
      Diagnostic_IncrementSaturating(&group_counters[group].contactor_close_count);
    }

    if ((previous_state == DIAGNOSTIC_FEEDBACK_ON) &&
        (current_state == DIAGNOSTIC_FEEDBACK_OFF))
    {
      Diagnostic_IncrementSaturating(&group_counters[group].contactor_open_count);
    }

    if ((current_state == DIAGNOSTIC_FEEDBACK_MISMATCH) &&
        (previous_state != DIAGNOSTIC_FEEDBACK_MISMATCH))
    {
      Diagnostic_IncrementSaturating(&group_counters[group].feedback_mismatch_count);
    }

    group_counters[group].last_feedback_state = current_state;
  }
}

void Diagnostic_RecordGroupFault(bsp_gpio_group_t group, uint8_t fault_code)
{
  if (!Diagnostic_IsValidGroup(group))
  {
    return;
  }

  Diagnostic_IncrementSaturating(&group_counters[group].group_fault_count);

  if (fault_code == (uint8_t)CONTACTOR_FAULT_ON_TIMEOUT)
  {
    Diagnostic_IncrementSaturating(&group_counters[group].on_timeout_fault_count);
  }
  else if (fault_code == (uint8_t)CONTACTOR_FAULT_UNEXPECTED_OFF)
  {
    Diagnostic_IncrementSaturating(&group_counters[group].unexpected_off_fault_count);
  }
}

void Diagnostic_RecordFaultClear(bsp_gpio_group_t group)
{
  if (!Diagnostic_IsValidGroup(group))
  {
    return;
  }

  Diagnostic_IncrementSaturating(&group_counters[group].fault_clear_count);
}

void Diagnostic_RecordCanCommandRx(void)
{
  Diagnostic_IncrementSaturating(&global_counters.can_command_rx_count);
}

void Diagnostic_RecordCanTxSuccess(void)
{
  Diagnostic_IncrementSaturating(&global_counters.can_tx_count);
}

void Diagnostic_RecordCanRxError(void)
{
  Diagnostic_IncrementSaturating(&global_counters.can_rx_error_count);
}

void Diagnostic_RecordCanTxError(void)
{
  Diagnostic_IncrementSaturating(&global_counters.can_tx_error_count);
}

void Diagnostic_RecordCanTimeout(void)
{
  Diagnostic_IncrementSaturating(&global_counters.can_timeout_count);
}

void Diagnostic_RecordFailSafeEnter(void)
{
  Diagnostic_IncrementSaturating(&global_counters.failsafe_enter_count);
}

void Diagnostic_RecordDiagRequest(void)
{
  Diagnostic_IncrementSaturating(&global_counters.diag_request_count);
}

void Diagnostic_RecordDiagError(void)
{
  Diagnostic_IncrementSaturating(&global_counters.diag_error_count);
}

bool Diagnostic_GetCounter(uint8_t counter_id, uint8_t group_index, uint32_t *value)
{
  if (value == NULL)
  {
    return false;
  }

  *value = 0U;

  switch (counter_id)
  {
    case CONFIG_CAN_DIAG_COUNTER_CONTACTOR_CLOSE:
      if (!Diagnostic_IsValidGroup(group_index))
      {
        return false;
      }
      *value = group_counters[group_index].contactor_close_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_CONTACTOR_OPEN:
      if (!Diagnostic_IsValidGroup(group_index))
      {
        return false;
      }
      *value = group_counters[group_index].contactor_open_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_FEEDBACK_MISMATCH:
      if (!Diagnostic_IsValidGroup(group_index))
      {
        return false;
      }
      *value = group_counters[group_index].feedback_mismatch_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_GROUP_FAULT:
      if (!Diagnostic_IsValidGroup(group_index))
      {
        return false;
      }
      *value = group_counters[group_index].group_fault_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_ON_TIMEOUT_FAULT:
      if (!Diagnostic_IsValidGroup(group_index))
      {
        return false;
      }
      *value = group_counters[group_index].on_timeout_fault_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_UNEXPECTED_OFF_FAULT:
      if (!Diagnostic_IsValidGroup(group_index))
      {
        return false;
      }
      *value = group_counters[group_index].unexpected_off_fault_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_FAULT_CLEAR:
      if (!Diagnostic_IsValidGroup(group_index))
      {
        return false;
      }
      *value = group_counters[group_index].fault_clear_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_CAN_COMMAND_RX:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.can_command_rx_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_CAN_TX:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.can_tx_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_CAN_RX_ERROR:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.can_rx_error_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_CAN_TX_ERROR:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.can_tx_error_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_CAN_TIMEOUT:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.can_timeout_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_FAILSAFE_ENTER:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.failsafe_enter_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_DIAG_REQUEST:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.diag_request_count;
      return true;

    case CONFIG_CAN_DIAG_COUNTER_DIAG_ERROR:
      if (!Diagnostic_IsGlobalGroup(group_index))
      {
        return false;
      }
      *value = global_counters.diag_error_count;
      return true;

    default:
      return false;
  }
}

void Diagnostic_ResetAllCounters(void)
{
  uint8_t group;

  global_counters.can_command_rx_count = 0U;
  global_counters.can_tx_count = 0U;
  global_counters.can_rx_error_count = 0U;
  global_counters.can_tx_error_count = 0U;
  global_counters.can_timeout_count = 0U;
  global_counters.failsafe_enter_count = 0U;
  global_counters.diag_request_count = 0U;
  global_counters.diag_error_count = 0U;

  for (group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    group_counters[group].contactor_close_count = 0U;
    group_counters[group].contactor_open_count = 0U;
    group_counters[group].feedback_mismatch_count = 0U;
    group_counters[group].group_fault_count = 0U;
    group_counters[group].on_timeout_fault_count = 0U;
    group_counters[group].unexpected_off_fault_count = 0U;
    group_counters[group].fault_clear_count = 0U;
    group_counters[group].last_feedback_state = DIAGNOSTIC_FEEDBACK_UNKNOWN;
  }
}

void Diagnostic_BuildReadCounterResponse(uint8_t counter_id,
                                         uint8_t group_index,
                                         uint8_t data[CONFIG_CAN_DIAG_RESPONSE_DLC])
{
  uint32_t value = 0U;
  uint8_t status = CONFIG_CAN_DIAG_STATUS_OK;

  Diagnostic_ClearData(data, CONFIG_CAN_DIAG_RESPONSE_DLC);

  if (data == NULL)
  {
    return;
  }

  if (!Diagnostic_GetCounter(counter_id, group_index, &value))
  {
    status = CONFIG_CAN_DIAG_STATUS_INVALID_COUNTER_ID;

    if ((counter_id >= CONFIG_CAN_DIAG_COUNTER_CONTACTOR_CLOSE) &&
        (counter_id <= CONFIG_CAN_DIAG_COUNTER_FAULT_CLEAR) &&
        !Diagnostic_IsValidGroup(group_index))
    {
      status = CONFIG_CAN_DIAG_STATUS_INVALID_GROUP;
    }
    else if ((counter_id >= CONFIG_CAN_DIAG_COUNTER_CAN_COMMAND_RX) &&
             (counter_id <= CONFIG_CAN_DIAG_COUNTER_DIAG_ERROR) &&
             !Diagnostic_IsGlobalGroup(group_index))
    {
      status = CONFIG_CAN_DIAG_STATUS_INVALID_GROUP;
    }
  }

  data[0] = CONFIG_CAN_DIAG_RESPONSE_READ_COUNTER;
  data[1] = counter_id;
  data[2] = group_index;
  Diagnostic_PutU32Le(data, CONFIG_CAN_DIAG_VALUE_INDEX, value);
  data[CONFIG_CAN_DIAG_STATUS_INDEX] = status;
}

void Diagnostic_BuildResetAllCountersResponse(uint8_t status,
                                              uint8_t data[CONFIG_CAN_DIAG_RESPONSE_DLC])
{
  Diagnostic_ClearData(data, CONFIG_CAN_DIAG_RESPONSE_DLC);

  if (data == NULL)
  {
    return;
  }

  data[0] = CONFIG_CAN_DIAG_RESPONSE_RESET_ALL_COUNTERS;
  data[1] = 0U;
  data[2] = CONFIG_CAN_DIAG_GROUP_GLOBAL;
  data[CONFIG_CAN_DIAG_STATUS_INDEX] = status;
}

void Diagnostic_BuildErrorResponse(uint8_t command,
                                   uint8_t counter_id,
                                   uint8_t group_index,
                                   uint8_t status,
                                   uint8_t data[CONFIG_CAN_DIAG_RESPONSE_DLC])
{
  Diagnostic_ClearData(data, CONFIG_CAN_DIAG_RESPONSE_DLC);

  if (data == NULL)
  {
    return;
  }

  data[0] = CONFIG_CAN_DIAG_RESPONSE_ERROR;
  data[1] = counter_id;
  data[2] = group_index;
  data[3] = command;
  data[CONFIG_CAN_DIAG_STATUS_INDEX] = status;
}
