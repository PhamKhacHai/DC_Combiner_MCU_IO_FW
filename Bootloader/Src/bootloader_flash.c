#include "bootloader_flash.h"

#include "stm32f1xx.h"

#define BL_FLASH_TIMEOUT                 (1000000U)
#define BL_FLASH_ERROR_FLAGS             (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)
#define BL_FLASH_CLEAR_FLAGS             (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR)

#define BL_GPIO_PIN(pin)                 (1UL << (pin))
#define BL_DO_GPIOA_MASK                 (BL_GPIO_PIN(8U) | BL_GPIO_PIN(10U))
#define BL_DO_GPIOB_MASK                 (BL_GPIO_PIN(3U) | BL_GPIO_PIN(5U) | \
                                          BL_GPIO_PIN(12U) | BL_GPIO_PIN(14U))

static const uint16_t bootloader_flash_test_pattern[] = {
    0xA55AU,
    0x5AA5U,
    0x1234U,
    0xC3C3U,
    0x0F0FU,
    0xF0F0U,
    0x55AAU,
    0xAA55U,
};

static void BootloaderFlash_ForceOutputsOff(void)
{
  GPIOA->BRR = BL_DO_GPIOA_MASK;
  GPIOB->BRR = BL_DO_GPIOB_MASK;
}

static void BootloaderFlash_ClearDetail(uint8_t detail[5])
{
  for (uint8_t index = 0U; index < 5U; index++)
  {
    detail[index] = 0U;
  }
}

static void BootloaderFlash_SetStatusDetail(uint8_t detail[5])
{
  uint32_t sr = FLASH->SR;

  detail[0] = (uint8_t)(sr & 0xFFU);
  detail[1] = (uint8_t)((sr >> 8U) & 0xFFU);
  detail[2] = (uint8_t)(FLASH->CR & 0xFFU);
}

static void BootloaderFlash_ClearFlags(void)
{
  FLASH->SR = BL_FLASH_CLEAR_FLAGS;
}

static bool BootloaderFlash_WaitNotBusy(void)
{
  uint32_t timeout = BL_FLASH_TIMEOUT;

  while (((FLASH->SR & FLASH_SR_BSY) != 0U) && (timeout > 0U))
  {
    timeout--;
  }

  if ((FLASH->SR & FLASH_SR_BSY) != 0U)
  {
    return false;
  }

  return true;
}

static bool BootloaderFlash_WaitReady(void)
{
  if (!BootloaderFlash_WaitNotBusy())
  {
    return false;
  }

  return (FLASH->SR & BL_FLASH_ERROR_FLAGS) == 0U;
}

static bool BootloaderFlash_Unlock(void)
{
  if ((FLASH->CR & FLASH_CR_LOCK) == 0U)
  {
    return true;
  }

  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;

  return (FLASH->CR & FLASH_CR_LOCK) == 0U;
}

static bool BootloaderFlash_Lock(void)
{
  if (!BootloaderFlash_WaitNotBusy())
  {
    return false;
  }

  FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_MER);
  FLASH->CR |= FLASH_CR_LOCK;
  return (FLASH->CR & FLASH_CR_LOCK) != 0U;
}

bool BootloaderFlash_IsScratchAddressRange(uint32_t address, uint32_t length)
{
  uint32_t end_address;

  if (length == 0U)
  {
    return false;
  }

  if (address < BOOTLOADER_FLASH_SCRATCH_PAGE_START)
  {
    return false;
  }

  end_address = address + length - 1U;
  if (end_address < address)
  {
    return false;
  }

  return end_address <= BOOTLOADER_FLASH_SCRATCH_PAGE_END;
}

