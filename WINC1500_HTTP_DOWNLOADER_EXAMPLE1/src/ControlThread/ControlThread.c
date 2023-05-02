/**************************************************************************/ /**
 * @file      ControlThread.c
 * @brief     Thread code for the ESE516 Online game control thread
 * @author    You!
 * @date      2020-04-015

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ControlThread/ControlThread.h"

#include <errno.h>

#include "SerialConsole.h"
#include "UiHandlerThread/UiHandlerThread.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "asf.h"
#include "main.h"
#include "shtc3.h"
#include "stdio_serial.h"

/******************************************************************************
 * Defines
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/
QueueHandle_t xQueueGameBufferIn = NULL;    ///< Queue to send the next play to the UI
QueueHandle_t xQueueRgbColorBuffer = NULL;  ///< Queue to receive an LED Color packet

controlStateMachine_state controlState;  ///< Holds the current state of the control thread

/******************************************************************************
 * Forward Declarations
 ******************************************************************************/

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/******************************************************************************
 * Task Functions
 ******************************************************************************/

/**
 * @fn		void vControlHandlerTask( void *pvParameters )
 * @brief	STUDENT TO FILL THIS
 * @details 	student to fill this

 * @param[in]	Parameters passed when task is initialized. In this case we can ignore them!
 * @return		Should not return! This is a task defining function.
 * @note
 */
void vControlHandlerTask(void *pvParameters)
{
    SerialConsoleWriteString((char *)"ESE516 - Control Init Code\r\n");

    // Initialize Queues
    xQueueGameBufferIn = xQueueCreate(2, sizeof(struct GameDataPacket));
    xQueueRgbColorBuffer = xQueueCreate(2, sizeof(struct RgbColorPacket));

    if (xQueueGameBufferIn == NULL || xQueueRgbColorBuffer == NULL) {
        SerialConsoleWriteString((char *)"ERROR Initializing Control Data queues!\r\n");
    }
    controlState = CONTROL_WAIT_FOR_GAME;  // Initial state

    while (1) {
        switch (controlState) {
            case (CONTROL_WAIT_FOR_GAME): {  // Should set the UI to ignore button presses and should wait until there is a message from the server with a new play.
                struct GameDataPacket gamePacketIn;
                if (pdPASS == xQueueReceive(xQueueGameBufferIn, &gamePacketIn, 0)) {
                    LogMessage(LOG_DEBUG_LVL, "Control Thread: Consumed game packet!\r\n");
                    UiOrderShowMoves(&gamePacketIn);
                    controlState = CONTROL_PLAYING_MOVE;
                }

                break;
            }

            case (CONTROL_PLAYING_MOVE): {  // Should wait until the UI thread has showed the move AND comes back with the play from the user. Should go back to CONTROL_WAIT_FOR_GAME
                // after posting the game to MQTT
                if (UiPlayIsDone() == true) {
                    // Send back local game packet
                    if (pdTRUE != WifiAddGameDataToQueue(UiGetGamePacketOut())) {
                        LogMessage(LOG_DEBUG_LVL, "Control Thread: Could not send game packet!\r\n");
                    }
                    controlState = CONTROL_WAIT_FOR_GAME;
                }

                break;
            }

            case (CONTROL_END_GAME): {
                break;
            }

            default:
                controlState = CONTROL_WAIT_FOR_GAME;
                break;
        }
        vTaskDelay(40);
    }
}

/**
 int ControlAddGameData(struct GameDataPacket *gameIn);
 * @brief	Adds an game data received from the internet to the local control for play
 * @param[out]

 * @return		Returns pdTrue if data can be added to queue, 0 if queue is full
 * @note

 */
int ControlAddGameData(struct GameDataPacket *gameIn)
{
    int error = xQueueSend(xQueueGameBufferIn, gameIn, (TickType_t)10);
    return error;
}
