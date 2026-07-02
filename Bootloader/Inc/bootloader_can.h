#ifndef BOOTLOADER_CAN_H
#define BOOTLOADER_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

bool BootloaderCan_ConfigSystemClock(void);
uint8_t BootloaderCan_ReadNodeId(void);
bool BootloaderCan_Init(uint8_t node_id);
void BootloaderCan_Task(bool app_valid);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_CAN_H */
