#ifndef CONTACTOR_H
#define CONTACTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "bsp_gpio.h"

typedef enum
{
  CONTACTOR_STATE_OFF = 0,
  CONTACTOR_STATE_ON_REQUESTED,
  CONTACTOR_STATE_ON_CONFIRMED,
  CONTACTOR_STATE_OFF_REQUESTED,
  CONTACTOR_STATE_FAULT
} contactor_state_t;

typedef enum
{
  CONTACTOR_FAULT_NONE = 0,
  CONTACTOR_FAULT_ON_TIMEOUT,
  CONTACTOR_FAULT_OFF_TIMEOUT,
  CONTACTOR_FAULT_FEEDBACK_MISMATCH,
  CONTACTOR_FAULT_STUCK_ON,
  CONTACTOR_FAULT_UNEXPECTED_OFF
} contactor_fault_t;

void Contactor_Init(uint32_t now_ms);
void Contactor_Task(uint32_t now_ms);

bool Contactor_RequestOn(bsp_gpio_group_t group, uint32_t now_ms);
bool Contactor_RequestOff(bsp_gpio_group_t group, uint32_t now_ms);

void Contactor_SetCommandMask(bsp_gpio_output_mask_t mask, uint32_t now_ms);
void Contactor_AllOff(uint32_t now_ms);

contactor_state_t Contactor_GetState(bsp_gpio_group_t group);
contactor_fault_t Contactor_GetFault(bsp_gpio_group_t group);

bool Contactor_IsGroupOnConfirmed(bsp_gpio_group_t group);
bool Contactor_IsGroupFault(bsp_gpio_group_t group);

bsp_gpio_output_mask_t Contactor_GetCommandMask(void);
bsp_gpio_output_mask_t Contactor_GetOnConfirmedMask(void);
bsp_gpio_output_mask_t Contactor_GetFaultMask(void);

bool Contactor_ClearFault(bsp_gpio_group_t group, uint32_t now_ms);
bool Contactor_ClearAllFaults(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* CONTACTOR_H */
