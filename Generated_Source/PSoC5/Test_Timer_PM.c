/*******************************************************************************
* File Name: Test_Timer_PM.c
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

#include "Test_Timer.h"

static Test_Timer_backupStruct Test_Timer_backup;


/*******************************************************************************
* Function Name: Test_Timer_SaveConfig
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
*  Test_Timer_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void Test_Timer_SaveConfig(void) 
{
    #if (!Test_Timer_UsingFixedFunction)
        Test_Timer_backup.TimerUdb = Test_Timer_ReadCounter();
        Test_Timer_backup.InterruptMaskValue = Test_Timer_STATUS_MASK;
        #if (Test_Timer_UsingHWCaptureCounter)
            Test_Timer_backup.TimerCaptureCounter = Test_Timer_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!Test_Timer_UDB_CONTROL_REG_REMOVED)
            Test_Timer_backup.TimerControlRegister = Test_Timer_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: Test_Timer_RestoreConfig
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
*  Test_Timer_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void Test_Timer_RestoreConfig(void) 
{   
    #if (!Test_Timer_UsingFixedFunction)

        Test_Timer_WriteCounter(Test_Timer_backup.TimerUdb);
        Test_Timer_STATUS_MASK =Test_Timer_backup.InterruptMaskValue;
        #if (Test_Timer_UsingHWCaptureCounter)
            Test_Timer_SetCaptureCount(Test_Timer_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!Test_Timer_UDB_CONTROL_REG_REMOVED)
            Test_Timer_WriteControlRegister(Test_Timer_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: Test_Timer_Sleep
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
*  Test_Timer_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void Test_Timer_Sleep(void) 
{
    #if(!Test_Timer_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(Test_Timer_CTRL_ENABLE == (Test_Timer_CONTROL & Test_Timer_CTRL_ENABLE))
        {
            /* Timer is enabled */
            Test_Timer_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            Test_Timer_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    Test_Timer_Stop();
    Test_Timer_SaveConfig();
}


/*******************************************************************************
* Function Name: Test_Timer_Wakeup
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
*  Test_Timer_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Test_Timer_Wakeup(void) 
{
    Test_Timer_RestoreConfig();
    #if(!Test_Timer_UDB_CONTROL_REG_REMOVED)
        if(Test_Timer_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                Test_Timer_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
