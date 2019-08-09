/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "stm32f4xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Valve_Enable_A_Pin GPIO_PIN_8
#define Valve_Enable_A_GPIO_Port GPIOI
#define Valve12_Pin GPIO_PIN_13
#define Valve12_GPIO_Port GPIOC
#define Valve11_Pin GPIO_PIN_9
#define Valve11_GPIO_Port GPIOI
#define Valve10_Pin GPIO_PIN_10
#define Valve10_GPIO_Port GPIOI
#define Valve9_Pin GPIO_PIN_11
#define Valve9_GPIO_Port GPIOI
#define H_Valve_2_Pin GPIO_PIN_12
#define H_Valve_2_GPIO_Port GPIOI
#define H_Valve_1_Pin GPIO_PIN_13
#define H_Valve_1_GPIO_Port GPIOI
#define Valve_Enable_B_Pin GPIO_PIN_14
#define Valve_Enable_B_GPIO_Port GPIOI
#define AD_Flow_Air_O2_Pin GPIO_PIN_8
#define AD_Flow_Air_O2_GPIO_Port GPIOF
#define AD_OEM_Temp_Pin GPIO_PIN_9
#define AD_OEM_Temp_GPIO_Port GPIOF
#define AD_Oxygen_Pin GPIO_PIN_10
#define AD_Oxygen_GPIO_Port GPIOF
#define AD_Flow1_H_Pin GPIO_PIN_0
#define AD_Flow1_H_GPIO_Port GPIOC
#define AD_Flow1_L_Pin GPIO_PIN_1
#define AD_Flow1_L_GPIO_Port GPIOC
#define AD_Flow2_H_Pin GPIO_PIN_2
#define AD_Flow2_H_GPIO_Port GPIOC
#define AD_Flow2_L_Pin GPIO_PIN_3
#define AD_Flow2_L_GPIO_Port GPIOC
#define AD_Temp_1_Pin GPIO_PIN_0
#define AD_Temp_1_GPIO_Port GPIOA
#define AD_Temp_2_Pin GPIO_PIN_2
#define AD_Temp_2_GPIO_Port GPIOA
#define F1_EEPROM_WP_Pin GPIO_PIN_3
#define F1_EEPROM_WP_GPIO_Port GPIOH
#define F1_EEPROM_CLK_Pin GPIO_PIN_4
#define F1_EEPROM_CLK_GPIO_Port GPIOH
#define SDRAM_WE_Pin GPIO_PIN_5
#define SDRAM_WE_GPIO_Port GPIOH
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
#define AD_High_Pressure_Pin GPIO_PIN_7
#define AD_High_Pressure_GPIO_Port GPIOA
#define AD_Pressure_2_Pin GPIO_PIN_4
#define AD_Pressure_2_GPIO_Port GPIOC
#define AD_Pressure_3_Pin GPIO_PIN_5
#define AD_Pressure_3_GPIO_Port GPIOC
#define AD_Pressure_1_Pin GPIO_PIN_0
#define AD_Pressure_1_GPIO_Port GPIOB
#define AD_Pressure_4_Pin GPIO_PIN_1
#define AD_Pressure_4_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_15
#define LED3_GPIO_Port GPIOI
#define RUN_LED_Pin GPIO_PIN_0
#define RUN_LED_GPIO_Port GPIOJ
#define LED4_Pin GPIO_PIN_1
#define LED4_GPIO_Port GPIOJ
#define LED1_Pin GPIO_PIN_2
#define LED1_GPIO_Port GPIOJ
#define LED5_Pin GPIO_PIN_3
#define LED5_GPIO_Port GPIOJ
#define LED2_Pin GPIO_PIN_4
#define LED2_GPIO_Port GPIOJ
#define SDRAM_RAS_Pin GPIO_PIN_11
#define SDRAM_RAS_GPIO_Port GPIOF
#define F1_EEPROM_DATA_Pin GPIO_PIN_11
#define F1_EEPROM_DATA_GPIO_Port GPIOB
#define IO_PG5_Pin GPIO_PIN_5
#define IO_PG5_GPIO_Port GPIOJ
#define SDRAM_CS_Pin GPIO_PIN_6
#define SDRAM_CS_GPIO_Port GPIOH
#define SDRAM_CKE_Pin GPIO_PIN_7
#define SDRAM_CKE_GPIO_Port GPIOH
#define Hall_CH3_Pin GPIO_PIN_10
#define Hall_CH3_GPIO_Port GPIOH
#define SD_PWR_Pin GPIO_PIN_11
#define SD_PWR_GPIO_Port GPIOH
#define SD_INS_Pin GPIO_PIN_12
#define SD_INS_GPIO_Port GPIOH
#define Blower_Enable_Pin GPIO_PIN_11
#define Blower_Enable_GPIO_Port GPIOD
#define Hall_CH1_Pin GPIO_PIN_12
#define Hall_CH1_GPIO_Port GPIOD
#define Hall_CH2_Pin GPIO_PIN_13
#define Hall_CH2_GPIO_Port GPIOD
#define _Disable_Pin GPIO_PIN_6
#define _Disable_GPIO_Port GPIOJ
#define Direction_Pin GPIO_PIN_7
#define Direction_GPIO_Port GPIOJ
#define AUX_Pin GPIO_PIN_8
#define AUX_GPIO_Port GPIOJ
#define _Brake_Pin GPIO_PIN_9
#define _Brake_GPIO_Port GPIOJ
#define SDRAM_CLK_Pin GPIO_PIN_8
#define SDRAM_CLK_GPIO_Port GPIOG
#define PWM1_Pin GPIO_PIN_6
#define PWM1_GPIO_Port GPIOC
#define PWM2_Pin GPIO_PIN_7
#define PWM2_GPIO_Port GPIOC
#define PWM3_Pin GPIO_PIN_8
#define PWM3_GPIO_Port GPIOC
#define PWM4_Pin GPIO_PIN_9
#define PWM4_GPIO_Port GPIOC
#define Valve4_Pin GPIO_PIN_11
#define Valve4_GPIO_Port GPIOA
#define Valve8_Pin GPIO_PIN_12
#define Valve8_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define Valve3_Pin GPIO_PIN_13
#define Valve3_GPIO_Port GPIOH
#define Valve7_Pin GPIO_PIN_14
#define Valve7_GPIO_Port GPIOH
#define Valve2_Pin GPIO_PIN_15
#define Valve2_GPIO_Port GPIOH
#define Valve6_Pin GPIO_PIN_0
#define Valve6_GPIO_Port GPIOI
#define Valve1_Pin GPIO_PIN_1
#define Valve1_GPIO_Port GPIOI
#define Valve5_Pin GPIO_PIN_2
#define Valve5_GPIO_Port GPIOI
#define BUZZER_Pin GPIO_PIN_3
#define BUZZER_GPIO_Port GPIOI
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define TDI_Pin GPIO_PIN_15
#define TDI_GPIO_Port GPIOA
#define SW06_Pin GPIO_PIN_15
#define SW06_GPIO_Port GPIOJ
#define SW05_Pin GPIO_PIN_10
#define SW05_GPIO_Port GPIOG
#define SW04_Pin GPIO_PIN_11
#define SW04_GPIO_Port GPIOG
#define SW03_Pin GPIO_PIN_12
#define SW03_GPIO_Port GPIOG
#define SW02_Pin GPIO_PIN_13
#define SW02_GPIO_Port GPIOG
#define SW01_Pin GPIO_PIN_3
#define SW01_GPIO_Port GPIOK
#define SW07_Pin GPIO_PIN_4
#define SW07_GPIO_Port GPIOK
#define SW08_Pin GPIO_PIN_5
#define SW08_GPIO_Port GPIOK
#define SW09_Pin GPIO_PIN_6
#define SW09_GPIO_Port GPIOK
#define SW10_Pin GPIO_PIN_7
#define SW10_GPIO_Port GPIOK
#define SDRAM_CAS_Pin GPIO_PIN_15
#define SDRAM_CAS_GPIO_Port GPIOG
#define TDO_Pin GPIO_PIN_3
#define TDO_GPIO_Port GPIOB
#define RST_Pin GPIO_PIN_4
#define RST_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
