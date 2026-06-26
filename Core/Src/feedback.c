#include "feedback.h"

typedef struct
{
  feedback_group_status_t raw;
  feedback_group_status_t stable;
  feedback_group_status_t candidate;
  uint32_t candidate_start_ms;
} feedback_group_debounce_t;

static feedback_group_debounce_t feedback_groups[CONFIG_GROUP_COUNT];
static bsp_gpio_feedback_mask_t raw_feedback_mask;
static bsp_gpio_output_mask_t group_on_mask;
static bsp_gpio_output_mask_t group_off_mask;
static bsp_gpio_output_mask_t group_mismatch_mask;

bool Feedback_IsGroupValid(bsp_gpio_group_t group)
{
  return group < CONFIG_GROUP_COUNT;
}

static feedback_group_state_t Feedback_ClassifyGroup(bool positive_on, bool negative_on)
{
  if (positive_on && negative_on)
  {
    return FEEDBACK_GROUP_ON;
  }

  if (!positive_on && !negative_on)
  {
    return FEEDBACK_GROUP_OFF;
  }

  return FEEDBACK_GROUP_MISMATCH;
}

static bool Feedback_IsTimeout(uint32_t now_ms, uint32_t start_ms, uint32_t timeout_ms)
{
  return (uint32_t)(now_ms - start_ms) >= timeout_ms;
}

static bool Feedback_IsSameStatus(const feedback_group_status_t *left,
                                  const feedback_group_status_t *right)
{
  return (left->positive_on == right->positive_on) &&
         (left->negative_on == right->negative_on) &&
         (left->state == right->state);
}

static feedback_group_status_t Feedback_MakeStatus(bool positive_on, bool negative_on)
{
  feedback_group_status_t status;

  status.positive_on = positive_on;
  status.negative_on = negative_on;
  status.state = Feedback_ClassifyGroup(positive_on, negative_on);

  return status;
}

static feedback_group_status_t Feedback_GetStatusFromRawMask(bsp_gpio_feedback_mask_t mask,
                                                             bsp_gpio_group_t group)
{
  uint8_t positive_bit = (uint8_t)(group * CONFIG_FEEDBACK_PER_GROUP);
  uint8_t negative_bit = (uint8_t)(positive_bit + 1U);
  bool positive_on = (mask & (bsp_gpio_feedback_mask_t)(1U << positive_bit)) != 0U;
  bool negative_on = (mask & (bsp_gpio_feedback_mask_t)(1U << negative_bit)) != 0U;

  return Feedback_MakeStatus(positive_on, negative_on);
}

static void Feedback_RebuildStableMasks(void)
{
  group_on_mask = 0U;
  group_off_mask = 0U;
  group_mismatch_mask = 0U;

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    bsp_gpio_output_mask_t group_bit = (bsp_gpio_output_mask_t)(1U << group);

    if (feedback_groups[group].stable.state == FEEDBACK_GROUP_ON)
    {
      group_on_mask |= group_bit;
    }
    else if (feedback_groups[group].stable.state == FEEDBACK_GROUP_OFF)
    {
      group_off_mask |= group_bit;
    }
    else
    {
      group_mismatch_mask |= group_bit;
    }
  }
}

static void Feedback_UpdateGroupDebounce(bsp_gpio_group_t group,
                                         feedback_group_status_t raw_status,
                                         uint32_t now_ms)
{
  feedback_group_debounce_t *feedback = &feedback_groups[group];

  feedback->raw = raw_status;

  if (!Feedback_IsSameStatus(&raw_status, &feedback->candidate))
  {
    feedback->candidate = raw_status;
    feedback->candidate_start_ms = now_ms;
  }

  if (!Feedback_IsSameStatus(&feedback->stable, &feedback->candidate) &&
      Feedback_IsTimeout(now_ms,
                         feedback->candidate_start_ms,
                         CONFIG_FEEDBACK_DEBOUNCE_MS))
  {
    feedback->stable = feedback->candidate;
  }
}

void Feedback_Init(uint32_t now_ms)
{
  raw_feedback_mask = BSP_GPIO_ReadFeedbackMask();

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    feedback_group_status_t raw_status = Feedback_GetStatusFromRawMask(raw_feedback_mask, group);

    feedback_groups[group].raw = raw_status;
    feedback_groups[group].stable = raw_status;
    feedback_groups[group].candidate = raw_status;
    feedback_groups[group].candidate_start_ms = now_ms;
  }

  Feedback_RebuildStableMasks();
}

void Feedback_Update(uint32_t now_ms)
{
  raw_feedback_mask = BSP_GPIO_ReadFeedbackMask();

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    feedback_group_status_t raw_status = Feedback_GetStatusFromRawMask(raw_feedback_mask, group);

    Feedback_UpdateGroupDebounce(group, raw_status, now_ms);
  }

  Feedback_RebuildStableMasks();
}

bool Feedback_IsPositiveOn(bsp_gpio_group_t group)
{
  if (!Feedback_IsGroupValid(group))
  {
    return false;
  }

  return feedback_groups[group].stable.positive_on;
}

bool Feedback_IsNegativeOn(bsp_gpio_group_t group)
{
  if (!Feedback_IsGroupValid(group))
  {
    return false;
  }

  return feedback_groups[group].stable.negative_on;
}

bool Feedback_IsGroupOn(bsp_gpio_group_t group)
{
  return Feedback_GetGroupState(group) == FEEDBACK_GROUP_ON;
}

bool Feedback_IsGroupOff(bsp_gpio_group_t group)
{
  return Feedback_GetGroupState(group) == FEEDBACK_GROUP_OFF;
}

bool Feedback_IsGroupMismatch(bsp_gpio_group_t group)
{
  return Feedback_GetGroupState(group) == FEEDBACK_GROUP_MISMATCH;
}

feedback_group_state_t Feedback_GetGroupState(bsp_gpio_group_t group)
{
  if (!Feedback_IsGroupValid(group))
  {
    /* Treat invalid group as mismatch so callers fail safe. */
    return FEEDBACK_GROUP_MISMATCH;
  }

  return feedback_groups[group].stable.state;
}

feedback_group_status_t Feedback_GetGroupStatus(bsp_gpio_group_t group)
{
  feedback_group_status_t invalid_status =
  {
    false,
    false,
    FEEDBACK_GROUP_MISMATCH,
  };

  if (!Feedback_IsGroupValid(group))
  {
    /* Treat invalid group as mismatch so callers fail safe. */
    return invalid_status;
  }

  return feedback_groups[group].stable;
}

bsp_gpio_feedback_mask_t Feedback_GetRawMask(void)
{
  return raw_feedback_mask;
}

bsp_gpio_output_mask_t Feedback_GetOnMask(void)
{
  return group_on_mask;
}

bsp_gpio_output_mask_t Feedback_GetOffMask(void)
{
  return group_off_mask;
}

bsp_gpio_output_mask_t Feedback_GetMismatchMask(void)
{
  return group_mismatch_mask;
}
