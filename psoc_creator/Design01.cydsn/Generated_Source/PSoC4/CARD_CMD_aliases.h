/*******************************************************************************
* File Name: CARD_CMD.h  
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

#if !defined(CY_PINS_CARD_CMD_ALIASES_H) /* Pins CARD_CMD_ALIASES_H */
#define CY_PINS_CARD_CMD_ALIASES_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"


/***************************************
*              Constants        
***************************************/
#define CARD_CMD_0			(CARD_CMD__0__PC)
#define CARD_CMD_0_PS		(CARD_CMD__0__PS)
#define CARD_CMD_0_PC		(CARD_CMD__0__PC)
#define CARD_CMD_0_DR		(CARD_CMD__0__DR)
#define CARD_CMD_0_SHIFT	(CARD_CMD__0__SHIFT)
#define CARD_CMD_0_INTR	((uint16)((uint16)0x0003u << (CARD_CMD__0__SHIFT*2u)))

#define CARD_CMD_INTR_ALL	 ((uint16)(CARD_CMD_0_INTR))


#endif /* End Pins CARD_CMD_ALIASES_H */


/* [] END OF FILE */
