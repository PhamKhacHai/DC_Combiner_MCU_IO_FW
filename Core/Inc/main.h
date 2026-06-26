/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FB4_N_Pin GPIO_PIN_13
#define FB4_N_GPIO_Port GPIOC
#define FB5_N_Pin GPIO_PIN_14
#define FB5_N_GPIO_Port GPIOC
#define FB6_N_Pin GPIO_PIN_15
#define FB6_N_GPIO_Port GPIOC
#define DIP1_Pin GPIO_PIN_5
#define DIP1_GPIO_Port GPIOA
#define DIP2_Pin GPIO_PIN_6
#define DIP2_GPIO_Port GPIOA
#define DIP3_Pin GPIO_PIN_7
#define DIP3_GPIO_Port GPIOA
#define OUT_6_Pin GPIO_PIN_12
#define OUT_6_GPIO_Port GPIOB
#define FB6_P_Pin GPIO_PIN_13
#define FB6_P_GPIO_Port GPIOB
#define OUT_5_Pin GPIO_PIN_14
#define OUT_5_GPIO_Port GPIOB
#define FB5_P_Pin GPIO_PIN_15
#define FB5_P_GPIO_Port GPIOB
#define OUT_4_Pin GPIO_PIN_8
#define OUT_4_GPIO_Port GPIOA
#define FB4_P_Pin GPIO_PIN_9
#define FB4_P_GPIO_Port GPIOA
#define OUT_3_Pin GPIO_PIN_10
#define OUT_3_GPIO_Port GPIOA
#define FB3_P_Pin GPIO_PIN_15
#define FB3_P_GPIO_Port GPIOA
#define OUT_2_Pin GPIO_PIN_3
#define OUT_2_GPIO_Port GPIOB
#define FB2_P_Pin GPIO_PIN_4
#define FB2_P_GPIO_Port GPIOB
#define OUT_1_Pin GPIO_PIN_5
#define OUT_1_GPIO_Port GPIOB
#define FB1_P_Pin GPIO_PIN_6
#define FB1_P_GPIO_Port GPIOB
#define FB1_N_Pin GPIO_PIN_7
#define FB1_N_GPIO_Port GPIOB
#define FB2_N_Pin GPIO_PIN_8
#define FB2_N_GPIO_Port GPIOB
#define FB3_N_Pin GPIO_PIN_9
#define FB3_N_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
