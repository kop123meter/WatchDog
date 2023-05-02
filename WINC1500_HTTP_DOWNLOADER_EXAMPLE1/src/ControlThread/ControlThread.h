/**************************************************************************/ /**
 * @file      ControlThread.h
 * @brief     Thread code for the ESE516 Online game control thread
 * @author    You!
 * @date      2020-04-015

 ******************************************************************************/

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "WifiHandlerThread/WifiHandler.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define CONTROL_TASK_SIZE 256  //<Size of stack to assign to the UI thread. In words
#define CONTROL_TASK_PRIORITY (configMAX_PRIORITIES - 1)
typedef enum controlStateMachine_state {
    CONTROL_WAIT_FOR_GAME = 0,  ///< State used to WAIT FOR A GAME COMMAND
    CONTROL_PLAYING_MOVE,       ///< State used to wait for user to play a game move
    CONTROL_END_GAME,           ///< State to show game end
    CONTROL_STATE_MAX_STATES    ///< Max number of states

} controlStateMachine_state;

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/
 struct GameDataPacket;
/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
void vControlHandlerTask(void *pvParameters);
int ControlAddGameData(struct GameDataPacket *gameIn);

#ifdef __cplusplus
}
#endif