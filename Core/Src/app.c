#include "app.h"

#include "can_comm.h"
#include "config.h"
#include "contactor.h"
#include "diagnostic.h"
#include "feedback.h"
#include "main.h"

static uint32_t app_last_task_ms;

void App_Init(void)
{
  uint32_t now_ms = HAL_GetTick();

  Diagnostic_Init();
  Feedback_Init(now_ms);
  Contactor_Init(now_ms);
  (void)CanComm_Init(now_ms);

  app_last_task_ms = now_ms;
}

void App_Task(void)
{
  uint32_t now_ms = HAL_GetTick();

  if ((uint32_t)(now_ms - app_last_task_ms) < CONFIG_APP_TASK_PERIOD_MS)
  {
    return;
  }

  app_last_task_ms = now_ms;

  Feedback_Update(now_ms);
  Diagnostic_UpdateFeedbackState(Feedback_GetOnMask(),
                                 Feedback_GetOffMask(),
                                 Feedback_GetMismatchMask());
  CanComm_Task(now_ms);
  Contactor_Task(now_ms);
}
