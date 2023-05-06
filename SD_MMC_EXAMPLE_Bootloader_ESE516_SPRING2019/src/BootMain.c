/**************************************************************************//**
* @file      BootMain.c
* @brief     Main file for the ESE516 bootloader. Handles updating the main application
* @details   Main file for the ESE516 bootloader. Handles updating the main application
* @author    Eduardo Garcia
* @date      2020-02-15
* @version   2.0
* @copyright Copyright University of Pennsylvania
******************************************************************************/


/******************************************************************************
* Includes
******************************************************************************/
#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "sd_mmc_spi.h"

#include "SD Card/SdCard.h"
#include "Systick/Systick.h"
#include "SerialConsole/SerialConsole.h"
#include "ASF/sam0/drivers/dsu/crc32/crc32.h"





/******************************************************************************
* Defines
******************************************************************************/
#define APP_START_ADDRESS  ((uint32_t)0x12000) ///<Start of main application. Must be address of start of main application
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04) ///< Main application reset vector address
//#define MEM_EXAMPLE 1 //COMMENT ME TO REMOVE THE MEMORY WRITE EXAMPLE BELOW

/******************************************************************************
* Structures and Enumerations
******************************************************************************/

struct usart_module cdc_uart_module; ///< Structure for UART module connected to EDBG (used for unit test output)

/******************************************************************************
* Local Function Declaration
******************************************************************************/
static void jumpToApplication(void);
static bool StartFilesystemAndTest(void);
static void configure_nvm(void);
void clean_buffer();
void read_bin_file();

/******************************************************************************
* Global Variables
******************************************************************************/
//INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";	///<Test TEXT File name
char test_bin_file[] = "0:sd_binary.bin";	///<Test BINARY File name
char testA_bin_file[] = "0:ESE516_MAIN_FW.bin";	///<Test BINARY File name
Ctrl_status status; ///<Holds the status of a system initialization
FRESULT res; //Holds the result of the FATFS functions done on the SD CARD TEST
FATFS fs; //Holds the File System of the SD CARD
FIL file_object; //FILE OBJECT used on main for the SD Card Test

FIL file_objectA;
FRESULT resA;


/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn		int main(void)
* @brief	Main function for ESE516 Bootloader Application

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/

int main(void)
{

	/*1.) INIT SYSTEM PERIPHERALS INITIALIZATION*/
	system_init();
	delay_init();
	InitializeSerialConsole();
	system_interrupt_enable_global();
	/* Initialize SD MMC stack */
	sd_mmc_init();

	//Initialize the NVM driver
	configure_nvm();

	irq_initialize_vectors();
	cpu_irq_enable();

	//Configure CRC32
	dsu_crc32_init();

	SerialConsoleWriteString("ESE516 - ENTER BOOTLOADER");	//Order to add string to TX Buffer

	/*END SYSTEM PERIPHERALS INITIALIZATION*/


	/*2.) STARTS SIMPLE SD CARD MOUNTING AND TEST!*/

	//EXAMPLE CODE ON MOUNTING THE SD CARD AND WRITING TO A FILE
	//See function inside to see how to open a file
	SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");

	if(StartFilesystemAndTest() == false)
	{
		SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...");
		delay_cycles_ms(5000);
		system_reset();
	}
	else
	{
		SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
	}

	/*END SIMPLE SD CARD MOUNTING AND TEST!*/


	/*3.) STARTS BOOTLOADER HERE!*/
     testA_bin_file[0] = LUN_ID_SD_MMC_0_MEM +'0';
	 resA = f_open(&file_objectA,(char const *)testA_bin_file,FA_READ);
	 if(resA == FR_OK){
		 read_bin_file(file_objectA);
		 f_close(&file_objectA);
	 }
	 else{
		  f_close(&file_objectA);
	 }


	//4.) DEINITIALIZE HW AND JUMP TO MAIN APPLICATION!
	SerialConsoleWriteString("ESE516 - EXIT BOOTLOADER");	//Order to add string to TX Buffer
	delay_cycles_ms(100); //Delay to allow print
		
		//Deinitialize HW - deinitialize started HW here!
		DeinitializeSerialConsole(); //Deinitializes UART
		sd_mmc_deinit(); //Deinitialize SD CARD


		//Jump to application
		jumpToApplication();

		//Should not reach here! The device should have jumped to the main FW.
	
}







/******************************************************************************
* Static Functions
******************************************************************************/



