#include "can_comm.h"

#include "can.h"
#include "config.h"
#include "contactor.h"
#include "diagnostic.h"
#include "feedback.h"

#define CAN_COMM_RX_STD_ID_MASK  (0x7CFU)

static bsp_gpio_node_id_t can_node_id;
static uint32_t can_command_id;
static uint32_t can_status_id;
static uint32_t can_heartbeat_id;
static uint32_t can_diag_request_id;
static uint32_t can_diag_response_id;

static volatile bool command_pending;
static volatile uint8_t pending_command_mask;
static volatile uint8_t pending_command_flags;

static volatile bool diag_request_pending;
static volatile uint8_t pending_diag_dlc;
static volatile uint8_t pending_diag_data[8];

static volatile bool has_received_command;
static volatile uint32_t last_rx_ms;
static uint32_t last_status_tx_ms;
static uint32_t last_heartbeat_tx_ms;

static bool can_started;
static bool can_failsafe_active;

static volatile uint32_t rx_count;
static uint32_t tx_count;
static volatile uint32_t rx_error_count;
static uint32_t tx_error_count;

static bool CanComm_IsElapsed(uint32_t now_ms, uint32_t start_ms, uint32_t period_ms)
{
  return (uint32_t)(now_ms - start_ms) >= period_ms;
}

static void CanComm_ResetState(uint32_t now_ms)
{
  can_node_id = 0U;
  can_command_id = Config_CanCommandId(0U);
  can_status_id = Config_CanStatusId(0U);
  can_heartbeat_id = Config_CanHeartbeatId(0U);
  can_diag_request_id = Config_CanDiagRequestId(0U);
  can_diag_response_id = Config_CanDiagResponseId(0U);

  command_pending = false;
  pending_command_mask = 0U;
  pending_command_flags = 0U;

  diag_request_pending = false;
  pending_diag_dlc = 0U;
  for (uint8_t index = 0U; index < 8U; index++)
  {
    pending_diag_data[index] = 0U;
  }

  has_received_command = false;
  last_rx_ms = now_ms;
  last_status_tx_ms = now_ms;
  last_heartbeat_tx_ms = now_ms;

  can_started = false;
  can_failsafe_active = false;
  Diagnostic_Init();

  rx_count = 0U;
  tx_count = 0U;
  rx_error_count = 0U;
  tx_error_count = 0U;
}

static bool CanComm_ConfigFilter(void)
{
  CAN_FilterTypeDef filter = {0};

  filter.FilterBank = 0U;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = (uint16_t)((can_command_id & CAN_COMM_RX_STD_ID_MASK) << 5);
  filter.FilterIdLow = 0x0000U;
  filter.FilterMaskIdHigh = (uint16_t)(CAN_COMM_RX_STD_ID_MASK << 5);
  filter.FilterMaskIdLow = 0x0000U;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  filter.FilterActivation = ENABLE;
  filter.SlaveStartFilterBank = 14U;

  return HAL_CAN_ConfigFilter(&hcan, &filter) == HAL_OK;
}

static void CanComm_CopyPendingCommand(bool *has_command, uint8_t *command_mask, uint8_t *command_flags)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();

  *has_command = command_pending;
  *command_mask = pending_command_mask;
  *command_flags = pending_command_flags;
  command_pending = false;
  pending_command_flags = 0U;

  __set_PRIMASK(primask);
}

static void CanComm_CopyPendingDiagnosticRequest(bool *has_request,
                                                 uint8_t *dlc,
                                                 uint8_t data[8])
{
  uint8_t index;
  uint32_t primask = __get_PRIMASK();

  __disable_irq();

  *has_request = diag_request_pending;
  *dlc = pending_diag_dlc;

  for (index = 0U; index < 8U; index++)
  {
    data[index] = pending_diag_data[index];
    pending_diag_data[index] = 0U;
  }

  diag_request_pending = false;
  pending_diag_dlc = 0U;

  __set_PRIMASK(primask);
}

static bool CanComm_IsClearFaultRequested(uint8_t command_flags)
{
#if (CONFIG_CAN_CLEAR_FAULT_ENABLE != 0U)
  return (command_flags & CONFIG_CAN_COMMAND_CLEAR_FAULT_MASK) != 0U;
#else
  (void)command_flags;
  return false;
#endif
}

static void CanComm_EnterFailSafe(uint32_t now_ms)
{
  if (can_failsafe_active)
  {
    return;
  }

  Contactor_AllOff(now_ms);
  can_failsafe_active = true;
  Diagnostic_RecordFailSafeEnter();
}

