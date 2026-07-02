#include "bootloader_can.h"

#include "bootloader_flash.h"
#include "bootloader_request.h"
#include "bootloader_update.h"
#include "stm32f1xx.h"

#define BL_CAN_VERSION_MAJOR          (1U)
#define BL_CAN_VERSION_MINOR          (0U)
#define BL_CAN_BOOT_MODE_ACTIVE       (1U)

#define BL_CAN_STD_ID_MASK            (0x7FFU)
#define BL_CAN_FILTER_BANK0           (1UL << 0)

#define BL_CAN_CLOCK_TIMEOUT          (1000000U)
#define BL_CAN_INIT_TIMEOUT           (100000U)
#define BL_CAN_TX_TIMEOUT             (100000U)

#define BL_GPIO_INPUT_FLOATING        (0x4U)
#define BL_GPIO_AF_PP_50MHZ           (0xBU)

#define BL_DIP1_PIN                   (5U)
#define BL_DIP2_PIN                   (6U)
#define BL_DIP3_PIN                   (7U)
#define BL_DIP_NODE_ID_MASK           (0x07U)

static uint8_t bootloader_node_id;
static uint32_t bootloader_request_id;
static uint32_t bootloader_response_id;

extern uint32_t SystemCoreClock;

static uint32_t BootloaderCan_ReadLe32(const uint8_t *data)
{
  return ((uint32_t)data[0]) |
         ((uint32_t)data[1] << 8U) |
         ((uint32_t)data[2] << 16U) |
         ((uint32_t)data[3] << 24U);
}

static uint16_t BootloaderCan_ReadLe16(const uint8_t *data)
{
  return (uint16_t)(((uint16_t)data[0]) | ((uint16_t)data[1] << 8U));
}

