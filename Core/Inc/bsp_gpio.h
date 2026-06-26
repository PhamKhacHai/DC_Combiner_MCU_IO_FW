#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

typedef uint8_t bsp_gpio_group_t;
typedef uint8_t bsp_gpio_output_mask_t;
typedef uint16_t bsp_gpio_feedback_mask_t;
typedef uint8_t bsp_gpio_node_id_t;

bool BSP_GPIO_IsGroupValid(bsp_gpio_group_t group);

void BSP_GPIO_AllOutputsOff(void);
void BSP_GPIO_SetOutput(bsp_gpio_group_t group, bool on);
bool BSP_GPIO_GetOutputCommand(bsp_gpio_group_t group);
bsp_gpio_output_mask_t BSP_GPIO_GetOutputMask(void);

bool BSP_GPIO_ReadFeedback(bsp_gpio_group_t group, config_feedback_side_t side);
bsp_gpio_feedback_mask_t BSP_GPIO_ReadFeedbackMask(void);

bsp_gpio_node_id_t BSP_GPIO_ReadNodeId(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_GPIO_H */
