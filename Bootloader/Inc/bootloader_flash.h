#ifndef BOOTLOADER_FLASH_H
#define BOOTLOADER_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BOOTLOADER_FLASH_BASE_ADDR              (0x08000000U)
#define BOOTLOADER_FLASH_TOTAL_SIZE_KB          (64U)
#define BOOTLOADER_FLASH_PAGE_SIZE_BYTES        (1024U)
#define BOOTLOADER_FLASH_PAGE_SIZE_KB           (1U)

#define BOOTLOADER_FLASH_BOOT_SIZE_KB           (16U)
#define BOOTLOADER_FLASH_APP_SIZE_KB            (47U)

#define BOOTLOADER_FLASH_APP_BASE_ADDR          (0x08004000U)
#define BOOTLOADER_FLASH_APP_END_ADDR           (0x0800FBFFU)
#define BOOTLOADER_FLASH_APP_MAX_SIZE_BYTES     \
  (BOOTLOADER_FLASH_APP_SIZE_KB * 1024U)

#define BOOTLOADER_FLASH_METADATA_PAGE_START    (0x0800FC00U)
#define BOOTLOADER_FLASH_METADATA_PAGE_END      (0x0800FFFFU)
#define BOOTLOADER_FLASH_METADATA_PAGE_INDEX    (63U)

#define BOOTLOADER_FLASH_SCRATCH_PAGE_START     BOOTLOADER_FLASH_METADATA_PAGE_START
#define BOOTLOADER_FLASH_SCRATCH_PAGE_END       BOOTLOADER_FLASH_METADATA_PAGE_END
#define BOOTLOADER_FLASH_SCRATCH_PAGE_INDEX     BOOTLOADER_FLASH_METADATA_PAGE_INDEX

#define BOOTLOADER_FLASH_STATUS_OK              (0x00U)
#define BOOTLOADER_FLASH_STATUS_BAD_MAGIC       (0x01U)
#define BOOTLOADER_FLASH_STATUS_ADDRESS_RANGE_ERROR (0x02U)
#define BOOTLOADER_FLASH_STATUS_UNLOCK_FAIL     (0x03U)
#define BOOTLOADER_FLASH_STATUS_ERASE_FAIL      (0x04U)
#define BOOTLOADER_FLASH_STATUS_ERASE_VERIFY_FAIL (0x05U)
#define BOOTLOADER_FLASH_STATUS_PROGRAM_FAIL    (0x06U)
#define BOOTLOADER_FLASH_STATUS_PROGRAM_VERIFY_FAIL (0x07U)
#define BOOTLOADER_FLASH_STATUS_LOCK_FAIL       (0x08U)

#define BOOTLOADER_FLASH_STAGE_DONE             (0x00U)
#define BOOTLOADER_FLASH_STAGE_UNLOCK           (0x01U)
#define BOOTLOADER_FLASH_STAGE_ERASE            (0x02U)
#define BOOTLOADER_FLASH_STAGE_ERASE_VERIFY     (0x03U)
#define BOOTLOADER_FLASH_STAGE_PROGRAM          (0x04U)
#define BOOTLOADER_FLASH_STAGE_PROGRAM_VERIFY   (0x05U)
#define BOOTLOADER_FLASH_STAGE_FINAL_ERASE      (0x06U)
#define BOOTLOADER_FLASH_STAGE_LOCK             (0x07U)

bool BootloaderFlash_IsScratchAddressRange(uint32_t address, uint32_t length);
bool BootloaderFlash_IsAppAddressRange(uint32_t address, uint32_t length);
bool BootloaderFlash_IsMetadataAddressRange(uint32_t address, uint32_t length);
bool BootloaderFlash_EraseScratchPage(void);
bool BootloaderFlash_EraseAppPages(uint32_t app_size, uint16_t *erased_pages);
bool BootloaderFlash_EraseMetadataPage(void);
bool BootloaderFlash_VerifyErased(uint32_t address, uint32_t length);
bool BootloaderFlash_ProgramAppBytes(uint32_t address,
                                     const uint8_t *data,
                                     uint32_t length);
bool BootloaderFlash_ProgramMetadataBytes(uint32_t address,
                                          const uint8_t *data,
                                          uint32_t length);
bool BootloaderFlash_VerifyBytes(uint32_t address,
                                 const uint8_t *data,
                                 uint32_t length);
bool BootloaderFlash_ProgramHalfWords(uint32_t address,
                                      const uint16_t *data,
                                      uint32_t halfword_count);
bool BootloaderFlash_VerifyHalfWords(uint32_t address,
                                     const uint16_t *data,
                                     uint32_t halfword_count);
bool BootloaderFlash_RunSelfTest(uint8_t *status, uint8_t *stage, uint8_t detail[5]);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_FLASH_H */
