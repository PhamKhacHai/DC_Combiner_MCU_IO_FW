#ifndef BOOTLOADER_REQUEST_H
#define BOOTLOADER_REQUEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BOOTLOADER_REQUEST_CAN_ID_BASE      (0x550U)
#define BOOTLOADER_RESPONSE_CAN_ID_BASE     (0x560U)

#define BOOTLOADER_REQUEST_DLC              (8U)
#define BOOTLOADER_COMMAND_ENTER            (0x10U)
#define BOOTLOADER_COMMAND_GET_BOOT_INFO    (0x11U)
#define BOOTLOADER_COMMAND_GET_FLASH_LAYOUT (0x12U)
#define BOOTLOADER_COMMAND_RUN_FLASH_SELF_TEST (0x20U)
#define BOOTLOADER_ENTER_MAGIC0             (0xA5U)
#define BOOTLOADER_ENTER_MAGIC1             (0x5AU)

#define BOOTLOADER_RESPONSE_ENTER_ACK       (0x90U)
#define BOOTLOADER_RESPONSE_GET_BOOT_INFO   (0x91U)
#define BOOTLOADER_RESPONSE_GET_FLASH_LAYOUT (0x92U)
#define BOOTLOADER_RESPONSE_FLASH_SELF_TEST (0xA0U)
#define BOOTLOADER_RESPONSE_STATUS_OK       (0x00U)
#define BOOTLOADER_RESPONSE_STATUS_UNKNOWN_COMMAND (0x01U)
#define BOOTLOADER_RESPONSE_STATUS_BAD_DLC  (0x02U)

uint32_t BootloaderRequest_CanRequestId(uint8_t node_id);
uint32_t BootloaderRequest_CanResponseId(uint8_t node_id);

bool BootloaderRequest_IsEnterCommand(uint8_t dlc, const uint8_t data[8]);
bool BootloaderRequest_IsGetBootInfoCommand(uint8_t dlc, const uint8_t data[8]);
bool BootloaderRequest_IsGetFlashLayoutCommand(uint8_t dlc, const uint8_t data[8]);
bool BootloaderRequest_IsRunFlashSelfTestCommand(uint8_t dlc, const uint8_t data[8]);

void BootloaderRequest_Set(void);
bool BootloaderRequest_IsSet(void);
void BootloaderRequest_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_REQUEST_H */
