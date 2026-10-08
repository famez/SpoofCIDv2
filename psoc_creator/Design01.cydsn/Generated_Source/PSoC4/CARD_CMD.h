/*******************************************************************************
* File Name: CARD_CMD.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_CARD_CMD_H) /* Pins CARD_CMD_H */
#define CY_PINS_CARD_CMD_H

#include "cytypes.h"
#include "cyfitter.h"
#include "CARD_CMD_aliases.h"


/***************************************
*     Data Struct Definitions
***************************************/

/**
* \addtogroup group_structures
* @{
*/
    
/* Structure for sleep mode support */
typedef struct
{
    uint32 pcState; /**< State of the port control register */
    uint32 sioState; /**< State of the SIO configuration */
    uint32 usbState; /**< State of the USBIO regulator */
} CARD_CMD_BACKUP_STRUCT;

/** @} structures */


/***************************************
*        Function Prototypes             
***************************************/
/**
* \addtogroup group_general
* @{
*/
uint8   CARD_CMD_Read(void);
void    CARD_CMD_Write(uint8 value);
uint8   CARD_CMD_ReadDataReg(void);
#if defined(CARD_CMD__PC) || (CY_PSOC4_4200L) 
    void    CARD_CMD_SetDriveMode(uint8 mode);
#endif
void    CARD_CMD_SetInterruptMode(uint16 position, uint16 mode);
uint8   CARD_CMD_ClearInterrupt(void);
/** @} general */

/**
* \addtogroup group_power
* @{
*/
void CARD_CMD_Sleep(void); 
void CARD_CMD_Wakeup(void);
/** @} power */


/***************************************
*           API Constants        
***************************************/
#if defined(CARD_CMD__PC) || (CY_PSOC4_4200L) 
    /* Drive Modes */
    #define CARD_CMD_DRIVE_MODE_BITS        (3)
    #define CARD_CMD_DRIVE_MODE_IND_MASK    (0xFFFFFFFFu >> (32 - CARD_CMD_DRIVE_MODE_BITS))

    /**
    * \addtogroup group_constants
    * @{
    */
        /** \addtogroup driveMode Drive mode constants
         * \brief Constants to be passed as "mode" parameter in the CARD_CMD_SetDriveMode() function.
         *  @{
         */
        #define CARD_CMD_DM_ALG_HIZ         (0x00u) /**< \brief High Impedance Analog   */
        #define CARD_CMD_DM_DIG_HIZ         (0x01u) /**< \brief High Impedance Digital  */
        #define CARD_CMD_DM_RES_UP          (0x02u) /**< \brief Resistive Pull Up       */
        #define CARD_CMD_DM_RES_DWN         (0x03u) /**< \brief Resistive Pull Down     */
        #define CARD_CMD_DM_OD_LO           (0x04u) /**< \brief Open Drain, Drives Low  */
        #define CARD_CMD_DM_OD_HI           (0x05u) /**< \brief Open Drain, Drives High */
        #define CARD_CMD_DM_STRONG          (0x06u) /**< \brief Strong Drive            */
        #define CARD_CMD_DM_RES_UPDWN       (0x07u) /**< \brief Resistive Pull Up/Down  */
        /** @} driveMode */
    /** @} group_constants */
#endif

