#include "contactor.h"

#include "config.h"
#include "diagnostic.h"
#include "feedback.h"

#define CONTACTOR_STABLE_FAULT_DELAY_MS  CONFIG_CONTACTOR_STABLE_FAULT_DELAY_MS

typedef struct
{
  contactor_state_t state;
  contactor_fault_t fault;
  bool command_on;
  uint32_t state_start_ms;
  uint32_t abnormal_start_ms;
  bool abnormal_active;
} contactor_group_t;

static contactor_group_t contactor_groups[CONFIG_GROUP_COUNT];

static bool Contactor_IsTimeout(uint32_t now_ms, uint32_t start_ms, uint32_t timeout_ms)
{
  return (uint32_t)(now_ms - start_ms) >= timeout_ms;
}

static bool Contactor_IsValidGroup(bsp_gpio_group_t group)
{
  return group < CONFIG_GROUP_COUNT;
}

static void Contactor_ClearAbnormal(bsp_gpio_group_t group)
{
  contactor_groups[group].abnormal_active = false;
}

static void Contactor_StartAbnormalIfNeeded(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (!contactor_groups[group].abnormal_active)
  {
    contactor_groups[group].abnormal_active = true;
    contactor_groups[group].abnormal_start_ms = now_ms;
  }
}

static void Contactor_SetFault(bsp_gpio_group_t group, contactor_fault_t fault)
{
  bool was_fault = (contactor_groups[group].state == CONTACTOR_STATE_FAULT) ||
                   (contactor_groups[group].fault != CONTACTOR_FAULT_NONE);

  BSP_GPIO_SetOutput(group, false);
  contactor_groups[group].command_on = false;
  contactor_groups[group].state = CONTACTOR_STATE_FAULT;
  contactor_groups[group].fault = fault;
  contactor_groups[group].abnormal_active = false;

  if (!was_fault && (fault != CONTACTOR_FAULT_NONE))
  {
    Diagnostic_RecordGroupFault(group, (uint8_t)fault);
  }
}

static void Contactor_HandleOnRequested(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (Feedback_IsGroupOn(group))
  {
    contactor_groups[group].state = CONTACTOR_STATE_ON_CONFIRMED;
    contactor_groups[group].fault = CONTACTOR_FAULT_NONE;
    Contactor_ClearAbnormal(group);
    return;
  }

  if (!Contactor_IsTimeout(now_ms,
                           contactor_groups[group].state_start_ms,
                           CONFIG_CONTACTOR_ON_TIMEOUT_MS))
  {
    return;
  }

  if (Feedback_IsGroupMismatch(group))
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_FEEDBACK_MISMATCH);
  }
  else
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_ON_TIMEOUT);
  }
}

static void Contactor_HandleOffRequested(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (Feedback_IsGroupOff(group))
  {
    contactor_groups[group].state = CONTACTOR_STATE_OFF;
    contactor_groups[group].fault = CONTACTOR_FAULT_NONE;
    Contactor_ClearAbnormal(group);
    return;
  }

  if (!Contactor_IsTimeout(now_ms,
                           contactor_groups[group].state_start_ms,
                           CONFIG_CONTACTOR_OFF_TIMEOUT_MS))
  {
    return;
  }

  if (Feedback_IsGroupMismatch(group))
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_FEEDBACK_MISMATCH);
  }
  else if (Feedback_IsGroupOn(group))
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_STUCK_ON);
  }
  else
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_OFF_TIMEOUT);
  }
}

static void Contactor_HandleOnConfirmed(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (Feedback_IsGroupOn(group))
  {
    Contactor_ClearAbnormal(group);
    return;
  }

  Contactor_StartAbnormalIfNeeded(group, now_ms);

  if (!Contactor_IsTimeout(now_ms,
                           contactor_groups[group].abnormal_start_ms,
                           CONTACTOR_STABLE_FAULT_DELAY_MS))
  {
    return;
  }

  if (Feedback_IsGroupMismatch(group))
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_FEEDBACK_MISMATCH);
  }
  else if (Feedback_IsGroupOff(group))
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_UNEXPECTED_OFF);
  }
  else
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_UNEXPECTED_OFF);
  }
}

static void Contactor_HandleOff(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (Feedback_IsGroupOff(group))
  {
    Contactor_ClearAbnormal(group);
    return;
  }

  Contactor_StartAbnormalIfNeeded(group, now_ms);

  if (!Contactor_IsTimeout(now_ms,
                           contactor_groups[group].abnormal_start_ms,
                           CONTACTOR_STABLE_FAULT_DELAY_MS))
  {
    return;
  }

  if (Feedback_IsGroupMismatch(group))
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_FEEDBACK_MISMATCH);
  }
  else if (Feedback_IsGroupOn(group))
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_STUCK_ON);
  }
  else
  {
    Contactor_SetFault(group, CONTACTOR_FAULT_OFF_TIMEOUT);
  }
}

