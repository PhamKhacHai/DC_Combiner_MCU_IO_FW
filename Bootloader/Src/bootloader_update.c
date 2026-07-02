#include "bootloader_update.h"

#include "bootloader_flash.h"

#define BL_METADATA_MAGIC              (0xA55A5AA5U)
#define BL_METADATA_VERSION            (0x00010000U)
#define BL_METADATA_STATE_VALID        (0x56414C44U)
#define BL_METADATA_STATE_IN_PROGRESS  (0x50524F47U)
#define BL_METADATA_STATE_INVALID      (0x494E5644U)

#define BL_SRAM_START_ADDR             (0x20000000U)
#define BL_SRAM_END_ADDR               (0x20005000U)
#define BL_CRC32_POLY_REFLECTED        (0xEDB88320U)
#define BL_CRC32_INIT                  (0xFFFFFFFFU)
#define BL_CRC32_FINAL_XOR             (0xFFFFFFFFU)

typedef struct
{
  uint32_t magic;
  uint32_t app_base;
  uint32_t app_size;
  uint32_t app_crc32;
  uint32_t version;
  uint32_t state;
  uint32_t reserved[2];
} bootloader_app_metadata_t;

typedef struct
{
  bootloader_update_state_t state;
  uint32_t app_size;
  uint32_t received_bytes;
  uint32_t verified_crc;
  uint16_t expected_sequence;
} bootloader_update_session_t;

static bootloader_update_session_t update_session;

static void BootloaderUpdate_ResetSession(bootloader_update_state_t state)
{
  update_session.state = state;
  update_session.app_size = 0U;
  update_session.received_bytes = 0U;
  update_session.verified_crc = 0U;
  update_session.expected_sequence = 0U;
}

void BootloaderUpdate_Init(void)
{
  BootloaderUpdate_ResetSession(BOOTLOADER_UPDATE_STATE_IDLE);
}

bootloader_update_state_t BootloaderUpdate_GetState(void)
{
  return update_session.state;
}

static uint32_t BootloaderUpdate_ReadWord(uint32_t address)
{
  return *(volatile const uint32_t *)address;
}

static bool BootloaderUpdate_IsMetadataBlank(void)
{
  uint32_t address;

  for (address = BOOTLOADER_FLASH_METADATA_PAGE_START;
       address <= BOOTLOADER_FLASH_METADATA_PAGE_END;
       address += sizeof(uint32_t))
  {
    if (BootloaderUpdate_ReadWord(address) != 0xFFFFFFFFU)
    {
      return false;
    }
  }

  return true;
}

static void BootloaderUpdate_ReadMetadata(bootloader_app_metadata_t *metadata)
{
  const bootloader_app_metadata_t *flash_metadata =
      (const bootloader_app_metadata_t *)BOOTLOADER_FLASH_METADATA_PAGE_START;

  *metadata = *flash_metadata;
}

static bool BootloaderUpdate_IsValidSize(uint32_t app_size)
{
  return (app_size > 8U) &&
         (app_size <= BOOTLOADER_FLASH_APP_MAX_SIZE_BYTES);
}

static bool BootloaderUpdate_IsVectorTableValid(uint32_t app_limit,
                                                uint32_t *app_sp,
                                                uint32_t *app_reset)
{
  uint32_t initial_sp = BootloaderUpdate_ReadWord(BOOTLOADER_FLASH_APP_BASE_ADDR);
  uint32_t reset_handler = BootloaderUpdate_ReadWord(BOOTLOADER_FLASH_APP_BASE_ADDR + 4U);
  uint32_t reset_address = reset_handler & ~1UL;

  if ((initial_sp == 0x00000000U) || (initial_sp == 0xFFFFFFFFU))
  {
    return false;
  }

  if ((reset_handler == 0x00000000U) || (reset_handler == 0xFFFFFFFFU))
  {
    return false;
  }

  if ((initial_sp < BL_SRAM_START_ADDR) || (initial_sp > BL_SRAM_END_ADDR))
  {
    return false;
  }

  if ((initial_sp & 0x3U) != 0U)
  {
    return false;
  }

  if ((reset_handler & 1U) == 0U)
  {
    return false;
  }

  if ((reset_address < BOOTLOADER_FLASH_APP_BASE_ADDR) || (reset_address > app_limit))
  {
    return false;
  }

  if (app_sp != 0)
  {
    *app_sp = initial_sp;
  }

  if (app_reset != 0)
  {
    *app_reset = reset_handler;
  }

  return true;
}

uint32_t BootloaderUpdate_Crc32Memory(uint32_t address, uint32_t length)
{
  uint32_t crc = BL_CRC32_INIT;
  uint32_t offset;

  for (offset = 0U; offset < length; offset++)
  {
    uint32_t byte_value = *(volatile const uint8_t *)(address + offset);
    crc ^= byte_value;

    for (uint8_t bit = 0U; bit < 8U; bit++)
    {
      if ((crc & 1U) != 0U)
      {
        crc = (crc >> 1U) ^ BL_CRC32_POLY_REFLECTED;
      }
      else
      {
        crc >>= 1U;
      }
    }
  }

  return crc ^ BL_CRC32_FINAL_XOR;
}

