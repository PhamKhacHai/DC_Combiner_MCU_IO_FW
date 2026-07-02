#include "stm32f1xx.h"

#include "bootloader_can.h"
#include "bootloader_flash.h"
#include "bootloader_request.h"
#include "bootloader_update.h"

#define BL_GPIO_PIN(pin)       (1UL << (pin))

#define DO_GPIOA_MASK          (BL_GPIO_PIN(8U) | BL_GPIO_PIN(10U))
#define DO_GPIOB_MASK          (BL_GPIO_PIN(3U) | BL_GPIO_PIN(5U) | \
                                BL_GPIO_PIN(12U) | BL_GPIO_PIN(14U))

typedef void (*app_entry_t)(void);

static void Bootloader_WriteOutputsReset(void)
{
  GPIOA->BRR = DO_GPIOA_MASK;
  GPIOB->BRR = DO_GPIOB_MASK;
}

static void Bootloader_ConfigOutputPin(GPIO_TypeDef *port, uint32_t pin)
{
  volatile uint32_t *config_reg;
  uint32_t shift;
  uint32_t value;

  if (pin < 8U)
  {
    config_reg = &port->CRL;
    shift = pin * 4U;
  }
  else
  {
    config_reg = &port->CRH;
    shift = (pin - 8U) * 4U;
  }

  value = *config_reg;
  value &= ~(0xFU << shift);
  value |= (0x2U << shift); /* Output push-pull, low speed. */
  *config_reg = value;
}

static void Bootloader_ForceOutputsOff(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN |
                  RCC_APB2ENR_IOPAEN |
                  RCC_APB2ENR_IOPBEN;
  (void)RCC->APB2ENR;

  AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk) |
               AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

  Bootloader_WriteOutputsReset();

  Bootloader_ConfigOutputPin(GPIOA, 8U);
  Bootloader_ConfigOutputPin(GPIOA, 10U);
  Bootloader_ConfigOutputPin(GPIOB, 3U);
  Bootloader_ConfigOutputPin(GPIOB, 5U);
  Bootloader_ConfigOutputPin(GPIOB, 12U);
  Bootloader_ConfigOutputPin(GPIOB, 14U);

  Bootloader_WriteOutputsReset();
}

__attribute__((noreturn))
static void Bootloader_StayInSafeLoop(void)
{
  while (1)
  {
    Bootloader_WriteOutputsReset();
  }
}

__attribute__((noreturn))
static void Bootloader_StayInCanLoop(void)
{
  while (1)
  {
    Bootloader_WriteOutputsReset();
    BootloaderCan_Task();
  }
}

__attribute__((noreturn))
static void Bootloader_JumpToApplication(uint32_t app_sp, uint32_t app_reset)
{
  app_entry_t app_entry = (app_entry_t)app_reset;

  __disable_irq();

  for (uint32_t index = 0U; index < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); index++)
  {
    NVIC->ICER[index] = 0xFFFFFFFFU;
    NVIC->ICPR[index] = 0xFFFFFFFFU;
  }

  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL = 0U;

  Bootloader_WriteOutputsReset();

  SCB->VTOR = BOOTLOADER_FLASH_APP_BASE_ADDR;
  __DSB();
  __ISB();

  __set_MSP(app_sp);
  __enable_irq();

  app_entry();

  while (1)
  {
  }
}

int main(void)
{
  uint32_t app_sp = 0U;
  uint32_t app_reset = 0U;
  bool app_valid;
  bool enter_bootloader_mode;
  uint8_t node_id;

  Bootloader_ForceOutputsOff();
  BootloaderUpdate_Init();
  app_valid = BootloaderUpdate_IsApplicationValid(&app_sp, &app_reset);
  enter_bootloader_mode = BootloaderRequest_IsSet() || !app_valid;

  if (BootloaderRequest_IsSet())
  {
    BootloaderRequest_Clear();
  }

  if (enter_bootloader_mode)
  {
    if (BootloaderCan_ConfigSystemClock())
    {
      node_id = BootloaderCan_ReadNodeId();
      if (BootloaderCan_Init(node_id))
      {
        Bootloader_StayInCanLoop();
      }
    }

    Bootloader_StayInSafeLoop();
  }

  if (app_valid)
  {
    Bootloader_JumpToApplication(app_sp, app_reset);
  }

  Bootloader_StayInSafeLoop();
}