/* Digital Port Constants */
#define CARD_CMD_MASK               CARD_CMD__MASK
#define CARD_CMD_SHIFT              CARD_CMD__SHIFT
#define CARD_CMD_WIDTH              1u

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in CARD_CMD_SetInterruptMode() function.
     *  @{
     */
        #define CARD_CMD_INTR_NONE      ((uint16)(0x0000u)) /**< \brief Disabled             */
        #define CARD_CMD_INTR_RISING    ((uint16)(0x5555u)) /**< \brief Rising edge trigger  */
        #define CARD_CMD_INTR_FALLING   ((uint16)(0xaaaau)) /**< \brief Falling edge trigger */
        #define CARD_CMD_INTR_BOTH      ((uint16)(0xffffu)) /**< \brief Both edge trigger    */
    /** @} intrMode */
/** @} group_constants */

/* SIO LPM definition */
#if defined(CARD_CMD__SIO)
    #define CARD_CMD_SIO_LPM_MASK       (0x03u)
#endif

/* USBIO definitions */
#if !defined(CARD_CMD__PC) && (CY_PSOC4_4200L)
    #define CARD_CMD_USBIO_ENABLE               ((uint32)0x80000000u)
    #define CARD_CMD_USBIO_DISABLE              ((uint32)(~CARD_CMD_USBIO_ENABLE))
    #define CARD_CMD_USBIO_SUSPEND_SHIFT        CYFLD_USBDEVv2_USB_SUSPEND__OFFSET
    #define CARD_CMD_USBIO_SUSPEND_DEL_SHIFT    CYFLD_USBDEVv2_USB_SUSPEND_DEL__OFFSET
    #define CARD_CMD_USBIO_ENTER_SLEEP          ((uint32)((1u << CARD_CMD_USBIO_SUSPEND_SHIFT) \
                                                        | (1u << CARD_CMD_USBIO_SUSPEND_DEL_SHIFT)))
    #define CARD_CMD_USBIO_EXIT_SLEEP_PH1       ((uint32)~((uint32)(1u << CARD_CMD_USBIO_SUSPEND_SHIFT)))
    #define CARD_CMD_USBIO_EXIT_SLEEP_PH2       ((uint32)~((uint32)(1u << CARD_CMD_USBIO_SUSPEND_DEL_SHIFT)))
    #define CARD_CMD_USBIO_CR1_OFF              ((uint32)0xfffffffeu)
#endif


/***************************************
*             Registers        
***************************************/
/* Main Port Registers */
#if defined(CARD_CMD__PC)
    /* Port Configuration */
    #define CARD_CMD_PC                 (* (reg32 *) CARD_CMD__PC)
#endif
/* Pin State */
#define CARD_CMD_PS                     (* (reg32 *) CARD_CMD__PS)
/* Data Register */
#define CARD_CMD_DR                     (* (reg32 *) CARD_CMD__DR)
/* Input Buffer Disable Override */
#define CARD_CMD_INP_DIS                (* (reg32 *) CARD_CMD__PC2)

/* Interrupt configuration Registers */
#define CARD_CMD_INTCFG                 (* (reg32 *) CARD_CMD__INTCFG)
#define CARD_CMD_INTSTAT                (* (reg32 *) CARD_CMD__INTSTAT)

/* "Interrupt cause" register for Combined Port Interrupt (AllPortInt) in GSRef component */
#if defined (CYREG_GPIO_INTR_CAUSE)
    #define CARD_CMD_INTR_CAUSE         (* (reg32 *) CYREG_GPIO_INTR_CAUSE)
#endif

/* SIO register */
#if defined(CARD_CMD__SIO)
    #define CARD_CMD_SIO_REG            (* (reg32 *) CARD_CMD__SIO)
#endif /* (CARD_CMD__SIO_CFG) */

/* USBIO registers */
#if !defined(CARD_CMD__PC) && (CY_PSOC4_4200L)
    #define CARD_CMD_USB_POWER_REG       (* (reg32 *) CYREG_USBDEVv2_USB_POWER_CTRL)
    #define CARD_CMD_CR1_REG             (* (reg32 *) CYREG_USBDEVv2_CR1)
    #define CARD_CMD_USBIO_CTRL_REG      (* (reg32 *) CYREG_USBDEVv2_USB_USBIO_CTRL)
#endif    
    
    
/***************************************
* The following code is DEPRECATED and 
* must not be used in new designs.
***************************************/
/**
* \addtogroup group_deprecated
* @{
*/
#define CARD_CMD_DRIVE_MODE_SHIFT       (0x00u)
#define CARD_CMD_DRIVE_MODE_MASK        (0x07u << CARD_CMD_DRIVE_MODE_SHIFT)
/** @} deprecated */

#endif /* End Pins CARD_CMD_H */


/* [] END OF FILE */
