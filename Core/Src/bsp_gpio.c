#include "bsp_gpio.h"
#include "config.h"
#include "main.h"

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
} bsp_gpio_pin_t;

static const bsp_gpio_pin_t output_pins[CONFIG_GROUP_COUNT] =
    {
        {OUT_1_GPIO_Port, OUT_1_Pin},
        {OUT_2_GPIO_Port, OUT_2_Pin},
        {OUT_3_GPIO_Port, OUT_3_Pin},
        {OUT_4_GPIO_Port, OUT_4_Pin},
        {OUT_5_GPIO_Port, OUT_5_Pin},
        {OUT_6_GPIO_Port, OUT_6_Pin},
};

static const bsp_gpio_pin_t feedback_pins[CONFIG_GROUP_COUNT][CONFIG_FEEDBACK_PER_GROUP] =
    {
        {
            {FB1_P_GPIO_Port, FB1_P_Pin},
            {FB1_N_GPIO_Port, FB1_N_Pin},
        },
        {
            {FB2_P_GPIO_Port, FB2_P_Pin},
            {FB2_N_GPIO_Port, FB2_N_Pin},
        },
        {
            {FB3_P_GPIO_Port, FB3_P_Pin},
            {FB3_N_GPIO_Port, FB3_N_Pin},
        },
        {
            {FB4_P_GPIO_Port, FB4_P_Pin},
            {FB4_N_GPIO_Port, FB4_N_Pin},
        },
        {
            {FB5_P_GPIO_Port, FB5_P_Pin},
            {FB5_N_GPIO_Port, FB5_N_Pin},
        },
        {
            {FB6_P_GPIO_Port, FB6_P_Pin},
            {FB6_N_GPIO_Port, FB6_N_Pin},
        },
};

static const bsp_gpio_pin_t dip_pins[CONFIG_DIP_SWITCH_COUNT] =
    {
        {DIP1_GPIO_Port, DIP1_Pin},
        {DIP2_GPIO_Port, DIP2_Pin},
        {DIP3_GPIO_Port, DIP3_Pin},
};

static bsp_gpio_output_mask_t output_mask = CONFIG_OUTPUT_DEFAULT_ON_MASK;

bool BSP_GPIO_IsGroupValid(bsp_gpio_group_t group)
{
  return group < CONFIG_GROUP_COUNT;
}

static bool BSP_GPIO_IsFeedbackSideValid(config_feedback_side_t side)
{
  return (side == CONFIG_FEEDBACK_POSITIVE) || (side == CONFIG_FEEDBACK_NEGATIVE);
}

static bool BSP_GPIO_ReadLogicalInput(const bsp_gpio_pin_t *gpio, bool active_low)
{
  GPIO_PinState raw_state = HAL_GPIO_ReadPin(gpio->port, gpio->pin);
  bool raw_active = raw_state == GPIO_PIN_SET;

  return active_low ? !raw_active : raw_active;
}

void BSP_GPIO_AllOutputsOff(void)
{
  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    BSP_GPIO_SetOutput(group, false);
  }

  output_mask = 0U;
}

void BSP_GPIO_SetOutput(bsp_gpio_group_t group, bool on)
{
  if (!BSP_GPIO_IsGroupValid(group))
  {
    return;
  }

  bool gpio_set = on;
#if (CONFIG_OUTPUT_ACTIVE_HIGH == 0U)
  gpio_set = !gpio_set;
#endif

  HAL_GPIO_WritePin(output_pins[group].port,
                    output_pins[group].pin,
                    gpio_set ? GPIO_PIN_SET : GPIO_PIN_RESET);

  if (on)
  {
    output_mask |= (uint8_t)(1U << group);
  }
  else
  {
    output_mask &= (uint8_t)~(1U << group);
  }
}

bool BSP_GPIO_GetOutputCommand(bsp_gpio_group_t group)
{
  if (!BSP_GPIO_IsGroupValid(group))
  {
    return false;
  }

  return (output_mask & (uint8_t)(1U << group)) != 0U;
}

bsp_gpio_output_mask_t BSP_GPIO_GetOutputMask(void)
{
  return (bsp_gpio_output_mask_t)(output_mask & CONFIG_GROUP_ALL_MASK);
}

bool BSP_GPIO_ReadFeedback(bsp_gpio_group_t group, config_feedback_side_t side)
{
  if (!BSP_GPIO_IsGroupValid(group) || !BSP_GPIO_IsFeedbackSideValid(side))
  {
    return false;
  }

  return BSP_GPIO_ReadLogicalInput(&feedback_pins[group][side],
                                   CONFIG_FEEDBACK_ACTIVE_LOW != 0U);
}

bsp_gpio_feedback_mask_t BSP_GPIO_ReadFeedbackMask(void)
{
  bsp_gpio_feedback_mask_t mask = 0U;

  for (bsp_gpio_group_t group = 0U; group < CONFIG_GROUP_COUNT; group++)
  {
    if (BSP_GPIO_ReadFeedback(group, CONFIG_FEEDBACK_POSITIVE))
    {
      mask |= (bsp_gpio_feedback_mask_t)(1U << (group * CONFIG_FEEDBACK_PER_GROUP));
    }

    if (BSP_GPIO_ReadFeedback(group, CONFIG_FEEDBACK_NEGATIVE))
    {
      mask |= (bsp_gpio_feedback_mask_t)(1U << ((group * CONFIG_FEEDBACK_PER_GROUP) + 1U));
    }
  }

  return mask;
}

bsp_gpio_node_id_t BSP_GPIO_ReadNodeId(void)
{
  bsp_gpio_node_id_t node_id = 0U;

  for (uint8_t bit = 0U; bit < CONFIG_DIP_SWITCH_COUNT; bit++)
  {
    if (BSP_GPIO_ReadLogicalInput(&dip_pins[bit], CONFIG_DIP_ACTIVE_LOW != 0U))
    {
      node_id |= (uint8_t)(1U << bit);
    }
  }

  return (uint8_t)(node_id & CONFIG_CAN_NODE_ID_MAX);
}