static void Contactor_HandleFault(bsp_gpio_group_t group)
{
  bool was_fault = (contactor_groups[group].state == CONTACTOR_STATE_FAULT) ||
                   (contactor_groups[group].fault != CONTACTOR_FAULT_NONE);

  BSP_GPIO_SetOutput(group, false);
  contactor_groups[group].command_on = false;
  contactor_groups[group].state = CONTACTOR_STATE_FAULT;
  contactor_groups[group].abnormal_active = false;

  if (contactor_groups[group].fault == CONTACTOR_FAULT_NONE)
  {
    contactor_groups[group].fault = CONTACTOR_FAULT_UNEXPECTED_OFF;

    if (!was_fault)
    {
      Diagnostic_RecordGroupFault(group, (uint8_t)contactor_groups[group].fault);
    }
  }
}

void Contactor_Init(uint32_t now_ms)
{
  BSP_GPIO_AllOutputsOff();

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    contactor_groups[group].command_on = false;
    contactor_groups[group].fault = CONTACTOR_FAULT_NONE;
    contactor_groups[group].abnormal_active = false;
    contactor_groups[group].abnormal_start_ms = now_ms;
    contactor_groups[group].state_start_ms = now_ms;

    if (Feedback_IsGroupOff(group))
    {
      contactor_groups[group].state = CONTACTOR_STATE_OFF;
    }
    else
    {
      contactor_groups[group].state = CONTACTOR_STATE_OFF_REQUESTED;
    }
  }
}

void Contactor_Task(uint32_t now_ms)
{
  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    switch (contactor_groups[group].state)
    {
      case CONTACTOR_STATE_OFF:
        Contactor_HandleOff(group, now_ms);
        break;

      case CONTACTOR_STATE_ON_REQUESTED:
        Contactor_HandleOnRequested(group, now_ms);
        break;

      case CONTACTOR_STATE_ON_CONFIRMED:
        Contactor_HandleOnConfirmed(group, now_ms);
        break;

      case CONTACTOR_STATE_OFF_REQUESTED:
        Contactor_HandleOffRequested(group, now_ms);
        break;

      case CONTACTOR_STATE_FAULT:
      default:
        Contactor_HandleFault(group);
        break;
    }
  }
}

bool Contactor_RequestOn(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (!Contactor_IsValidGroup(group))
  {
    return false;
  }

  contactor_state_t state = contactor_groups[group].state;

  if ((state == CONTACTOR_STATE_FAULT) ||
      (state == CONTACTOR_STATE_OFF_REQUESTED))
  {
    return false;
  }

  if ((state == CONTACTOR_STATE_ON_REQUESTED) ||
      (state == CONTACTOR_STATE_ON_CONFIRMED))
  {
    contactor_groups[group].command_on = true;
    return true;
  }

  if (state != CONTACTOR_STATE_OFF)
  {
    return false;
  }

  contactor_groups[group].command_on = true;
  contactor_groups[group].abnormal_active = false;
  BSP_GPIO_SetOutput(group, true);
  contactor_groups[group].state = CONTACTOR_STATE_ON_REQUESTED;
  contactor_groups[group].fault = CONTACTOR_FAULT_NONE;
  contactor_groups[group].state_start_ms = now_ms;

  return true;
}

bool Contactor_RequestOff(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (!Contactor_IsValidGroup(group))
  {
    return false;
  }

  contactor_state_t state = contactor_groups[group].state;

  contactor_groups[group].command_on = false;
  contactor_groups[group].abnormal_active = false;
  BSP_GPIO_SetOutput(group, false);

  if (state == CONTACTOR_STATE_FAULT)
  {
    return true;
  }

  if (state == CONTACTOR_STATE_OFF_REQUESTED)
  {
    return true;
  }

  if ((state == CONTACTOR_STATE_OFF) && Feedback_IsGroupOff(group))
  {
    return true;
  }

  contactor_groups[group].state = CONTACTOR_STATE_OFF_REQUESTED;
  contactor_groups[group].state_start_ms = now_ms;

  return true;
}