static void CanComm_CheckCommandTimeout(uint32_t now_ms)
{
  if (has_received_command &&
      !can_failsafe_active &&
      CanComm_IsElapsed(now_ms, last_rx_ms, CONFIG_CAN_COMMAND_TIMEOUT_MS))
  {
    Diagnostic_RecordCanTimeout();
    CanComm_EnterFailSafe(now_ms);
  }
}

static bool CanComm_IsFailSafeExitCommand(uint8_t command_mask, uint8_t command_flags)
{
  if (command_mask != 0U)
  {
    return false;
  }

  if (command_flags == 0U)
  {
    return true;
  }

  return CanComm_IsClearFaultRequested(command_flags);
}

static void CanComm_HandleCommand(uint8_t command_mask, uint8_t command_flags, uint32_t now_ms)
{
  if (can_failsafe_active)
  {
    if (CanComm_IsFailSafeExitCommand(command_mask, command_flags))
    {
      can_failsafe_active = false;
      Contactor_AllOff(now_ms);

      if (CanComm_IsClearFaultRequested(command_flags))
      {
        (void)Contactor_ClearAllFaults(now_ms);
        Contactor_AllOff(now_ms);
      }
    }
    else
    {
      Contactor_AllOff(now_ms);
    }

    return;
  }

  if (CanComm_IsClearFaultRequested(command_flags))
  {
    if (command_mask == 0U)
    {
      Contactor_AllOff(now_ms);
      (void)Contactor_ClearAllFaults(now_ms);
      Contactor_AllOff(now_ms);
    }
    else
    {
      Contactor_AllOff(now_ms);
    }

    return;
  }

  Contactor_SetCommandMask((bsp_gpio_output_mask_t)command_mask, now_ms);
}

static uint8_t CanComm_BuildGlobalStatus(bsp_gpio_output_mask_t fault_mask)
{
  uint8_t status = 0U;

  if (can_started)
  {
    status |= CONFIG_STATUS_CAN_ONLINE_MASK;
  }

  if (fault_mask != 0U)
  {
    status |= CONFIG_STATUS_ANY_FAULT_MASK;
  }

  if (can_failsafe_active)
  {
    status |= CONFIG_STATUS_ANY_FAULT_MASK;
    status |= CONFIG_STATUS_FAILSAFE_ACTIVE_MASK;
  }

  return status;
}

static bool CanComm_SendDataFrame(uint32_t std_id, uint8_t dlc, const uint8_t *data)
{
  CAN_TxHeaderTypeDef tx_header = {0};
  uint32_t tx_mailbox = 0U;

  if (!can_started)
  {
    return false;
  }

  tx_header.StdId = std_id;
  tx_header.ExtId = 0U;
  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = dlc;
  tx_header.TransmitGlobalTime = DISABLE;

  if (HAL_CAN_AddTxMessage(&hcan, &tx_header, (uint8_t *)data, &tx_mailbox) == HAL_OK)
  {
    tx_count++;
    Diagnostic_RecordCanTxSuccess();
    return true;
  }

  tx_error_count++;
  Diagnostic_RecordCanTxError();
  return false;
}

static bool CanComm_SendHeartbeat(uint32_t now_ms)
{
  uint8_t data[CONFIG_CAN_HEARTBEAT_DLC] = {0};
  bsp_gpio_output_mask_t fault_mask;
  diagnostic_heartbeat_input_t heartbeat_input;

  (void)now_ms;

  if (!can_started)
  {
    return false;
  }

  fault_mask = Contactor_GetFaultMask();
  heartbeat_input.fw_major = (uint8_t)CONFIG_FW_VERSION_MAJOR;
  heartbeat_input.fw_minor = (uint8_t)CONFIG_FW_VERSION_MINOR;
  heartbeat_input.fw_patch = (uint8_t)CONFIG_FW_VERSION_PATCH;
  heartbeat_input.node_id = can_node_id;
  heartbeat_input.global_status = CanComm_BuildGlobalStatus(fault_mask);
  heartbeat_input.valid_command_rx_count = rx_count;
  heartbeat_input.can_tx_fail_count = tx_error_count;

  Diagnostic_BuildHeartbeatPayload(&heartbeat_input, data);

  if (CanComm_SendDataFrame(can_heartbeat_id, CONFIG_CAN_HEARTBEAT_DLC, data))
  {
    Diagnostic_OnHeartbeatTxSuccess();
    return true;
  }

  return false;
}

