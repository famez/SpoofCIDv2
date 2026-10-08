/*******************************************************************************
* File Name: PSOC_CMD.h  
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

#if !defined(CY_PINS_PSOC_CMD_H) /* Pins PSOC_CMD_H */
#define CY_PINS_PSOC_CMD_H

#include "cytypes.h"
#include "cyfitter.h"
#include "PSOC_CMD_aliases.h"


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
} PSOC_CMD_BACKUP_STRUCT;

/** @} structures */


/***************************************
*        Function Prototypes             
***************************************/
/**
* \addtogroup group_general
* @{
*/
uint8   PSOC_CMD_Read(void);
void    PSOC_CMD_Write(uint8 value);
uint8   PSOC_CMD_ReadDataReg(void);
#if defined(PSOC_CMD__PC) || (CY_PSOC4_4200L) 
    void    PSOC_CMD_SetDriveMode(uint8 mode);
#endif
void    PSOC_CMD_SetInterruptMode(uint16 position, uint16 mode);
uint8   PSOC_CMD_ClearInterrupt(void);
/** @} general */

/**
* \addtogroup group_power
* @{
*/
void PSOC_CMD_Sleep(void); 
void PSOC_CMD_Wakeup(void);
/** @} power */


/***************************************
*           API Constants        
***************************************/
#if defined(PSOC_CMD__PC) || (CY_PSOC4_4200L) 
    /* Drive Modes */
    #define PSOC_CMD_DRIVE_MODE_BITS        (3)
    #define PSOC_CMD_DRIVE_MODE_IND_MASK    (0xFFFFFFFFu >> (32 - PSOC_CMD_DRIVE_MODE_BITS))

    /**
    * \addtogroup group_constants
    * @{
    */
        /** \addtogroup driveMode Drive mode constants
         * \brief Constants to be passed as "mode" parameter in the PSOC_CMD_SetDriveMode() function.
         *  @{
         */
        #define PSOC_CMD_DM_ALG_HIZ         (0x00u) /**< \brief High Impedance Analog   */
        #define PSOC_CMD_DM_DIG_HIZ         (0x01u) /**< \brief High Impedance Digital  */
        #define PSOC_CMD_DM_RES_UP          (0x02u) /**< \brief Resistive Pull Up       */
        #define PSOC_CMD_DM_RES_DWN         (0x03u) /**< \brief Resistive Pull Down     */
        #define PSOC_CMD_DM_OD_LO           (0x04u) /**< \brief Open Drain, Drives Low  */
        #define PSOC_CMD_DM_OD_HI           (0x05u) /**< \brief Open Drain, Drives High */
        #define PSOC_CMD_DM_STRONG          (0x06u) /**< \brief Strong Drive            */
        #define PSOC_CMD_DM_RES_UPDWN       (0x07u) /**< \brief Resistive Pull Up/Down  */
        /** @} driveMode */
    /** @} group_constants */
#endif