static void BootloaderCan_WriteLe16(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)(value & 0xFFU);
  data[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static void BootloaderCan_WriteLe32(uint8_t *data, uint32_t value)
{
  data[0] = (uint8_t)(value & 0xFFU);
  data[1] = (uint8_t)((value >> 8U) & 0xFFU);
  data[2] = (uint8_t)((value >> 16U) & 0xFFU);
  data[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

static bool BootloaderCan_WaitSet(volatile uint32_t *reg, uint32_t mask, uint32_t timeout)
{
  while (((*reg & mask) == 0U) && (timeout > 0U))
  {
    timeout--;
  }

  return (*reg & mask) != 0U;
}

static bool BootloaderCan_WaitClear(volatile uint32_t *reg, uint32_t mask, uint32_t timeout)
{
  while (((*reg & mask) != 0U) && (timeout > 0U))
  {
    timeout--;
  }

  return (*reg & mask) == 0U;
}

static void BootloaderCan_ConfigGpioPin(GPIO_TypeDef *port, uint8_t pin, uint32_t mode)
{
  volatile uint32_t *config_reg;
  uint32_t shift;
  uint32_t value;

  if (pin < 8U)
  {
    config_reg = &port->CRL;
    shift = (uint32_t)pin * 4U;
  }
  else
  {
    config_reg = &port->CRH;
    shift = ((uint32_t)pin - 8U) * 4U;
  }

  value = *config_reg;
  value &= ~(0xFU << shift);
  value |= (mode & 0xFU) << shift;
  *config_reg = value;
}

bool BootloaderCan_ConfigSystemClock(void)
{
  RCC->CR |= RCC_CR_HSION;
  if (!BootloaderCan_WaitSet(&RCC->CR, RCC_CR_HSIRDY, BL_CAN_CLOCK_TIMEOUT))
  {
    return false;
  }

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
  if (!BootloaderCan_WaitClear(&RCC->CFGR, RCC_CFGR_SWS, BL_CAN_CLOCK_TIMEOUT))
  {
    return false;
  }

  RCC->CR &= ~RCC_CR_PLLON;
  if (!BootloaderCan_WaitClear(&RCC->CR, RCC_CR_PLLRDY, BL_CAN_CLOCK_TIMEOUT))
  {
    return false;
  }

  RCC->CR |= RCC_CR_HSEON;
  if (!BootloaderCan_WaitSet(&RCC->CR, RCC_CR_HSERDY, BL_CAN_CLOCK_TIMEOUT))
  {
    return false;
  }

  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
               FLASH_ACR_PRFTBE |
               FLASH_ACR_LATENCY_1;

  RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE |
                             RCC_CFGR_PPRE1 |
                             RCC_CFGR_PPRE2 |
                             RCC_CFGR_PLLSRC |
                             RCC_CFGR_PLLXTPRE |
                             RCC_CFGR_PLLMULL)) |
              RCC_CFGR_PPRE1_DIV2 |
              RCC_CFGR_PLLSRC |
              RCC_CFGR_PLLMULL9;

  RCC->CR |= RCC_CR_PLLON;
  if (!BootloaderCan_WaitSet(&RCC->CR, RCC_CR_PLLRDY, BL_CAN_CLOCK_TIMEOUT))
  {
    return false;
  }

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
  if (!BootloaderCan_WaitSet(&RCC->CFGR, RCC_CFGR_SWS_PLL, BL_CAN_CLOCK_TIMEOUT))
  {
    return false;
  }

  SystemCoreClock = 72000000U;
  return true;
}

uint8_t BootloaderCan_ReadNodeId(void)
{
  uint8_t node_id = 0U;

  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
  (void)RCC->APB2ENR;

  BootloaderCan_ConfigGpioPin(GPIOA, BL_DIP1_PIN, BL_GPIO_INPUT_FLOATING);
  BootloaderCan_ConfigGpioPin(GPIOA, BL_DIP2_PIN, BL_GPIO_INPUT_FLOATING);
  BootloaderCan_ConfigGpioPin(GPIOA, BL_DIP3_PIN, BL_GPIO_INPUT_FLOATING);

  if ((GPIOA->IDR & (1UL << BL_DIP1_PIN)) == 0U)
  {
    node_id |= 1U << 0;
  }

  if ((GPIOA->IDR & (1UL << BL_DIP2_PIN)) == 0U)
  {
    node_id |= 1U << 1;
  }

  if ((GPIOA->IDR & (1UL << BL_DIP3_PIN)) == 0U)
  {
    node_id |= 1U << 2;
  }

  return node_id & BL_DIP_NODE_ID_MASK;
}

static bool BootloaderCan_EnterInitMode(void)
{
  CAN1->MCR |= CAN_MCR_INRQ;
  return BootloaderCan_WaitSet(&CAN1->MSR, CAN_MSR_INAK, BL_CAN_INIT_TIMEOUT);
}

static bool BootloaderCan_LeaveInitMode(void)
{
  CAN1->MCR &= ~CAN_MCR_INRQ;
  return BootloaderCan_WaitClear(&CAN1->MSR, CAN_MSR_INAK, BL_CAN_INIT_TIMEOUT);
}

static void BootloaderCan_ConfigFilter(uint32_t std_id)
{
  CAN1->FMR = (CAN1->FMR & ~CAN_FMR_CAN2SB) |
              CAN_FMR_FINIT |
              (14U << CAN_FMR_CAN2SB_Pos);

  CAN1->FA1R &= ~BL_CAN_FILTER_BANK0;
  CAN1->FM1R &= ~BL_CAN_FILTER_BANK0;
  CAN1->FS1R |= BL_CAN_FILTER_BANK0;
  CAN1->FFA1R &= ~BL_CAN_FILTER_BANK0;

  CAN1->sFilterRegister[0].FR1 = (std_id & BL_CAN_STD_ID_MASK) << CAN_TI0R_STID_Pos;
  CAN1->sFilterRegister[0].FR2 = BL_CAN_STD_ID_MASK << CAN_TI0R_STID_Pos;
  CAN1->FA1R |= BL_CAN_FILTER_BANK0;

  CAN1->FMR &= ~CAN_FMR_FINIT;
}

bool BootloaderCan_Init(uint8_t node_id)
{
  bootloader_node_id = node_id & BL_DIP_NODE_ID_MASK;
  bootloader_request_id = BootloaderRequest_CanRequestId(bootloader_node_id);
  bootloader_response_id = BootloaderRequest_CanResponseId(bootloader_node_id);

  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
  RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
  (void)RCC->APB2ENR;
  (void)RCC->APB1ENR;

  BootloaderCan_ConfigGpioPin(GPIOA, 11U, BL_GPIO_INPUT_FLOATING);
  BootloaderCan_ConfigGpioPin(GPIOA, 12U, BL_GPIO_AF_PP_50MHZ);

  RCC->APB1RSTR |= RCC_APB1RSTR_CAN1RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_CAN1RST;

  if (!BootloaderCan_EnterInitMode())
  {
    return false;
  }

  CAN1->MCR = CAN_MCR_INRQ | CAN_MCR_ABOM;
  CAN1->IER = 0U;
  CAN1->BTR = (3U << CAN_BTR_BRP_Pos) |
              (12U << CAN_BTR_TS1_Pos) |
              (3U << CAN_BTR_TS2_Pos);

  BootloaderCan_ConfigFilter(bootloader_request_id);

  return BootloaderCan_LeaveInitMode();
}

static bool BootloaderCan_ReadFrame(uint32_t *std_id, uint8_t *dlc, uint8_t data[8])
{
  uint32_t rir;
  uint32_t rdtr;
  uint32_t rdlr;
  uint32_t rdhr;

  if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0U)
  {
    return false;
  }

  rir = CAN1->sFIFOMailBox[0].RIR;
  rdtr = CAN1->sFIFOMailBox[0].RDTR;
  rdlr = CAN1->sFIFOMailBox[0].RDLR;
  rdhr = CAN1->sFIFOMailBox[0].RDHR;
  CAN1->RF0R = CAN_RF0R_RFOM0;

  if (((rir & CAN_RI0R_IDE) != 0U) || ((rir & CAN_RI0R_RTR) != 0U))
  {
    return false;
  }

  *std_id = (rir & CAN_RI0R_STID) >> CAN_RI0R_STID_Pos;
  *dlc = (uint8_t)(rdtr & CAN_RDT0R_DLC);

  data[0] = (uint8_t)(rdlr & 0xFFU);
  data[1] = (uint8_t)((rdlr >> 8U) & 0xFFU);
  data[2] = (uint8_t)((rdlr >> 16U) & 0xFFU);
  data[3] = (uint8_t)((rdlr >> 24U) & 0xFFU);
  data[4] = (uint8_t)(rdhr & 0xFFU);
  data[5] = (uint8_t)((rdhr >> 8U) & 0xFFU);
  data[6] = (uint8_t)((rdhr >> 16U) & 0xFFU);
  data[7] = (uint8_t)((rdhr >> 24U) & 0xFFU);

  return true;
}

