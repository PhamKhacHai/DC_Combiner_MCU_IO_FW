#include "stm32f1xx.h"

#include "bootloader_request.h"

#define APP_BASE_ADDR          (0x08004000U)
#define APP_FLASH_END_ADDR     (0x0800FBFFU)
#define SRAM_START_ADDR        (0x20000000U)
#define SRAM_END_ADDR          (0x20005000U)

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

static int Bootloader_IsValidApplication(uint32_t *app_sp, uint32_t *app_reset)
{
  uint32_t initial_sp = *(uint32_t *)APP_BASE_ADDR;
  uint32_t reset_handler = *(uint32_t *)(APP_BASE_ADDR + 4U);
  uint32_t reset_address = reset_handler & ~1UL;

  if ((initial_sp == 0x00000000U) || (initial_sp == 0xFFFFFFFFU))
  {
    return 0;
  }

  if ((reset_handler == 0x00000000U) || (reset_handler == 0xFFFFFFFFU))
  {
    return 0;
  }

  if ((initial_sp < SRAM_START_ADDR) || (initial_sp > SRAM_END_ADDR))
  {
    return 0;
  }

  if ((initial_sp & 0x3U) != 0U)
  {
    return 0;
  }

  if ((reset_handler & 1U) == 0U)
  {
    return 0;
  }

  if ((reset_address < APP_BASE_ADDR) || (reset_address > APP_FLASH_END_ADDR))
  {
    return 0;
  }

  *app_sp = initial_sp;
  *app_reset = reset_handler;
  return 1;
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

  SCB->VTOR = APP_BASE_ADDR;
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

  Bootloader_ForceOutputsOff();

  if (BootloaderRequest_IsSet())
  {
    BootloaderRequest_Clear();
    Bootloader_StayInSafeLoop();
  }

  if (Bootloader_IsValidApplication(&app_sp, &app_reset))
  {
    Bootloader_JumpToApplication(app_sp, app_reset);
  }

  Bootloader_StayInSafeLoop();
}