static bool BootloaderUpdate_WriteMetadata(uint32_t app_size,
                                           uint32_t app_crc32,
                                           uint32_t state)
{
  bootloader_app_metadata_t metadata = {
      BL_METADATA_MAGIC,
      BOOTLOADER_FLASH_APP_BASE_ADDR,
      app_size,
      app_crc32,
      BL_METADATA_VERSION,
      state,
      {0xFFFFFFFFU, 0xFFFFFFFFU},
  };

  if (!BootloaderFlash_EraseMetadataPage())
  {
    return false;
  }

  if (!BootloaderFlash_ProgramMetadataBytes(BOOTLOADER_FLASH_METADATA_PAGE_START,
                                            (const uint8_t *)&metadata,
                                            sizeof(metadata)))
  {
    return false;
  }

  return BootloaderFlash_VerifyBytes(BOOTLOADER_FLASH_METADATA_PAGE_START,
                                     (const uint8_t *)&metadata,
                                     sizeof(metadata));
}

bool BootloaderUpdate_IsApplicationValid(uint32_t *app_sp, uint32_t *app_reset)
{
  bootloader_app_metadata_t metadata;
  uint32_t app_limit;
  uint32_t actual_crc;

  if (BootloaderUpdate_IsMetadataBlank())
  {
    return BootloaderUpdate_IsVectorTableValid(BOOTLOADER_FLASH_APP_END_ADDR,
                                               app_sp,
                                               app_reset);
  }

  BootloaderUpdate_ReadMetadata(&metadata);

  if ((metadata.magic != BL_METADATA_MAGIC) ||
      (metadata.app_base != BOOTLOADER_FLASH_APP_BASE_ADDR))
  {
    return false;
  }

  if ((metadata.state == BL_METADATA_STATE_IN_PROGRESS) ||
      (metadata.state == BL_METADATA_STATE_INVALID))
  {
    return false;
  }

  if (metadata.state != BL_METADATA_STATE_VALID)
  {
    return false;
  }

  if (!BootloaderUpdate_IsValidSize(metadata.app_size))
  {
    return false;
  }

  app_limit = BOOTLOADER_FLASH_APP_BASE_ADDR + metadata.app_size - 1U;
  if (app_limit > BOOTLOADER_FLASH_APP_END_ADDR)
  {
    return false;
  }

  if (!BootloaderUpdate_IsVectorTableValid(app_limit, app_sp, app_reset))
  {
    return false;
  }

  actual_crc = BootloaderUpdate_Crc32Memory(BOOTLOADER_FLASH_APP_BASE_ADDR,
                                            metadata.app_size);

  return actual_crc == metadata.app_crc32;
}

uint8_t BootloaderUpdate_Start(uint32_t app_size, uint8_t flags, uint8_t *stage)
{
  if (stage != 0)
  {
    *stage = (uint8_t)update_session.state;
  }

  if (!BootloaderUpdate_IsValidSize(app_size))
  {
    return BOOTLOADER_UPDATE_STATUS_SIZE_ERROR;
  }

  if (flags != 0U)
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
  }

  if ((update_session.state == BOOTLOADER_UPDATE_STATE_STARTED) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_ERASED) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_WRITING) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_WRITE_COMPLETE) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_CRC_OK))
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
  }

  if (!BootloaderUpdate_WriteMetadata(app_size, 0xFFFFFFFFU, BL_METADATA_STATE_IN_PROGRESS))
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    if (stage != 0)
    {
      *stage = (uint8_t)update_session.state;
    }
    return BOOTLOADER_UPDATE_STATUS_METADATA_FAIL;
  }

  update_session.state = BOOTLOADER_UPDATE_STATE_STARTED;
  update_session.app_size = app_size;
  update_session.received_bytes = 0U;
  update_session.verified_crc = 0U;
  update_session.expected_sequence = 0U;

  if (stage != 0)
  {
    *stage = (uint8_t)update_session.state;
  }

  return BOOTLOADER_UPDATE_STATUS_OK;
}

uint8_t BootloaderUpdate_EraseApp(uint8_t *stage, uint16_t *erased_pages)
{
  uint16_t pages = 0U;

  if (stage != 0)
  {
    *stage = (uint8_t)update_session.state;
  }

  if (update_session.state != BOOTLOADER_UPDATE_STATE_STARTED)
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
  }

  if (!BootloaderFlash_EraseAppPages(update_session.app_size, &pages))
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    if (stage != 0)
    {
      *stage = (uint8_t)update_session.state;
    }
    return BOOTLOADER_UPDATE_STATUS_ERASE_FAIL;
  }

  update_session.state = BOOTLOADER_UPDATE_STATE_ERASED;

  if (erased_pages != 0)
  {
    *erased_pages = pages;
  }

  if (stage != 0)
  {
    *stage = (uint8_t)update_session.state;
  }

  return BOOTLOADER_UPDATE_STATUS_OK;
}

