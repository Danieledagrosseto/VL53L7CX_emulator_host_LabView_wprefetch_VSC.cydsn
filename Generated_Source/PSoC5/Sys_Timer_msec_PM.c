/*******************************************************************************
* File Name: Sys_Timer_msec_PM.c
* Version 2.80
*
*  Description:
*     This file provides the power management source code to API for the
*     Timer.
*
*   Note:
*     None
*
*******************************************************************************
* Copyright 2008-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
********************************************************************************/

#include "Sys_Timer_msec.h"

static Sys_Timer_msec_backupStruct Sys_Timer_msec_backup;


/*******************************************************************************
* Function Name: Sys_Timer_msec_SaveConfig
********************************************************************************
*
* Summary:
*     Save the current user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Sys_Timer_msec_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void Sys_Timer_msec_SaveConfig(void) 
{
    #if (!Sys_Timer_msec_UsingFixedFunction)
        Sys_Timer_msec_backup.TimerUdb = Sys_Timer_msec_ReadCounter();
        Sys_Timer_msec_backup.InterruptMaskValue = Sys_Timer_msec_STATUS_MASK;
        #if (Sys_Timer_msec_UsingHWCaptureCounter)
            Sys_Timer_msec_backup.TimerCaptureCounter = Sys_Timer_msec_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!Sys_Timer_msec_UDB_CONTROL_REG_REMOVED)
            Sys_Timer_msec_backup.TimerControlRegister = Sys_Timer_msec_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: Sys_Timer_msec_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Sys_Timer_msec_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void Sys_Timer_msec_RestoreConfig(void) 
{   
    #if (!Sys_Timer_msec_UsingFixedFunction)

        Sys_Timer_msec_WriteCounter(Sys_Timer_msec_backup.TimerUdb);
        Sys_Timer_msec_STATUS_MASK =Sys_Timer_msec_backup.InterruptMaskValue;
        #if (Sys_Timer_msec_UsingHWCaptureCounter)
            Sys_Timer_msec_SetCaptureCount(Sys_Timer_msec_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!Sys_Timer_msec_UDB_CONTROL_REG_REMOVED)
            Sys_Timer_msec_WriteControlRegister(Sys_Timer_msec_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: Sys_Timer_msec_Sleep
********************************************************************************
*
* Summary:
*     Stop and Save the user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Sys_Timer_msec_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void Sys_Timer_msec_Sleep(void) 
{
    #if(!Sys_Timer_msec_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(Sys_Timer_msec_CTRL_ENABLE == (Sys_Timer_msec_CONTROL & Sys_Timer_msec_CTRL_ENABLE))
        {
            /* Timer is enabled */
            Sys_Timer_msec_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            Sys_Timer_msec_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    Sys_Timer_msec_Stop();
    Sys_Timer_msec_SaveConfig();
}


/*******************************************************************************
* Function Name: Sys_Timer_msec_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Sys_Timer_msec_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Sys_Timer_msec_Wakeup(void) 
{
    Sys_Timer_msec_RestoreConfig();
    #if(!Sys_Timer_msec_UDB_CONTROL_REG_REMOVED)
        if(Sys_Timer_msec_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                Sys_Timer_msec_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
