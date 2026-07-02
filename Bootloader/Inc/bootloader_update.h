#ifndef BOOTLOADER_UPDATE_H
#define BOOTLOADER_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BOOTLOADER_UPDATE_STATUS_OK                (0x00U)
#define BOOTLOADER_UPDATE_STATUS_UNKNOWN_COMMAND   (0x01U)
#define BOOTLOADER_UPDATE_STATUS_BAD_DLC           (0x02U)
#define BOOTLOADER_UPDATE_STATUS_BAD_MAGIC         (0x03U)
#define BOOTLOADER_UPDATE_STATUS_BAD_STATE         (0x04U)
#define BOOTLOADER_UPDATE_STATUS_SIZE_ERROR        (0x05U)
#define BOOTLOADER_UPDATE_STATUS_ADDRESS_RANGE_ERROR (0x06U)
#define BOOTLOADER_UPDATE_STATUS_FLASH_UNLOCK_FAIL (0x07U)
#define BOOTLOADER_UPDATE_STATUS_ERASE_FAIL        (0x08U)
#define BOOTLOADER_UPDATE_STATUS_PROGRAM_FAIL      (0x09U)
#define BOOTLOADER_UPDATE_STATUS_VERIFY_FAIL       (0x0AU)
#define BOOTLOADER_UPDATE_STATUS_BAD_SEQUENCE      (0x0BU)
#define BOOTLOADER_UPDATE_STATUS_CRC_MISMATCH      (0x0CU)
#define BOOTLOADER_UPDATE_STATUS_APP_INVALID       (0x0DU)
#define BOOTLOADER_UPDATE_STATUS_METADATA_FAIL     (0x0EU)

typedef enum
{
  BOOTLOADER_UPDATE_STATE_IDLE = 0,
  BOOTLOADER_UPDATE_STATE_STARTED = 1,
  BOOTLOADER_UPDATE_STATE_ERASED = 2,
  BOOTLOADER_UPDATE_STATE_WRITING = 3,
  BOOTLOADER_UPDATE_STATE_WRITE_COMPLETE = 4,
  BOOTLOADER_UPDATE_STATE_CRC_OK = 5,
  BOOTLOADER_UPDATE_STATE_FINISHED = 6,
  BOOTLOADER_UPDATE_STATE_ERROR = 7,
} bootloader_update_state_t;

void BootloaderUpdate_Init(void);
bootloader_update_state_t BootloaderUpdate_GetState(void);
bool BootloaderUpdate_IsApplicationValid(uint32_t *app_sp, uint32_t *app_reset);
uint32_t BootloaderUpdate_Crc32Memory(uint32_t address, uint32_t length);

uint8_t BootloaderUpdate_Start(uint32_t app_size, uint8_t flags, uint8_t *stage);
uint8_t BootloaderUpdate_EraseApp(uint8_t *stage, uint16_t *erased_pages);
uint8_t BootloaderUpdate_WriteChunk(uint16_t sequence,
                                    uint8_t length,
                                    const uint8_t payload[4],
                                    uint16_t *next_sequence);
uint8_t BootloaderUpdate_VerifyCrc(uint32_t expected_crc, uint32_t *actual_crc);
uint8_t BootloaderUpdate_Finish(void);
uint8_t BootloaderUpdate_Abort(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_UPDATE_H */