uint8_t BootloaderUpdate_WriteChunk(uint16_t sequence,
                                    uint8_t length,
                                    const uint8_t payload[4],
                                    uint16_t *next_sequence)
{
  uint32_t write_address;
  uint32_t remaining;
  uint32_t padded_length;

  if (next_sequence != 0)
  {
    *next_sequence = update_session.expected_sequence;
  }

  if ((payload == 0) || (length == 0U) || (length > 4U))
  {
    return BOOTLOADER_UPDATE_STATUS_SIZE_ERROR;
  }

  if ((update_session.state != BOOTLOADER_UPDATE_STATE_ERASED) &&
      (update_session.state != BOOTLOADER_UPDATE_STATE_WRITING))
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
  }

  if (sequence != update_session.expected_sequence)
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_SEQUENCE;
  }

  if (update_session.received_bytes >= update_session.app_size)
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
  }

  remaining = update_session.app_size - update_session.received_bytes;
  if (length > remaining)
  {
    return BOOTLOADER_UPDATE_STATUS_ADDRESS_RANGE_ERROR;
  }

  write_address = BOOTLOADER_FLASH_APP_BASE_ADDR + ((uint32_t)sequence * 4U);
  padded_length = ((uint32_t)length + 1U) & ~1U;

  if (!BootloaderFlash_IsAppAddressRange(write_address, padded_length))
  {
    return BOOTLOADER_UPDATE_STATUS_ADDRESS_RANGE_ERROR;
  }

  if (!BootloaderFlash_ProgramAppBytes(write_address, payload, length))
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    return BOOTLOADER_UPDATE_STATUS_PROGRAM_FAIL;
  }

  if (!BootloaderFlash_VerifyBytes(write_address, payload, length))
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    return BOOTLOADER_UPDATE_STATUS_VERIFY_FAIL;
  }

  update_session.received_bytes += length;
  update_session.expected_sequence++;

  if (next_sequence != 0)
  {
    *next_sequence = update_session.expected_sequence;
  }

  if (update_session.received_bytes == update_session.app_size)
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_WRITE_COMPLETE;
  }
  else
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_WRITING;
  }

  return BOOTLOADER_UPDATE_STATUS_OK;
}

uint8_t BootloaderUpdate_VerifyCrc(uint32_t expected_crc, uint32_t *actual_crc)
{
  uint32_t crc;

  if (update_session.state != BOOTLOADER_UPDATE_STATE_WRITE_COMPLETE)
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
  }

  if (!BootloaderFlash_IsAppAddressRange(BOOTLOADER_FLASH_APP_BASE_ADDR,
                                         update_session.app_size))
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    return BOOTLOADER_UPDATE_STATUS_ADDRESS_RANGE_ERROR;
  }

  crc = BootloaderUpdate_Crc32Memory(BOOTLOADER_FLASH_APP_BASE_ADDR,
                                     update_session.app_size);
  update_session.verified_crc = crc;

  if (actual_crc != 0)
  {
    *actual_crc = crc;
  }

  if (crc != expected_crc)
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    return BOOTLOADER_UPDATE_STATUS_CRC_MISMATCH;
  }

  update_session.state = BOOTLOADER_UPDATE_STATE_CRC_OK;
  return BOOTLOADER_UPDATE_STATUS_OK;
}

uint8_t BootloaderUpdate_Finish(void)
{
  uint32_t app_sp = 0U;
  uint32_t app_reset = 0U;
  uint32_t app_limit;

  if (update_session.state != BOOTLOADER_UPDATE_STATE_CRC_OK)
  {
    return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
  }

  app_limit = BOOTLOADER_FLASH_APP_BASE_ADDR + update_session.app_size - 1U;
  if (!BootloaderUpdate_IsVectorTableValid(app_limit, &app_sp, &app_reset))
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    return BOOTLOADER_UPDATE_STATUS_APP_INVALID;
  }

  (void)app_sp;
  (void)app_reset;

  if (!BootloaderUpdate_WriteMetadata(update_session.app_size,
                                      update_session.verified_crc,
                                      BL_METADATA_STATE_VALID))
  {
    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    return BOOTLOADER_UPDATE_STATUS_METADATA_FAIL;
  }

  update_session.state = BOOTLOADER_UPDATE_STATE_FINISHED;
  return BOOTLOADER_UPDATE_STATUS_OK;
}

uint8_t BootloaderUpdate_Abort(void)
{
  if ((update_session.state == BOOTLOADER_UPDATE_STATE_STARTED) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_ERASED) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_WRITING) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_WRITE_COMPLETE) ||
      (update_session.state == BOOTLOADER_UPDATE_STATE_CRC_OK))
  {
    if (!BootloaderUpdate_WriteMetadata(update_session.app_size,
                                        0U,
                                        BL_METADATA_STATE_INVALID))
    {
      update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
      return BOOTLOADER_UPDATE_STATUS_METADATA_FAIL;
    }

    update_session.state = BOOTLOADER_UPDATE_STATE_ERROR;
    return BOOTLOADER_UPDATE_STATUS_OK;
  }

  return BOOTLOADER_UPDATE_STATUS_BAD_STATE;
}
