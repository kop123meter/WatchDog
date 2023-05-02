/**********************************************************************//**
* @file      CliThread.h
* @brief     File for the CLI Thread handler. Uses FREERTOS + CLI
* @author    Eduardo Garcia
* @date      2020-02-15

******************************************************************************/

#pragma once

#include "asf.h"
#include "SerialConsole.h"
#include "FreeRTOS_CLI.h"


#define CLI_TASK_SIZE	400		
#define CLI_PRIORITY (configMAX_PRIORITIES - 1) 
#define CLI_TASK_DELAY 150	

#define MAX_INPUT_LENGTH_CLI    50	
#define MAX_OUTPUT_LENGTH_CLI   100	

#define CLI_MSG_LEN						16
#define CLI_PC_ESCAPE_CODE_SIZE			4
#define CLI_PC_MIN_ESCAPE_CODE_SIZE		2


#define ASCII_BACKSPACE					0x08
#define ASCII_DELETE                    0x7F
#define ASCII_WHITESPACE				0x20
#define ASCII_ESC						27


BaseType_t xCliClearTerminalScreen( char *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString );

#define	CLI_COMMAND_CLEAR_SCREEN		"cls"
#define CLI_HELP_CLEAR_SCREEN			"cls: Clears the terminal screen\r\n"
#define CLI_CALLBACK_CLEAR_SCREEN		(pdCOMMAND_LINE_CALLBACK)xCliClearTerminalScreen
#define CLI_PARAMS_CLEAR_SCREEN			0


void vCommandConsoleTask( void *pvParameters );
void CliCharReadySemaphoreGiveFromISR(void);

BaseType_t CLI_GetImuData( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString );
BaseType_t CLI_OTAU( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString );
BaseType_t CLI_NeotrellisSetLed( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString );
BaseType_t CLI_NeotrellProcessButtonBuffer( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString );
BaseType_t CLI_DistanceSensorGetDistance( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString );
BaseType_t CLI_ResetDevice( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString );
BaseType_t CLI_SendDummyGameData(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_i2cScan(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_RAP(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_CLLCD(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_UNLOCK(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_LOCK(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_BME(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);