static bool BootloaderCan_SendFrame(uint32_t std_id, const uint8_t data[8])
{
  uint32_t timeout = BL_CAN_TX_TIMEOUT;
  bool tx_ok;

  while (((CAN1->TSR & CAN_TSR_TME0) == 0U) && (timeout > 0U))
  {
    timeout--;
  }

  if ((CAN1->TSR & CAN_TSR_TME0) == 0U)
  {
    return false;
  }

  CAN1->sTxMailBox[0].TIR = (std_id & BL_CAN_STD_ID_MASK) << CAN_TI0R_STID_Pos;
  CAN1->sTxMailBox[0].TDTR = BOOTLOADER_REQUEST_DLC;
  CAN1->sTxMailBox[0].TDLR = ((uint32_t)data[0]) |
                              ((uint32_t)data[1] << 8U) |
                              ((uint32_t)data[2] << 16U) |
                              ((uint32_t)data[3] << 24U);
  CAN1->sTxMailBox[0].TDHR = ((uint32_t)data[4]) |
                              ((uint32_t)data[5] << 8U) |
                              ((uint32_t)data[6] << 16U) |
                              ((uint32_t)data[7] << 24U);
  CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;

  timeout = BL_CAN_TX_TIMEOUT;
  while (((CAN1->TSR & CAN_TSR_RQCP0) == 0U) && (timeout > 0U))
  {
    timeout--;
  }

  if ((CAN1->TSR & CAN_TSR_RQCP0) == 0U)
  {
    CAN1->TSR = CAN_TSR_ABRQ0;
    return false;
  }

  tx_ok = (CAN1->TSR & CAN_TSR_TXOK0) != 0U;
  CAN1->TSR = CAN_TSR_RQCP0;

  return tx_ok;
}

