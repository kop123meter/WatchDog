/**
 * @file      main.c
 * @brief     Main application entry point
 * @author    Eduardo Garcia
 * @date      2022-04-14
 * @copyright Copyright Bresslergroup\n
 *            This file is proprietary to Bresslergroup.
 *            All rights reserved. Reproduction or distribution, in whole
 *            or in part, is forbidden except by express written permission
 *            of Bresslergroup.
 ******************************************************************************/

/****
 * Includes
 ******************************************************************************/
#include <errno.h>

#include "CliThread/CliThread.h"
#include "ControlThread\ControlThread.h"
#include "DistanceDriver\DistanceSensor.h"
#include "FreeRTOS.h"
#include "IMU\lsm6dso_reg.h"
//#include "SeesawDriver/Seesaw.h"
#include "SerialConsole.h"
#include "UiHandlerThread\UiHandlerThread.h"
#include "I2cDriver/I2cDriver.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "asf.h"
#include "driver/include/m2m_wifi.h"
#include "main.h"
#include "stdio_serial.h"
#include "bme680/bme680.h"


/****
 * Defines and Types
 ******************************************************************************/
#define APP_TASK_ID 0 /**< @brief ID for the application task */
#define CLI_TASK_ID 1 /**< @brief ID for the command line interface task */

/****
 * Local Function Declaration
 ******************************************************************************/
void vApplicationIdleHook(void);
//!< Initial task used to initialize HW before other tasks are initialized
static void StartTasks(void);
void vApplicationDaemonTaskStartupHook(void);

void vApplicationStackOverflowHook(void);
void vApplicationMallocFailedHook(void);
void vApplicationTickHook(void);

/****
 * Variables
 ******************************************************************************/
static TaskHandle_t cliTaskHandle = NULL;      //!< CLI task handle
static TaskHandle_t daemonTaskHandle = NULL;   //!< Daemon task handle
static TaskHandle_t wifiTaskHandle = NULL;     //!< Wifi task handle
static TaskHandle_t uiTaskHandle = NULL;       //!< UI task handle
static TaskHandle_t controlTaskHandle = NULL;  //!< Control task handle

char bufferPrint[64];  ///< Buffer for daemon task

/**
 * @brief Main application function.
 * Application entry point.
 * @return int
 */
int main(void)
{
    /* Initialize the board. */
    system_init();

    /* Initialize the UART console. */
    InitializeSerialConsole();

    // Initialize trace capabilities
    vTraceEnable(TRC_START);
    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;  // Will not get here
}

/**
 * function          vApplicationDaemonTaskStartupHook
 * @brief            Initialization code for all subsystems that require FreeRToS
 * @details			This function is called from the FreeRToS timer task. Any code
 *					here will be called before other tasks are initilized.
 * @param[in]        None
 * @return           None
 */
void vApplicationDaemonTaskStartupHook(void)
{
    SerialConsoleWriteString("\r\n\r\n-----ESE516 Main Program-----\r\n");

    // Initialize HW that needs FreeRTOS Initialization
    SerialConsoleWriteString("\r\n\r\nInitialize HW...\r\n");
    if (I2cInitializeDriver() != STATUS_OK) {
        SerialConsoleWriteString("Error initializing I2C Driver!\r\n");
    } else {
        SerialConsoleWriteString("Initialized I2C Driver!\r\n");
    }
    /*
    if (0 != InitializeSeesaw()) {
        SerialConsoleWriteString("Error initializing Seesaw!\r\n");
    } else {
        SerialConsoleWriteString("Initialized Seesaw!\r\n");
    }*/

   
    uint8_t whoamI = 0;
    (lsm6dso_device_id_get(GetImuStruct(), &whoamI));

    if (whoamI != LSM6DSO_ID) {
        SerialConsoleWriteString("Cannot find IMU!\r\n");
    } else {
        SerialConsoleWriteString("IMU found!\r\n");
        if (InitImu() == 0) {
            SerialConsoleWriteString("IMU initialized!\r\n");
        } else {
            SerialConsoleWriteString("Could not initialize IMU\r\n");
        }
    }

/*
    SerialConsoleWriteString("Initializing distance sensor\r\n");
    InitializeDistanceSensor();
    SerialConsoleWriteString("Distance sensor initialized\r\n");
	*/

    StartTasks();

    vTaskSuspend(daemonTaskHandle);
}

/**
 * function          StartTasks
 * @brief            Initialize application tasks
 * @details
 * @param[in]        None
 * @return           None
 */
static void StartTasks(void)
{
    snprintf(bufferPrint, 64, "Heap before starting tasks: %d\r\n", xPortGetFreeHeapSize());
    SerialConsoleWriteString(bufferPrint);

    // Initialize Tasks here

    if (xTaskCreate(vCommandConsoleTask, "CLI_TASK", CLI_TASK_SIZE, NULL, CLI_PRIORITY, &cliTaskHandle) != pdPASS) {
        SerialConsoleWriteString("ERR: CLI task could not be initialized!\r\n");
    }

    snprintf(bufferPrint, 64, "Heap after starting CLI: %d\r\n", xPortGetFreeHeapSize());
    SerialConsoleWriteString(bufferPrint);

    if (xTaskCreate(vWifiTask, "WIFI_TASK", WIFI_TASK_SIZE, NULL, WIFI_PRIORITY, &wifiTaskHandle) != pdPASS) {
        SerialConsoleWriteString("ERR: WIFI task could not be initialized!\r\n");
    }
    snprintf(bufferPrint, 64, "Heap after starting WIFI: %d\r\n", xPortGetFreeHeapSize());
    SerialConsoleWriteString(bufferPrint);

    if (xTaskCreate(vUiHandlerTask, "UI Task", UI_TASK_SIZE, NULL, UI_TASK_PRIORITY, &uiTaskHandle) != pdPASS) {
        SerialConsoleWriteString("ERR: UI task could not be initialized!\r\n");
    }

    snprintf(bufferPrint, 64, "Heap after starting UI Task: %d\r\n", xPortGetFreeHeapSize());
    SerialConsoleWriteString(bufferPrint);

    if (xTaskCreate(vControlHandlerTask, "Control Task", CONTROL_TASK_SIZE, NULL, CONTROL_TASK_PRIORITY, &controlTaskHandle) != pdPASS) {
        SerialConsoleWriteString("ERR: Control task could not be initialized!\r\n");
    }
    snprintf(bufferPrint, 64, "Heap after starting Control Task: %d\r\n", xPortGetFreeHeapSize());
    SerialConsoleWriteString(bufferPrint);
}



void vApplicationMallocFailedHook(void)
{
    SerialConsoleWriteString("Error on memory allocation on FREERTOS!\r\n");
    while (1)
        ;
}

void vApplicationStackOverflowHook(void)
{
    SerialConsoleWriteString("Error on stack overflow on FREERTOS!\r\n");
    while (1)
        ;
}

#include "MCHP_ATWx.h"
void vApplicationTickHook(void)
{
    SysTick_Handler_MQTT();
}