static void CanComm_HandleDiagnosticRequest(uint8_t dlc, const uint8_t data[8])
{
  uint8_t command = 0U;
  uint8_t counter_id = 0U;
  uint8_t group_index = CONFIG_CAN_DIAG_GROUP_GLOBAL;
  uint8_t response[CONFIG_CAN_DIAG_RESPONSE_DLC] = {0};

  if (dlc > CONFIG_CAN_DIAG_COMMAND_INDEX)
  {
    command = data[CONFIG_CAN_DIAG_COMMAND_INDEX];
  }

  if (dlc > CONFIG_CAN_DIAG_COUNTER_ID_INDEX)
  {
    counter_id = data[CONFIG_CAN_DIAG_COUNTER_ID_INDEX];
  }

  if (dlc > CONFIG_CAN_DIAG_GROUP_INDEX)
  {
    group_index = data[CONFIG_CAN_DIAG_GROUP_INDEX];
  }

  if (dlc < CONFIG_CAN_DIAG_REQUEST_DLC_MIN)
  {
    Diagnostic_RecordDiagError();
    Diagnostic_BuildErrorResponse(command,
                                  counter_id,
                                  group_index,
                                  CONFIG_CAN_DIAG_STATUS_BAD_DLC,
                                  response);
    (void)CanComm_SendDataFrame(can_diag_response_id, CONFIG_CAN_DIAG_RESPONSE_DLC, response);
    return;
  }

  if (command == CONFIG_CAN_DIAG_COMMAND_READ_COUNTER)
  {
    uint32_t counter_value;

    if (Diagnostic_GetCounter(counter_id, group_index, &counter_value))
    {
      Diagnostic_RecordDiagRequest();
    }

    Diagnostic_BuildReadCounterResponse(counter_id, group_index, response);

    if (response[CONFIG_CAN_DIAG_STATUS_INDEX] != CONFIG_CAN_DIAG_STATUS_OK)
    {
      Diagnostic_RecordDiagError();
    }

    (void)CanComm_SendDataFrame(can_diag_response_id, CONFIG_CAN_DIAG_RESPONSE_DLC, response);
    return;
  }

  if (command == CONFIG_CAN_DIAG_COMMAND_RESET_ALL_COUNTERS)
  {
    if ((counter_id == CONFIG_CAN_DIAG_RESET_MAGIC0) &&
        (group_index == CONFIG_CAN_DIAG_RESET_MAGIC1))
    {
      Diagnostic_RecordDiagRequest();
      Diagnostic_ResetAllCounters();
      Diagnostic_BuildResetAllCountersResponse(CONFIG_CAN_DIAG_STATUS_OK, response);
    }
    else
    {
      Diagnostic_RecordDiagError();
      Diagnostic_BuildResetAllCountersResponse(CONFIG_CAN_DIAG_STATUS_RESET_MAGIC_INVALID,
                                               response);
    }

    (void)CanComm_SendDataFrame(can_diag_response_id, CONFIG_CAN_DIAG_RESPONSE_DLC, response);
    return;
  }

  Diagnostic_RecordDiagError();
  Diagnostic_BuildErrorResponse(command,
                                counter_id,
                                group_index,
                                CONFIG_CAN_DIAG_STATUS_INVALID_COMMAND,
                                response);
  (void)CanComm_SendDataFrame(can_diag_response_id, CONFIG_CAN_DIAG_RESPONSE_DLC, response);
}

bool CanComm_Init(uint32_t now_ms)
{
  CanComm_ResetState(now_ms);

  can_node_id = (bsp_gpio_node_id_t)(BSP_GPIO_ReadNodeId() & CONFIG_CAN_NODE_ID_MAX);
  can_command_id = Config_CanCommandId(can_node_id);
  can_status_id = Config_CanStatusId(can_node_id);
  can_heartbeat_id = Config_CanHeartbeatId(can_node_id);
  can_diag_request_id = Config_CanDiagRequestId(can_node_id);
  can_diag_response_id = Config_CanDiagResponseId(can_node_id);

  if (!CanComm_ConfigFilter())
  {
    tx_error_count++;
    return false;
  }

  if (HAL_CAN_Start(&hcan) != HAL_OK)
  {
    tx_error_count++;
    return false;
  }

  if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    tx_error_count++;
    return false;
  }

  can_started = true;
  last_rx_ms = now_ms;
  last_status_tx_ms = now_ms;
  last_heartbeat_tx_ms = now_ms;

  return true;
}