static void BootloaderCan_SendBootInfo(uint8_t status)
{
  bool app_valid = BootloaderUpdate_IsApplicationValid(0, 0);
  uint8_t response[8] = {
      BOOTLOADER_RESPONSE_GET_BOOT_INFO,
      status,
      BL_CAN_VERSION_MAJOR,
      BL_CAN_VERSION_MINOR,
      app_valid ? 1U : 0U,
      BL_CAN_BOOT_MODE_ACTIVE,
      0U,
      0U,
  };

  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendFlashLayout(void)
{
  uint8_t response[8] = {
      BOOTLOADER_RESPONSE_GET_FLASH_LAYOUT,
      BOOTLOADER_RESPONSE_STATUS_OK,
      BOOTLOADER_FLASH_PAGE_SIZE_KB,
      BOOTLOADER_FLASH_BOOT_SIZE_KB,
      BOOTLOADER_FLASH_APP_SIZE_KB,
      BOOTLOADER_FLASH_SCRATCH_PAGE_INDEX,
      0U,
      0U,
  };

  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendFlashSelfTestResponse(uint8_t status,
                                                    uint8_t stage,
                                                    const uint8_t detail[5])
{
  uint8_t response[8] = {
      BOOTLOADER_RESPONSE_FLASH_SELF_TEST,
      status,
      stage,
      detail[0],
      detail[1],
      detail[2],
      detail[3],
      detail[4],
  };

  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendStartUpdateResponse(uint8_t status,
                                                  uint8_t stage,
                                                  uint32_t app_size)
{
  uint8_t response[8] = {
      BOOTLOADER_RESPONSE_START_UPDATE,
      status,
      stage,
      0U,
      0U,
      0U,
      0U,
      0U,
  };

  BootloaderCan_WriteLe32(&response[3], app_size);
  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendEraseAppResponse(uint8_t status,
                                               uint8_t stage,
                                               uint16_t erased_pages)
{
  uint8_t response[8] = {
      BOOTLOADER_RESPONSE_ERASE_APP,
      status,
      stage,
      0U,
      0U,
      0U,
      0U,
      0U,
  };

  BootloaderCan_WriteLe16(&response[3], erased_pages);
  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendWriteChunkResponse(uint8_t status,
                                                 uint16_t sequence,
                                                 uint16_t next_sequence,
                                                 uint8_t detail0,
                                                 uint8_t detail1)
{
  uint8_t response[8] = {
      BOOTLOADER_RESPONSE_WRITE_CHUNK,
      status,
      0U,
      0U,
      0U,
      0U,
      detail0,
      detail1,
  };

  BootloaderCan_WriteLe16(&response[2], sequence);
  BootloaderCan_WriteLe16(&response[4], next_sequence);
  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendVerifyCrcResponse(uint8_t status, uint32_t actual_crc)
{
  uint8_t response[8] = {
      BOOTLOADER_RESPONSE_VERIFY_CRC,
      status,
      0U,
      0U,
      0U,
      0U,
      0U,
      0U,
  };

  BootloaderCan_WriteLe32(&response[2], actual_crc);
  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendSimpleUpdateResponse(uint8_t response_type, uint8_t status)
{
  uint8_t response[8] = {
      response_type,
      status,
      0U,
      0U,
      0U,
      0U,
      0U,
      0U,
  };

  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_SendError(uint8_t command, uint8_t status)
{
  uint8_t response[8] = {
      (uint8_t)(command | 0x80U),
      status,
      0U,
      0U,
      0U,
      0U,
      0U,
      0U,
  };

  (void)BootloaderCan_SendFrame(bootloader_response_id, response);
}

static void BootloaderCan_HandleStartUpdate(const uint8_t data[8])
{
  uint32_t app_size = BootloaderCan_ReadLe32(&data[3]);
  uint8_t stage = (uint8_t)BootloaderUpdate_GetState();
  uint8_t status;

  if (!BootloaderRequest_HasUpdateMagic(BOOTLOADER_REQUEST_DLC, data))
  {
    BootloaderCan_SendStartUpdateResponse(BOOTLOADER_UPDATE_STATUS_BAD_MAGIC,
                                          stage,
                                          app_size);
    return;
  }

  status = BootloaderUpdate_Start(app_size, data[7], &stage);
  BootloaderCan_SendStartUpdateResponse(status, stage, app_size);
}

static void BootloaderCan_HandleEraseApp(const uint8_t data[8])
{
  uint8_t stage = (uint8_t)BootloaderUpdate_GetState();
  uint16_t erased_pages = 0U;
  uint8_t status;

  if (!BootloaderRequest_HasUpdateMagic(BOOTLOADER_REQUEST_DLC, data))
  {
    BootloaderCan_SendEraseAppResponse(BOOTLOADER_UPDATE_STATUS_BAD_MAGIC,
                                       stage,
                                       erased_pages);
    return;
  }

  status = BootloaderUpdate_EraseApp(&stage, &erased_pages);
  BootloaderCan_SendEraseAppResponse(status, stage, erased_pages);
}

static void BootloaderCan_HandleWriteChunk(const uint8_t data[8])
{
  uint16_t sequence = BootloaderCan_ReadLe16(&data[1]);
  uint16_t next_sequence = 0U;
  uint8_t status;

  status = BootloaderUpdate_WriteChunk(sequence, data[3], &data[4], &next_sequence);
  BootloaderCan_SendWriteChunkResponse(status, sequence, next_sequence, 0U, 0U);
}

static void BootloaderCan_HandleVerifyCrc(const uint8_t data[8])
{
  uint32_t expected_crc = BootloaderCan_ReadLe32(&data[1]);
  uint32_t actual_crc = 0U;
  uint8_t status;

  status = BootloaderUpdate_VerifyCrc(expected_crc, &actual_crc);
  BootloaderCan_SendVerifyCrcResponse(status, actual_crc);
}

static void BootloaderCan_HandleFinishUpdate(const uint8_t data[8])
{
  uint8_t status;

  if (!BootloaderRequest_HasUpdateMagic(BOOTLOADER_REQUEST_DLC, data))
  {
    BootloaderCan_SendSimpleUpdateResponse(BOOTLOADER_RESPONSE_FINISH_UPDATE,
                                           BOOTLOADER_UPDATE_STATUS_BAD_MAGIC);
    return;
  }

  status = BootloaderUpdate_Finish();
  BootloaderCan_SendSimpleUpdateResponse(BOOTLOADER_RESPONSE_FINISH_UPDATE, status);
}

static void BootloaderCan_HandleAbortUpdate(const uint8_t data[8])
{
  uint8_t status;

  if (!BootloaderRequest_HasUpdateMagic(BOOTLOADER_REQUEST_DLC, data))
  {
    BootloaderCan_SendSimpleUpdateResponse(BOOTLOADER_RESPONSE_ABORT_UPDATE,
                                           BOOTLOADER_UPDATE_STATUS_BAD_MAGIC);
    return;
  }

  status = BootloaderUpdate_Abort();
  BootloaderCan_SendSimpleUpdateResponse(BOOTLOADER_RESPONSE_ABORT_UPDATE, status);
}

static void BootloaderCan_HandleResetToApp(const uint8_t data[8])
{
  uint8_t status = BOOTLOADER_UPDATE_STATUS_OK;

  if (!BootloaderRequest_HasUpdateMagic(BOOTLOADER_REQUEST_DLC, data))
  {
    BootloaderCan_SendSimpleUpdateResponse(BOOTLOADER_RESPONSE_RESET_TO_APP,
                                           BOOTLOADER_UPDATE_STATUS_BAD_MAGIC);
    return;
  }

  if (!BootloaderUpdate_IsApplicationValid(0, 0))
  {
    status = BOOTLOADER_UPDATE_STATUS_APP_INVALID;
  }

  BootloaderCan_SendSimpleUpdateResponse(BOOTLOADER_RESPONSE_RESET_TO_APP, status);

  if (status == BOOTLOADER_UPDATE_STATUS_OK)
  {
    __disable_irq();
    NVIC_SystemReset();
  }
}

void BootloaderCan_Task(void)
{
  uint32_t std_id = 0U;
  uint8_t dlc = 0U;
  uint8_t data[8] = {0};
  uint8_t command = 0U;

  if (!BootloaderCan_ReadFrame(&std_id, &dlc, data))
  {
    return;
  }

  if (std_id != bootloader_request_id)
  {
    return;
  }

  command = data[0];
  if (dlc != BOOTLOADER_REQUEST_DLC)
  {
    if (command == BOOTLOADER_COMMAND_GET_BOOT_INFO)
    {
      BootloaderCan_SendBootInfo(BOOTLOADER_UPDATE_STATUS_BAD_DLC);
    }
    else if (command == BOOTLOADER_COMMAND_RUN_FLASH_SELF_TEST)
    {
      uint8_t detail[5] = {0};
      BootloaderCan_SendFlashSelfTestResponse(BOOTLOADER_FLASH_STATUS_ADDRESS_RANGE_ERROR,
                                              BOOTLOADER_FLASH_STAGE_DONE,
                                              detail);
    }
    else
    {
      BootloaderCan_SendError(command, BOOTLOADER_RESPONSE_STATUS_BAD_DLC);
    }
    return;
  }

  if (BootloaderRequest_IsGetBootInfoCommand(dlc, data))
  {
    BootloaderCan_SendBootInfo(BOOTLOADER_UPDATE_STATUS_OK);
    return;
  }

  if (BootloaderRequest_IsGetFlashLayoutCommand(dlc, data))
  {
    BootloaderCan_SendFlashLayout();
    return;
  }

  if (data[0] == BOOTLOADER_COMMAND_RUN_FLASH_SELF_TEST)
  {
    uint8_t detail[5] = {0};
    (void)BootloaderRequest_IsRunFlashSelfTestCommand(dlc, data);
    BootloaderCan_SendFlashSelfTestResponse(BOOTLOADER_FLASH_STATUS_ADDRESS_RANGE_ERROR,
                                            BOOTLOADER_FLASH_STAGE_DONE,
                                            detail);
    return;
  }

  if (command == BOOTLOADER_COMMAND_START_UPDATE)
  {
    BootloaderCan_HandleStartUpdate(data);
    return;
  }

  if (command == BOOTLOADER_COMMAND_ERASE_APP)
  {
    BootloaderCan_HandleEraseApp(data);
    return;
  }

  if (command == BOOTLOADER_COMMAND_WRITE_CHUNK)
  {
    BootloaderCan_HandleWriteChunk(data);
    return;
  }

  if (command == BOOTLOADER_COMMAND_VERIFY_CRC)
  {
    BootloaderCan_HandleVerifyCrc(data);
    return;
  }

  if (command == BOOTLOADER_COMMAND_FINISH_UPDATE)
  {
    BootloaderCan_HandleFinishUpdate(data);
    return;
  }

  if (command == BOOTLOADER_COMMAND_ABORT_UPDATE)
  {
    BootloaderCan_HandleAbortUpdate(data);
    return;
  }

  if (command == BOOTLOADER_COMMAND_RESET_TO_APP)
  {
    BootloaderCan_HandleResetToApp(data);
    return;
  }

  BootloaderCan_SendError(command, BOOTLOADER_RESPONSE_STATUS_UNKNOWN_COMMAND);
}