bool BootloaderFlash_EraseScratchPage(void)
{
  if (!BootloaderFlash_IsScratchAddressRange(BOOTLOADER_FLASH_SCRATCH_PAGE_START,
                                             BOOTLOADER_FLASH_PAGE_SIZE_BYTES))
  {
    return false;
  }

  if (!BootloaderFlash_WaitNotBusy())
  {
    return false;
  }

  BootloaderFlash_ClearFlags();

  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = BOOTLOADER_FLASH_SCRATCH_PAGE_START;
  FLASH->CR |= FLASH_CR_STRT;

  if (!BootloaderFlash_WaitReady())
  {
    FLASH->CR &= ~FLASH_CR_PER;
    return false;
  }

  FLASH->CR &= ~FLASH_CR_PER;
  BootloaderFlash_ClearFlags();

  return true;
}

bool BootloaderFlash_VerifyErased(uint32_t address, uint32_t length)
{
  uint32_t offset;

  if (((address & 1U) != 0U) || ((length & 1U) != 0U))
  {
    return false;
  }

  if (!BootloaderFlash_IsScratchAddressRange(address, length))
  {
    return false;
  }

  for (offset = 0U; offset < length; offset += 2U)
  {
    if (*(volatile const uint16_t *)(address + offset) != 0xFFFFU)
    {
      return false;
    }
  }

  return true;
}

bool BootloaderFlash_ProgramHalfWords(uint32_t address,
                                      const uint16_t *data,
                                      uint32_t halfword_count)
{
  uint32_t index;
  uint32_t length;

  if ((data == 0) || (halfword_count == 0U) || ((address & 1U) != 0U))
  {
    return false;
  }

  if (halfword_count > (0xFFFFFFFFU / 2U))
  {
    return false;
  }

  length = halfword_count * 2U;
  if (!BootloaderFlash_IsScratchAddressRange(address, length))
  {
    return false;
  }

  for (index = 0U; index < halfword_count; index++)
  {
    if (!BootloaderFlash_WaitNotBusy())
    {
      return false;
    }

    BootloaderFlash_ClearFlags();
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)(address + (index * 2U)) = data[index];

    if (!BootloaderFlash_WaitReady())
    {
      FLASH->CR &= ~FLASH_CR_PG;
      return false;
    }

    FLASH->CR &= ~FLASH_CR_PG;
    BootloaderFlash_ClearFlags();
  }

  return true;
}

bool BootloaderFlash_VerifyHalfWords(uint32_t address,
                                     const uint16_t *data,
                                     uint32_t halfword_count)
{
  uint32_t index;
  uint32_t length;

  if ((data == 0) || (halfword_count == 0U) || ((address & 1U) != 0U))
  {
    return false;
  }

  if (halfword_count > (0xFFFFFFFFU / 2U))
  {
    return false;
  }

  length = halfword_count * 2U;
  if (!BootloaderFlash_IsScratchAddressRange(address, length))
  {
    return false;
  }

  for (index = 0U; index < halfword_count; index++)
  {
    if (*(volatile const uint16_t *)(address + (index * 2U)) != data[index])
    {
      return false;
    }
  }

  return true;
}

