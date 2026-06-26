#ifndef FEEDBACK_H
#define FEEDBACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "bsp_gpio.h"
#include "config.h"

typedef enum
{
  FEEDBACK_GROUP_OFF = 0,
  FEEDBACK_GROUP_ON,
  FEEDBACK_GROUP_MISMATCH,
} feedback_group_state_t;

typedef struct
{
  bool positive_on;
  bool negative_on;
  feedback_group_state_t state;
} feedback_group_status_t;

void Feedback_Init(uint32_t now_ms);
void Feedback_Update(uint32_t now_ms);

bool Feedback_IsGroupValid(bsp_gpio_group_t group);
bool Feedback_IsPositiveOn(bsp_gpio_group_t group);
bool Feedback_IsNegativeOn(bsp_gpio_group_t group);
bool Feedback_IsGroupOn(bsp_gpio_group_t group);
bool Feedback_IsGroupOff(bsp_gpio_group_t group);
bool Feedback_IsGroupMismatch(bsp_gpio_group_t group);

feedback_group_state_t Feedback_GetGroupState(bsp_gpio_group_t group);
feedback_group_status_t Feedback_GetGroupStatus(bsp_gpio_group_t group);

bsp_gpio_feedback_mask_t Feedback_GetRawMask(void);
bsp_gpio_output_mask_t Feedback_GetOnMask(void);
bsp_gpio_output_mask_t Feedback_GetOffMask(void);
bsp_gpio_output_mask_t Feedback_GetMismatchMask(void);

#ifdef __cplusplus
}
#endif

#endif /* FEEDBACK_H */
