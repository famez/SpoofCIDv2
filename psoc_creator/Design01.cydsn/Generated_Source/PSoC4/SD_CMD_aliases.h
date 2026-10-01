/*******************************************************************************
* File Name: SD_CMD.h  
* Version 2.20
*
* Description:
*  This file contains the Alias definitions for Per-Pin APIs in cypins.h. 
*  Information on using these APIs can be found in the System Reference Guide.
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_SD_CMD_ALIASES_H) /* Pins SD_CMD_ALIASES_H */
#define CY_PINS_SD_CMD_ALIASES_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"


/***************************************
*              Constants        
***************************************/
#define SD_CMD_0			(SD_CMD__0__PC)
#define SD_CMD_0_PS		(SD_CMD__0__PS)
#define SD_CMD_0_PC		(SD_CMD__0__PC)
#define SD_CMD_0_DR		(SD_CMD__0__DR)
#define SD_CMD_0_SHIFT	(SD_CMD__0__SHIFT)
#define SD_CMD_0_INTR	((uint16)((uint16)0x0003u << (SD_CMD__0__SHIFT*2u)))

#define SD_CMD_INTR_ALL	 ((uint16)(SD_CMD_0_INTR))


#endif /* End Pins SD_CMD_ALIASES_H */


/* [] END OF FILE */