/**************************************************************************//**
* function      static void StartFilesystemAndTest()
* @brief        Starts the filesystem and tests it. Sets the filesystem to the global variable fs
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       Returns true is SD card and file system test passed. False otherwise.
******************************************************************************/
static bool StartFilesystemAndTest(void)
{
	bool sdCardPass = true;
	uint8_t binbuff[256];

	//Before we begin - fill buffer for binary write test
	//Fill binbuff with values 0x00 - 0xFF
	for(int i = 0; i < 256; i++)
	{
		binbuff[i] = i;
	}

	//MOUNT SD CARD
	Ctrl_status sdStatus= SdCard_Initiate();
	if(sdStatus == CTRL_GOOD) //If the SD card is good we continue mounting the system!
	{
		SerialConsoleWriteString("SD Card initiated correctly!\n\r");

		//Attempt to mount a FAT file system on the SD Card using FATFS
		SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs); //Order FATFS Mount
		if (FR_INVALID_DRIVE == res)
		{
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}
		SerialConsoleWriteString("[OK]\r\n");

		//Create and open a file
		SerialConsoleWriteString("Create a file (f_open)...\r\n");

		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object,
		(char const *)test_file_name,
		FA_CREATE_ALWAYS | FA_WRITE);
		
		if (res != FR_OK)
		{
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");

		//Write to a file
		SerialConsoleWriteString("Write to test file (f_puts)...\r\n");

		if (0 == f_puts("Test SD/MMC stack\n", &file_object))
		{
			f_close(&file_object);
			LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");
		f_close(&file_object); //Close file
		SerialConsoleWriteString("Test is successful.\n\r");


		//Write binary file
		//Read SD Card File
		test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)test_bin_file, FA_WRITE | FA_CREATE_ALWAYS);
		
		if (res != FR_OK)
		{
			SerialConsoleWriteString("Could not open binary file!\r\n");
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}

		//Write to a binaryfile
		SerialConsoleWriteString("Write to test file (f_write)...\r\n");
		uint32_t varWrite = 0;
		if (0 != f_write(&file_object, binbuff,256, &varWrite))
		{
			f_close(&file_object);
			LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");
		f_close(&file_object); //Close file
		SerialConsoleWriteString("Test is successful.\n\r");
		
		main_end_of_test:
		SerialConsoleWriteString("End of Test.\n\r");

	}
	else
	{
		SerialConsoleWriteString("SD Card failed initiation! Check connections!\n\r");
		sdCardPass = false;
	}

	return sdCardPass;
}



/**************************************************************************//**
* function      static void jumpToApplication(void)
* @brief        Jumps to main application
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       
******************************************************************************/
static void jumpToApplication(void)
{
// Function pointer to application section
void (*applicationCodeEntry)(void);

// Rebase stack pointer
__set_MSP(*(uint32_t *) APP_START_ADDRESS);

// Rebase vector table
SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

// Set pointer to application section
applicationCodeEntry =
(void (*)(void))(unsigned *)(*(unsigned *)(APP_START_RESET_VEC_ADDRESS));

// Jump to application. By calling applicationCodeEntry() as a function we move the PC to the point in memory pointed by applicationCodeEntry, 
//which should be the start of the main FW.
applicationCodeEntry();
}



/**************************************************************************//**
* function      static void configure_nvm(void)
* @brief        Configures the NVM driver
* @details      
* @return       
******************************************************************************/
static void configure_nvm(void)
{
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    config_nvm.manual_page_write = false;
    nvm_set_config(&config_nvm);
}

void clean_buffer(){
	struct nvm_parameters parameters;
	nvm_get_parameters (&parameters);
	char helpStr[64]; 
	snprintf(helpStr, 63,"NVM Info: Number of Pages %d. Size of a page: %d bytes. \r\n", parameters.nvm_number_of_pages, parameters.page_size);
	SerialConsoleWriteString(helpStr);
	for(int i = 0; i < (parameters.nvm_number_of_pages / 4) - 288; i++){
		enum status_code nvmError = nvm_erase_row((APP_START_ADDRESS + i * 256));
		
		if(nvmError != STATUS_OK)
		{
			snprintf(helpStr, 63,"NVM ERROR: Erase error at row %d \r\n", i + 288);
			SerialConsoleWriteString(helpStr);
			return;
		}
	}
	for(int iter = 0; iter < 256; iter++)
	{
		char *a = (char *)(APP_START_ADDRESS + iter); //Pointer pointing to address APP_START_ADDRESS
		if(*a != 0xFF)
		{
			SerialConsoleWriteString("Error - test page is not erased!");
			break;
		}
	}
}
void read_bin_file(FIL file_object){
	char helpStr[64]; 
	uint32_t resultCrcSd = 0;
	uint32_t resultCrcNVM = 0;
	clean_buffer();
	uint8_t readBuffer[256];
int i =0;
UINT numBytesRead = 256;
//int numberBytesTotal = 0;
while(numBytesRead == 256){
	res = f_read(&file_object, &readBuffer[0],  256, &numBytesRead);
		SerialConsoleWriteString("read success \r\n");
	if(res != FR_OK){
		SerialConsoleWriteString("read error \r\n");
		f_close(&file_object);
		return false;
	}

	int cur_address = APP_START_ADDRESS + (i*256);
	
	//pad last data with 0xFF
	if(numBytesRead < 256){
		for(int iter = numBytesRead; iter < 256; iter++){
			readBuffer[iter] = 0xFF;
		}
	}

	res = nvm_write_buffer(APP_START_ADDRESS + (i*256), &readBuffer[0], 64);
	res = nvm_write_buffer(APP_START_ADDRESS + (i*256) + 64, &readBuffer[64], 64);
	res = nvm_write_buffer(APP_START_ADDRESS +(i*256) + 128, &readBuffer[128], 64);
	res = nvm_write_buffer(APP_START_ADDRESS + (i*256) + 192, &readBuffer[192], 64);
	
	*((volatile unsigned int*) 0x41007058) &= ~0x30000UL;
	dsu_crc32_cal(readBuffer, 256, &resultCrcSd);
	*((volatile unsigned int*) 0x41007058) |= 0x20000UL;
	
	
	//calculate NVM CRC chunck
	dsu_crc32_cal(APP_START_ADDRESS + (i*256), 256, &resultCrcNVM);
	
	i++;
}
	if(resultCrcSd != resultCrcNVM){
		SerialConsoleWriteString("CRC ERROR\r\n");
		snprintf(helpStr, 63,"CRC SD CARD: %d  CRC NVM: %d \r\n", resultCrcSd, resultCrcNVM);
		SerialConsoleWriteString(helpStr);
		//f_close(&file_object_bin);
	}
	
	//f_close(&file_object_bin);
}
