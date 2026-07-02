#include "bootloader_flash.h"

#include "stm32f1xx.h"

#define BL_FLASH_TIMEOUT                 (1000000U)
#define BL_FLASH_ERROR_FLAGS             (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)
#define BL_FLASH_CLEAR_FLAGS             (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR)

static void BootloaderFlash_ClearDetail(uint8_t detail[5])
{
  for (uint8_t index = 0U; index < 5U; index++)
  {
    detail[index] = 0U;
  }
}

static bool BootloaderFlash_IsAddressRange(uint32_t address,
                                           uint32_t length,
                                           uint32_t start,
                                           uint32_t end)
{
  uint32_t end_address;

  if (length == 0U)
  {
    return false;
  }

  if (address < start)
  {
    return false;
  }

  end_address = address + length - 1U;
  if (end_address < address)
  {
    return false;
  }

  return end_address <= end;
}

bool BootloaderFlash_IsScratchAddressRange(uint32_t address, uint32_t length)
{
  return BootloaderFlash_IsMetadataAddressRange(address, length);
}

bool BootloaderFlash_IsAppAddressRange(uint32_t address, uint32_t length)
{
  return BootloaderFlash_IsAddressRange(address,
                                        length,
                                        BOOTLOADER_FLASH_APP_BASE_ADDR,
                                        BOOTLOADER_FLASH_APP_END_ADDR);
}

