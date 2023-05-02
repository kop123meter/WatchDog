/**************************************************************************//**
* @file      SdCard.c
* @brief     Contains code that helps us deal with the SD card initialization, read, writes, etc.
* @author    Eduardo Garcia
* @date      2020-02-15

******************************************************************************/


/******************************************************************************
* Includes
******************************************************************************/
#include "SdCard.h"
#include "Systick/Systick.h"

/******************************************************************************
* Defines
******************************************************************************/
#define SD_CARD_TIMEOUT	500 ///< TIMEOUT IN SYSTICK AFTER WHICH AN SD CARD FUNCTION WILL FAIL.

/******************************************************************************
* Variables
******************************************************************************/

/******************************************************************************
* Forward Declarations
******************************************************************************/

/******************************************************************************
* Callback Functions
******************************************************************************/


/**************************************************************************//**
* @fn		Ctrl_status SdCard_Initiate()
* @brief	Initiates an SD Card.
* @details 	Initiates an SD Card.
                				
* @return		Returns CTRL_GOOD if the SDS card was mounted incorrectly - other status otherwise
* @note         
*****************************************************************************/

Ctrl_status SdCard_Initiate(void)
{
	Ctrl_status status;
	uint32_t timeStart = GetSystick();

/* Wait card present and ready */
do {
	status = sd_mmc_test_unit_ready(0);
	if (CTRL_FAIL == status) {
		while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
		}
	}

	if (GetSystick() - timeStart > SD_CARD_TIMEOUT)
	{
		status = CTRL_FAIL;
		break;
	}
} while (CTRL_GOOD != status );

return status;
}