bool BootloaderFlash_RunSelfTest(uint8_t *status, uint8_t *stage, uint8_t detail[5])
{
  bool ok = false;
  bool cleanup_needed = false;
  uint8_t result_status = BOOTLOADER_FLASH_STATUS_OK;
  uint8_t result_stage = BOOTLOADER_FLASH_STAGE_DONE;

  if ((status == 0) || (stage == 0) || (detail == 0))
  {
    return false;
  }

  BootloaderFlash_ClearDetail(detail);
  BootloaderFlash_ForceOutputsOff();

  if (!BootloaderFlash_IsScratchAddressRange(BOOTLOADER_FLASH_SCRATCH_PAGE_START,
                                             BOOTLOADER_FLASH_PAGE_SIZE_BYTES))
  {
    result_status = BOOTLOADER_FLASH_STATUS_ADDRESS_RANGE_ERROR;
    result_stage = BOOTLOADER_FLASH_STAGE_DONE;
    goto finish_without_lock;
  }

  result_stage = BOOTLOADER_FLASH_STAGE_UNLOCK;
  if (!BootloaderFlash_Unlock())
  {
    result_status = BOOTLOADER_FLASH_STATUS_UNLOCK_FAIL;
    BootloaderFlash_SetStatusDetail(detail);
    goto finish_without_lock;
  }

  result_stage = BOOTLOADER_FLASH_STAGE_ERASE;
  if (!BootloaderFlash_EraseScratchPage())
  {
    result_status = BOOTLOADER_FLASH_STATUS_ERASE_FAIL;
    BootloaderFlash_SetStatusDetail(detail);
    goto finish_with_lock;
  }

  result_stage = BOOTLOADER_FLASH_STAGE_ERASE_VERIFY;
  if (!BootloaderFlash_VerifyErased(BOOTLOADER_FLASH_SCRATCH_PAGE_START,
                                    BOOTLOADER_FLASH_PAGE_SIZE_BYTES))
  {
    result_status = BOOTLOADER_FLASH_STATUS_ERASE_VERIFY_FAIL;
    BootloaderFlash_SetStatusDetail(detail);
    goto finish_with_lock;
  }

  result_stage = BOOTLOADER_FLASH_STAGE_PROGRAM;
  cleanup_needed = true;
  if (!BootloaderFlash_ProgramHalfWords(BOOTLOADER_FLASH_SCRATCH_PAGE_START,
                                        bootloader_flash_test_pattern,
                                        (uint32_t)(sizeof(bootloader_flash_test_pattern) /
                                                   sizeof(bootloader_flash_test_pattern[0]))))
  {
    result_status = BOOTLOADER_FLASH_STATUS_PROGRAM_FAIL;
    BootloaderFlash_SetStatusDetail(detail);
    goto cleanup_with_lock;
  }

  result_stage = BOOTLOADER_FLASH_STAGE_PROGRAM_VERIFY;
  if (!BootloaderFlash_VerifyHalfWords(BOOTLOADER_FLASH_SCRATCH_PAGE_START,
                                       bootloader_flash_test_pattern,
                                       (uint32_t)(sizeof(bootloader_flash_test_pattern) /
                                                  sizeof(bootloader_flash_test_pattern[0]))))
  {
    result_status = BOOTLOADER_FLASH_STATUS_PROGRAM_VERIFY_FAIL;
    BootloaderFlash_SetStatusDetail(detail);
    goto cleanup_with_lock;
  }

  result_stage = BOOTLOADER_FLASH_STAGE_FINAL_ERASE;
  cleanup_needed = false;
  if (!BootloaderFlash_EraseScratchPage())
  {
    result_status = BOOTLOADER_FLASH_STATUS_ERASE_FAIL;
    BootloaderFlash_SetStatusDetail(detail);
    goto finish_with_lock;
  }

  if (!BootloaderFlash_VerifyErased(BOOTLOADER_FLASH_SCRATCH_PAGE_START,
                                    BOOTLOADER_FLASH_PAGE_SIZE_BYTES))
  {
    result_status = BOOTLOADER_FLASH_STATUS_ERASE_VERIFY_FAIL;
    BootloaderFlash_SetStatusDetail(detail);
    goto finish_with_lock;
  }

  ok = true;

cleanup_with_lock:
  if (cleanup_needed)
  {
    (void)BootloaderFlash_EraseScratchPage();
  }

finish_with_lock:
  if (!BootloaderFlash_Lock() && ok)
  {
    result_status = BOOTLOADER_FLASH_STATUS_LOCK_FAIL;
    result_stage = BOOTLOADER_FLASH_STAGE_LOCK;
    BootloaderFlash_SetStatusDetail(detail);
    ok = false;
  }

finish_without_lock:
  if (ok)
  {
    result_stage = BOOTLOADER_FLASH_STAGE_DONE;
  }

  *status = result_status;
  *stage = result_stage;
  BootloaderFlash_ForceOutputsOff();

  return ok;
}