bool BootloaderFlash_IsMetadataAddressRange(uint32_t address, uint32_t length)
{
  return BootloaderFlash_IsAddressRange(address,
                                        length,
                                        BOOTLOADER_FLASH_METADATA_PAGE_START,
                                        BOOTLOADER_FLASH_METADATA_PAGE_END);
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

  return (FLASH->SR & FLASH_SR_BSY) == 0U;
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

static bool BootloaderFlash_ErasePageUnlocked(uint32_t page_address)
{
  if ((page_address % BOOTLOADER_FLASH_PAGE_SIZE_BYTES) != 0U)
  {
    return false;
  }

  if (!BootloaderFlash_WaitNotBusy())
  {
    return false;
  }

  BootloaderFlash_ClearFlags();

  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = page_address;
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

static bool BootloaderFlash_ProgramHalfWordUnlocked(uint32_t address, uint16_t value)
{
  if ((address & 1U) != 0U)
  {
    return false;
  }

  if (!BootloaderFlash_WaitNotBusy())
  {
    return false;
  }

  BootloaderFlash_ClearFlags();
  FLASH->CR |= FLASH_CR_PG;
  *(volatile uint16_t *)address = value;

  if (!BootloaderFlash_WaitReady())
  {
    FLASH->CR &= ~FLASH_CR_PG;
    return false;
  }

  FLASH->CR &= ~FLASH_CR_PG;
  BootloaderFlash_ClearFlags();

  return true;
}

static bool BootloaderFlash_ProgramBytesInRange(uint32_t address,
                                                const uint8_t *data,
                                                uint32_t length,
                                                bool metadata_region)
{
  uint32_t padded_length;
  uint32_t offset;
  bool ok = false;

  if ((data == 0) || (length == 0U) || ((address & 1U) != 0U))
  {
    return false;
  }

  padded_length = (length + 1U) & ~1U;
  if (padded_length < length)
  {
    return false;
  }

  if (metadata_region)
  {
    if (!BootloaderFlash_IsMetadataAddressRange(address, padded_length))
    {
      return false;
    }
  }
  else if (!BootloaderFlash_IsAppAddressRange(address, padded_length))
  {
    return false;
  }

  if (!BootloaderFlash_Unlock())
  {
    return false;
  }

  for (offset = 0U; offset < padded_length; offset += 2U)
  {
    uint16_t value = data[offset];

    if ((offset + 1U) < length)
    {
      value |= (uint16_t)data[offset + 1U] << 8U;
    }
    else
    {
      value |= 0xFF00U;
    }

    if (!BootloaderFlash_ProgramHalfWordUnlocked(address + offset, value))
    {
      goto finish;
    }
  }

  ok = true;

finish:
  return BootloaderFlash_Lock() && ok;
}

bool BootloaderFlash_EraseScratchPage(void)
{
  return false;
}

bool BootloaderFlash_EraseAppPages(uint32_t app_size, uint16_t *erased_pages)
{
  uint32_t page_count;
  uint32_t page_index;
  bool ok = false;

  if ((app_size == 0U) || (app_size > BOOTLOADER_FLASH_APP_MAX_SIZE_BYTES))
  {
    return false;
  }

  page_count = (app_size + BOOTLOADER_FLASH_PAGE_SIZE_BYTES - 1U) /
               BOOTLOADER_FLASH_PAGE_SIZE_BYTES;

  if (!BootloaderFlash_IsAppAddressRange(BOOTLOADER_FLASH_APP_BASE_ADDR,
                                         page_count * BOOTLOADER_FLASH_PAGE_SIZE_BYTES))
  {
    return false;
  }

  if (erased_pages != 0)
  {
    *erased_pages = 0U;
  }

  if (!BootloaderFlash_Unlock())
  {
    return false;
  }

  for (page_index = 0U; page_index < page_count; page_index++)
  {
    uint32_t page_address = BOOTLOADER_FLASH_APP_BASE_ADDR +
                            (page_index * BOOTLOADER_FLASH_PAGE_SIZE_BYTES);

    if (!BootloaderFlash_ErasePageUnlocked(page_address))
    {
      goto finish;
    }

    if (erased_pages != 0)
    {
      *erased_pages = (uint16_t)(*erased_pages + 1U);
    }
  }

  ok = true;

finish:
  return BootloaderFlash_Lock() && ok;
}

bool BootloaderFlash_EraseMetadataPage(void)
{
  bool ok = false;

  if (!BootloaderFlash_IsMetadataAddressRange(BOOTLOADER_FLASH_METADATA_PAGE_START,
                                              BOOTLOADER_FLASH_PAGE_SIZE_BYTES))
  {
    return false;
  }

  if (!BootloaderFlash_Unlock())
  {
    return false;
  }

  ok = BootloaderFlash_ErasePageUnlocked(BOOTLOADER_FLASH_METADATA_PAGE_START);

  return BootloaderFlash_Lock() && ok;
}

bool BootloaderFlash_VerifyErased(uint32_t address, uint32_t length)
{
  uint32_t offset;

  if (((address & 1U) != 0U) || ((length & 1U) != 0U))
  {
    return false;
  }

  if (!BootloaderFlash_IsAppAddressRange(address, length) &&
      !BootloaderFlash_IsMetadataAddressRange(address, length))
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

bool BootloaderFlash_ProgramAppBytes(uint32_t address,
                                     const uint8_t *data,
                                     uint32_t length)
{
  return BootloaderFlash_ProgramBytesInRange(address, data, length, false);
}

bool BootloaderFlash_ProgramMetadataBytes(uint32_t address,
                                          const uint8_t *data,
                                          uint32_t length)
{
  return BootloaderFlash_ProgramBytesInRange(address, data, length, true);
}

bool BootloaderFlash_VerifyBytes(uint32_t address,
                                 const uint8_t *data,
                                 uint32_t length)
{
  uint32_t offset;

  if ((data == 0) || (length == 0U))
  {
    return false;
  }

  if (!BootloaderFlash_IsAppAddressRange(address, length) &&
      !BootloaderFlash_IsMetadataAddressRange(address, length))
  {
    return false;
  }

  for (offset = 0U; offset < length; offset++)
  {
    if (*(volatile const uint8_t *)(address + offset) != data[offset])
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
  return BootloaderFlash_ProgramMetadataBytes(address,
                                             (const uint8_t *)data,
                                             halfword_count * 2U);
}

bool BootloaderFlash_VerifyHalfWords(uint32_t address,
                                     const uint16_t *data,
                                     uint32_t halfword_count)
{
  return BootloaderFlash_VerifyBytes(address,
                                     (const uint8_t *)data,
                                     halfword_count * 2U);
}

bool BootloaderFlash_RunSelfTest(uint8_t *status, uint8_t *stage, uint8_t detail[5])
{
  if ((status == 0) || (stage == 0) || (detail == 0))
  {
    return false;
  }

  BootloaderFlash_ClearDetail(detail);
  *status = BOOTLOADER_FLASH_STATUS_ADDRESS_RANGE_ERROR;
  *stage = BOOTLOADER_FLASH_STAGE_DONE;

  return false;
}