void CanComm_Task(uint32_t now_ms)
{
  bool has_command = false;
  bool has_diag_request = false;
  uint8_t command_mask = 0U;
  uint8_t command_flags = 0U;
  uint8_t diag_dlc = 0U;
  uint8_t diag_data[8] = {0};

  if (!can_started)
  {
    return;
  }

  CanComm_CopyPendingCommand(&has_command, &command_mask, &command_flags);
  CanComm_CopyPendingDiagnosticRequest(&has_diag_request, &diag_dlc, diag_data);

  if (has_command)
  {
    CanComm_HandleCommand(command_mask, command_flags, now_ms);
  }

  if (has_diag_request)
  {
    CanComm_HandleDiagnosticRequest(diag_dlc, diag_data);
  }

  CanComm_CheckCommandTimeout(now_ms);

  if (CanComm_IsElapsed(now_ms, last_status_tx_ms, CONFIG_STATUS_PERIOD_MS) &&
      CanComm_SendStatus(now_ms))
  {
    last_status_tx_ms = now_ms;
  }

#if (CONFIG_CAN_HEARTBEAT_ENABLE != 0U)
  if (CanComm_IsElapsed(now_ms, last_heartbeat_tx_ms, CONFIG_CAN_HEARTBEAT_PERIOD_MS))
  {
    (void)CanComm_SendHeartbeat(now_ms);
    last_heartbeat_tx_ms = now_ms;
  }
#endif
}

bool CanComm_SendStatus(uint32_t now_ms)
{
  uint8_t data[CONFIG_CAN_STATUS_DLC] = {0};
  bsp_gpio_feedback_mask_t raw_feedback_mask;
  bsp_gpio_output_mask_t fault_mask;

  (void)now_ms;

  if (!can_started)
  {
    return false;
  }

  raw_feedback_mask = Feedback_GetRawMask();
  fault_mask = Contactor_GetFaultMask();

  data[CONFIG_CAN_STATUS_DO_MASK_INDEX] = BSP_GPIO_GetOutputMask();
  data[CONFIG_CAN_STATUS_FB_LOW_INDEX] = (uint8_t)(raw_feedback_mask & 0xFFU);
  data[CONFIG_CAN_STATUS_FB_HIGH_INDEX] = (uint8_t)((raw_feedback_mask >> 8) & 0xFFU);
  data[CONFIG_CAN_STATUS_FAULT_MASK_INDEX] = fault_mask;
  data[CONFIG_CAN_STATUS_GLOBAL_INDEX] = CanComm_BuildGlobalStatus(fault_mask);

  return CanComm_SendDataFrame(can_status_id, CONFIG_CAN_STATUS_DLC, data);
}

bsp_gpio_node_id_t CanComm_GetNodeId(void)
{
  return can_node_id;
}

uint32_t CanComm_GetLastRxTimeMs(void)
{
  return last_rx_ms;
}

bool CanComm_HasPendingCommand(void)
{
  return command_pending;
}

uint32_t CanComm_GetRxCount(void)
{
  return rx_count;
}

uint32_t CanComm_GetTxCount(void)
{
  return tx_count;
}

uint32_t CanComm_GetRxErrorCount(void)
{
  return rx_error_count;
}

uint32_t CanComm_GetTxErrorCount(void)
{
  return tx_error_count;
}

bool CanComm_IsStarted(void)
{
  return can_started;
}

bool CanComm_IsFailSafeActive(void)
{
  return can_failsafe_active;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *can_handle)
{
  CAN_RxHeaderTypeDef rx_header = {0};
  uint8_t data[8] = {0};

  if (can_handle != &hcan)
  {
    return;
  }

  if (HAL_CAN_GetRxMessage(can_handle, CAN_RX_FIFO0, &rx_header, data) != HAL_OK)
  {
    rx_error_count++;
    Diagnostic_RecordCanRxError();
    return;
  }

  if ((rx_header.IDE != CAN_ID_STD) ||
      (rx_header.RTR != CAN_RTR_DATA))
  {
    return;
  }

  if (rx_header.StdId == can_command_id)
  {
    if (rx_header.DLC < CONFIG_CAN_COMMAND_DLC)
    {
      return;
    }

    pending_command_mask = (uint8_t)(data[CONFIG_CAN_COMMAND_MASK_INDEX] & CONFIG_GROUP_ALL_MASK);
    pending_command_flags = 0U;

    if (rx_header.DLC > CONFIG_CAN_COMMAND_FLAGS_INDEX)
    {
      pending_command_flags = data[CONFIG_CAN_COMMAND_FLAGS_INDEX];
    }

    command_pending = true;
    has_received_command = true;
    last_rx_ms = HAL_GetTick();
    rx_count++;
    Diagnostic_RecordCanCommandRx();
    return;
  }

  if (rx_header.StdId == can_diag_request_id)
  {
    uint8_t index;

    pending_diag_dlc = (uint8_t)rx_header.DLC;

    for (index = 0U; index < 8U; index++)
    {
      pending_diag_data[index] = data[index];
    }

    diag_request_pending = true;
  }
}
