

/**************************************************************************//**
* @file      template.c
* @brief     Template for ESE516 with Doxygen-style comments
* @author    Eduardo Garcia
* @date      2020-01-01

******************************************************************************/


/******************************************************************************
* Includes
******************************************************************************/
#include "Systick.h"

/******************************************************************************
* Defines
******************************************************************************/

/******************************************************************************
* Variables
******************************************************************************/
static uint32_t ul_tickcount=0 ;	///< Global state variable for tick count

/******************************************************************************
* Forward Declarations
******************************************************************************/
void SysTick_Handler(void);

/******************************************************************************
* Function Definitions
******************************************************************************/




/**************************************************************************//**
* @fn		void InitSystick(void)
* @brief	Initializes the Systick timer. Useful to measure lengths of time.

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/
void InitSystick(void)
{

	// Configure SysTick to trigger every millisecond using the CPU Clock
	SysTick->CTRL = 0;					// Disable SysTick
	SysTick->LOAD = 999UL;				// Set reload register for 1mS interrupts
	NVIC_SetPriority(SysTick_IRQn, 3);	// Set interrupt priority to least urgency
	SysTick->VAL = 0;					// Reset the SysTick counter value
	SysTick->CTRL = 0x00000007;			// Enable SysTick, Enable SysTick Exceptions, Use CPU Clock
	NVIC_EnableIRQ(SysTick_IRQn);		// Enable SysTick Interrupt

}


/**************************************************************************//**
* @fn		uint32_t GetSystick(void)
* @brief	Initializes the Systick timer. Useful to measure lengths of time.

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/
uint32_t GetSystick(void)
{
	return ul_tickcount;
}


/******************************************************************************
* Callback Functions
******************************************************************************/


/*******************************************************************************
 * Function:        void SysTick_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This interrupt handler is called on SysTick timer underflow
 *
 * Note:
 *
 ******************************************************************************/
void SysTick_Handler(void)
{
	
	ul_tickcount++;
}