/* Digital Port Constants */
#define PSOC_CMD_MASK               PSOC_CMD__MASK
#define PSOC_CMD_SHIFT              PSOC_CMD__SHIFT
#define PSOC_CMD_WIDTH              1u

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in PSOC_CMD_SetInterruptMode() function.
     *  @{
     */
        #define PSOC_CMD_INTR_NONE      ((uint16)(0x0000u)) /**< \brief Disabled             */
        #define PSOC_CMD_INTR_RISING    ((uint16)(0x5555u)) /**< \brief Rising edge trigger  */
        #define PSOC_CMD_INTR_FALLING   ((uint16)(0xaaaau)) /**< \brief Falling edge trigger */
        #define PSOC_CMD_INTR_BOTH      ((uint16)(0xffffu)) /**< \brief Both edge trigger    */
    /** @} intrMode */
/** @} group_constants */

/* SIO LPM definition */
#if defined(PSOC_CMD__SIO)
    #define PSOC_CMD_SIO_LPM_MASK       (0x03u)
#endif

/* USBIO definitions */
#if !defined(PSOC_CMD__PC) && (CY_PSOC4_4200L)
    #define PSOC_CMD_USBIO_ENABLE               ((uint32)0x80000000u)
    #define PSOC_CMD_USBIO_DISABLE              ((uint32)(~PSOC_CMD_USBIO_ENABLE))
    #define PSOC_CMD_USBIO_SUSPEND_SHIFT        CYFLD_USBDEVv2_USB_SUSPEND__OFFSET
    #define PSOC_CMD_USBIO_SUSPEND_DEL_SHIFT    CYFLD_USBDEVv2_USB_SUSPEND_DEL__OFFSET
    #define PSOC_CMD_USBIO_ENTER_SLEEP          ((uint32)((1u << PSOC_CMD_USBIO_SUSPEND_SHIFT) \
                                                        | (1u << PSOC_CMD_USBIO_SUSPEND_DEL_SHIFT)))
    #define PSOC_CMD_USBIO_EXIT_SLEEP_PH1       ((uint32)~((uint32)(1u << PSOC_CMD_USBIO_SUSPEND_SHIFT)))
    #define PSOC_CMD_USBIO_EXIT_SLEEP_PH2       ((uint32)~((uint32)(1u << PSOC_CMD_USBIO_SUSPEND_DEL_SHIFT)))
    #define PSOC_CMD_USBIO_CR1_OFF              ((uint32)0xfffffffeu)
#endif


/***************************************
*             Registers        
***************************************/
/* Main Port Registers */
#if defined(PSOC_CMD__PC)
    /* Port Configuration */
    #define PSOC_CMD_PC                 (* (reg32 *) PSOC_CMD__PC)
#endif
/* Pin State */
#define PSOC_CMD_PS                     (* (reg32 *) PSOC_CMD__PS)
/* Data Register */
#define PSOC_CMD_DR                     (* (reg32 *) PSOC_CMD__DR)
/* Input Buffer Disable Override */
#define PSOC_CMD_INP_DIS                (* (reg32 *) PSOC_CMD__PC2)

/* Interrupt configuration Registers */
#define PSOC_CMD_INTCFG                 (* (reg32 *) PSOC_CMD__INTCFG)
#define PSOC_CMD_INTSTAT                (* (reg32 *) PSOC_CMD__INTSTAT)

/* "Interrupt cause" register for Combined Port Interrupt (AllPortInt) in GSRef component */
#if defined (CYREG_GPIO_INTR_CAUSE)
    #define PSOC_CMD_INTR_CAUSE         (* (reg32 *) CYREG_GPIO_INTR_CAUSE)
#endif

/* SIO register */
#if defined(PSOC_CMD__SIO)
    #define PSOC_CMD_SIO_REG            (* (reg32 *) PSOC_CMD__SIO)
#endif /* (PSOC_CMD__SIO_CFG) */

/* USBIO registers */
#if !defined(PSOC_CMD__PC) && (CY_PSOC4_4200L)
    #define PSOC_CMD_USB_POWER_REG       (* (reg32 *) CYREG_USBDEVv2_USB_POWER_CTRL)
    #define PSOC_CMD_CR1_REG             (* (reg32 *) CYREG_USBDEVv2_CR1)
    #define PSOC_CMD_USBIO_CTRL_REG      (* (reg32 *) CYREG_USBDEVv2_USB_USBIO_CTRL)
#endif    
    
    
/***************************************
* The following code is DEPRECATED and 
* must not be used in new designs.
***************************************/
/**
* \addtogroup group_deprecated
* @{
*/
#define PSOC_CMD_DRIVE_MODE_SHIFT       (0x00u)
#define PSOC_CMD_DRIVE_MODE_MASK        (0x07u << PSOC_CMD_DRIVE_MODE_SHIFT)
/** @} deprecated */

#endif /* End Pins PSOC_CMD_H */


/* [] END OF FILE */
