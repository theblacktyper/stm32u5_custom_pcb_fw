/**
  ******************************************************************************
  * @file    b_u585i_iot02a_bus.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the b_u585i_iot02a_bus.c driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef B_U585I_IOT02A_BUS_H
#define B_U585I_IOT02A_BUS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Includes ------------------------------------------------------------------*/
#include "b_u585i_iot02a_conf.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup B_U585I_IOT02A
  * @{
  */

/** @addtogroup B_U585I_IOT02A_BUS
  * @{
  */
/** @defgroup B_U585I_IOT02A_BUS_Exported_Types BUS Exported Types
  * @{
  */

/**
  * @}
  */
/** @defgroup B_U585I_IOT02A_BUS_Exported_Constants BUS Exported Constants
  * @{
  */
/* Definition for I2C1 clock resources */
#define BUS_I2C3                              &hi2c3//&hi2c1

/**
  * @}
  */

/** @addtogroup B_U585I_IOT02A_BUS_Exported_Variables
  * @{
  */

/**
  * @}
  */

/** @addtogroup B_U585I_IOT02A_BUS_Exported_Functions
  * @{
  */
int32_t BSP_I2C1_Init(void);
int32_t BSP_I2C1_DeInit(void);
int32_t BSP_I2C1_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_IsReady(uint16_t DevAddr, uint32_t Trials);

int32_t BSP_GetTick(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* B_U585I_IOT02A_BUS_H */
