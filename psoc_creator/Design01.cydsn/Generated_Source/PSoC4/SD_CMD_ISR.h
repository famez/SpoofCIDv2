/*******************************************************************************
* File Name: SD_CMD_ISR.h
* Version 1.70
*
*  Description:
*   Provides the function definitions for the Interrupt Controller.
*
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/
#if !defined(CY_ISR_SD_CMD_ISR_H)
#define CY_ISR_SD_CMD_ISR_H


#include <cytypes.h>
#include <cyfitter.h>

/* Interrupt Controller API. */
void SD_CMD_ISR_Start(void);
void SD_CMD_ISR_StartEx(cyisraddress address);
void SD_CMD_ISR_Stop(void);

CY_ISR_PROTO(SD_CMD_ISR_Interrupt);

void SD_CMD_ISR_SetVector(cyisraddress address);
cyisraddress SD_CMD_ISR_GetVector(void);

void SD_CMD_ISR_SetPriority(uint8 priority);
uint8 SD_CMD_ISR_GetPriority(void);

void SD_CMD_ISR_Enable(void);
uint8 SD_CMD_ISR_GetState(void);
void SD_CMD_ISR_Disable(void);

void SD_CMD_ISR_SetPending(void);
void SD_CMD_ISR_ClearPending(void);


/* Interrupt Controller Constants */

/* Address of the INTC.VECT[x] register that contains the Address of the SD_CMD_ISR ISR. */
#define SD_CMD_ISR_INTC_VECTOR            ((reg32 *) SD_CMD_ISR__INTC_VECT)

/* Address of the SD_CMD_ISR ISR priority. */
#define SD_CMD_ISR_INTC_PRIOR             ((reg32 *) SD_CMD_ISR__INTC_PRIOR_REG)

/* Priority of the SD_CMD_ISR interrupt. */
#define SD_CMD_ISR_INTC_PRIOR_NUMBER      SD_CMD_ISR__INTC_PRIOR_NUM

/* Address of the INTC.SET_EN[x] byte to bit enable SD_CMD_ISR interrupt. */
#define SD_CMD_ISR_INTC_SET_EN            ((reg32 *) SD_CMD_ISR__INTC_SET_EN_REG)

/* Address of the INTC.CLR_EN[x] register to bit clear the SD_CMD_ISR interrupt. */
#define SD_CMD_ISR_INTC_CLR_EN            ((reg32 *) SD_CMD_ISR__INTC_CLR_EN_REG)

/* Address of the INTC.SET_PD[x] register to set the SD_CMD_ISR interrupt state to pending. */
#define SD_CMD_ISR_INTC_SET_PD            ((reg32 *) SD_CMD_ISR__INTC_SET_PD_REG)

/* Address of the INTC.CLR_PD[x] register to clear the SD_CMD_ISR interrupt. */
#define SD_CMD_ISR_INTC_CLR_PD            ((reg32 *) SD_CMD_ISR__INTC_CLR_PD_REG)



#endif /* CY_ISR_SD_CMD_ISR_H */


/* [] END OF FILE */
