#include "bootloader_request.h"

#include "stm32f1xx.h"

#define BOOTLOADER_REQUEST_NODE_ID_MASK   (0x07U)
#define BOOTLOADER_REQUEST_FLAG_MAGIC0    (0xB007U)
#define BOOTLOADER_REQUEST_FLAG_MAGIC1    (0x10ADU)

uint32_t BootloaderRequest_CanRequestId(uint8_t node_id)
{
  return BOOTLOADER_REQUEST_CAN_ID_BASE + (uint32_t)(node_id & BOOTLOADER_REQUEST_NODE_ID_MASK);
}

uint32_t BootloaderRequest_CanResponseId(uint8_t node_id)
{
  return BOOTLOADER_RESPONSE_CAN_ID_BASE + (uint32_t)(node_id & BOOTLOADER_REQUEST_NODE_ID_MASK);
}

bool BootloaderRequest_IsEnterCommand(uint8_t dlc, const uint8_t data[8])
{
  if (dlc != BOOTLOADER_REQUEST_DLC)
  {
    return false;
  }

  return (data[0] == BOOTLOADER_COMMAND_ENTER) &&
         (data[1] == BOOTLOADER_ENTER_MAGIC0) &&
         (data[2] == BOOTLOADER_ENTER_MAGIC1);
}

bool BootloaderRequest_IsGetBootInfoCommand(uint8_t dlc, const uint8_t data[8])
{
  if (dlc != BOOTLOADER_REQUEST_DLC)
  {
    return false;
  }

  return data[0] == BOOTLOADER_COMMAND_GET_BOOT_INFO;
}

static void BootloaderRequest_EnableBackupAccess(void)
{
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  (void)RCC->APB1ENR;

  PWR->CR |= PWR_CR_DBP;
}

void BootloaderRequest_Set(void)
{
  BootloaderRequest_EnableBackupAccess();

  BKP->DR1 = BOOTLOADER_REQUEST_FLAG_MAGIC0;
  BKP->DR2 = BOOTLOADER_REQUEST_FLAG_MAGIC1;
}

bool BootloaderRequest_IsSet(void)
{
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  (void)RCC->APB1ENR;

  return ((BKP->DR1 & BKP_DR1_D_Msk) == BOOTLOADER_REQUEST_FLAG_MAGIC0) &&
         ((BKP->DR2 & BKP_DR2_D_Msk) == BOOTLOADER_REQUEST_FLAG_MAGIC1);
}

void BootloaderRequest_Clear(void)
{
  BootloaderRequest_EnableBackupAccess();

  BKP->DR1 = 0U;
  BKP->DR2 = 0U;
}