void Contactor_SetCommandMask(bsp_gpio_output_mask_t mask, uint32_t now_ms)
{
  bsp_gpio_output_mask_t command_mask = (bsp_gpio_output_mask_t)(mask & CONFIG_GROUP_ALL_MASK);

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    bsp_gpio_output_mask_t group_bit = (bsp_gpio_output_mask_t)(1U << group);

    if ((command_mask & group_bit) == 0U)
    {
      if (contactor_groups[group].command_on ||
          (contactor_groups[group].state == CONTACTOR_STATE_ON_REQUESTED) ||
          (contactor_groups[group].state == CONTACTOR_STATE_ON_CONFIRMED))
      {
        (void)Contactor_RequestOff(group, now_ms);
      }
    }
  }

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    bsp_gpio_output_mask_t group_bit = (bsp_gpio_output_mask_t)(1U << group);

    if ((command_mask & group_bit) != 0U)
    {
      if (contactor_groups[group].state == CONTACTOR_STATE_OFF)
      {
        (void)Contactor_RequestOn(group, now_ms);
      }
    }
  }
}

void Contactor_AllOff(uint32_t now_ms)
{
  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    (void)Contactor_RequestOff(group, now_ms);
  }
}

contactor_state_t Contactor_GetState(bsp_gpio_group_t group)
{
  if (!Contactor_IsValidGroup(group))
  {
    return CONTACTOR_STATE_FAULT;
  }

  return contactor_groups[group].state;
}

contactor_fault_t Contactor_GetFault(bsp_gpio_group_t group)
{
  if (!Contactor_IsValidGroup(group))
  {
    return CONTACTOR_FAULT_NONE;
  }

  return contactor_groups[group].fault;
}

bool Contactor_IsGroupOnConfirmed(bsp_gpio_group_t group)
{
  return Contactor_GetState(group) == CONTACTOR_STATE_ON_CONFIRMED;
}

bool Contactor_IsGroupFault(bsp_gpio_group_t group)
{
  if (!Contactor_IsValidGroup(group))
  {
    return true;
  }

  return (contactor_groups[group].state == CONTACTOR_STATE_FAULT) ||
         (contactor_groups[group].fault != CONTACTOR_FAULT_NONE);
}

bsp_gpio_output_mask_t Contactor_GetCommandMask(void)
{
  bsp_gpio_output_mask_t mask = 0U;

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    if (contactor_groups[group].command_on)
    {
      mask |= (bsp_gpio_output_mask_t)(1U << group);
    }
  }

  return (bsp_gpio_output_mask_t)(mask & CONFIG_GROUP_ALL_MASK);
}

bsp_gpio_output_mask_t Contactor_GetOnConfirmedMask(void)
{
  bsp_gpio_output_mask_t mask = 0U;

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    if (contactor_groups[group].state == CONTACTOR_STATE_ON_CONFIRMED)
    {
      mask |= (bsp_gpio_output_mask_t)(1U << group);
    }
  }

  return (bsp_gpio_output_mask_t)(mask & CONFIG_GROUP_ALL_MASK);
}

bsp_gpio_output_mask_t Contactor_GetFaultMask(void)
{
  bsp_gpio_output_mask_t mask = 0U;

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    if (Contactor_IsGroupFault(group))
    {
      mask |= (bsp_gpio_output_mask_t)(1U << group);
    }
  }

  return (bsp_gpio_output_mask_t)(mask & CONFIG_GROUP_ALL_MASK);
}

bool Contactor_ClearFault(bsp_gpio_group_t group, uint32_t now_ms)
{
  if (!Contactor_IsValidGroup(group))
  {
    return false;
  }

  if (!Contactor_IsGroupFault(group))
  {
    return true;
  }

  BSP_GPIO_SetOutput(group, false);
  contactor_groups[group].command_on = false;
  contactor_groups[group].abnormal_active = false;

  if (Feedback_IsGroupOff(group))
  {
    contactor_groups[group].fault = CONTACTOR_FAULT_NONE;
    contactor_groups[group].state = CONTACTOR_STATE_OFF;
    contactor_groups[group].state_start_ms = now_ms;
    Diagnostic_RecordFaultClear(group);
    return true;
  }

  contactor_groups[group].state = CONTACTOR_STATE_FAULT;
  contactor_groups[group].state_start_ms = now_ms;

  if (Feedback_IsGroupOn(group))
  {
    contactor_groups[group].fault = CONTACTOR_FAULT_STUCK_ON;
  }
  else if (Feedback_IsGroupMismatch(group))
  {
    contactor_groups[group].fault = CONTACTOR_FAULT_FEEDBACK_MISMATCH;
  }
  else
  {
    contactor_groups[group].fault = CONTACTOR_FAULT_OFF_TIMEOUT;
  }

  return false;
}

bool Contactor_ClearAllFaults(uint32_t now_ms)
{
  bool all_cleared = true;

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    if (Contactor_IsGroupFault(group) &&
        !Contactor_ClearFault(group, now_ms))
    {
      all_cleared = false;
    }
  }

  return all_cleared;
}
