/**************************************************************************/ /**
 * @file      I2cDriver.h
 * @brief     FreeRTOS compatible driver for I2C communications
 * @author    Eduardo Garcia
 * @date      2020-04-05

 ******************************************************************************/

#ifndef I2C_DRIVER_H_
#define I2C_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <FreeRTOS.h>
#include <asf.h>
#include <semphr.h>
#include <task.h>

#include "i2c_master.h"
#include "i2c_master_interrupt.h"

#define I2C_INIT_ATTEMPTS 3
#define WAIT_I2C_LINE_MS 300

#define ERROR_NONE 0
#define ERROR_INVALID_DATA -1
#define ERROR_NO_CHANGE -2
#define ERROR_ABORTED -3
#define ERROR_BUSY -4
#define ERROR_SUSPEND -5
#define ERROR_IO -6
#define ERROR_REQ_FLUSHED -7
#define ERROR_TIMEOUT -8
#define ERROR_BAD_DATA -9
#define ERROR_NOT_FOUND -10
#define ERROR_UNSUPPORTED_DEV -11
#define ERROR_NO_MEMORY -12
#define ERROR_INVALID_ARG -13
#define ERROR_BAD_ADDRESS -14
#define ERROR_BAD_FORMAT -15
#define ERROR_BAD_FRQ -16
#define ERROR_DENIED -17
#define ERROR_ALREADY_INITIALIZED -18
#define ERROR_OVERFLOW -19
#define ERROR_NOT_INITIALIZED -20
#define ERROR_SAMPLERATE_UNAVAILABLE -21
#define ERROR_RESOLUTION_UNAVAILABLE -22
#define ERROR_BAUDRATE_UNAVAILABLE -23
#define ERROR_PACKET_COLLISION -24
#define ERROR_PROTOCOL -25
#define ERROR_PIN_MUX_INVALID -26
#define ERROR_UNSUPPORTED_OP -27
#define ERROR_NO_RESOURCE -28
#define ERROR_NOT_READY -29
#define ERROR_FAILURE -30
#define ERROR_WRONG_LENGTH -31
#define ERROR_RINGBUFFER_NO_SPACE_LEFT -32
#define ERROR_I2C_HANG_RESET -33

/// Defines the possible states that an I2C bus can be at
typedef enum eI2cBusState {
    I2C_BUS_READY = 0,   ///< The bus is ready to be used
    I2C_BUS_BUSY,        ///< The bus is currently busy performing an action
    I2C_BUS_MAX_STATES,  ///< Maximum number of allowable states of a bus
} eI2cBusState;

/// Structure that describes an I2C data, determining address to use, data buffer to send, etc.
typedef struct I2C_Data {
    uint8_t address;        ///< Address of the I2C device
    const uint8_t *msgOut;  ///< Pointer to array buffer that we will write from
    uint8_t *msgIn;         ///< Pointer to array buffer that we will get message to
    uint16_t lenIn;         ///< Length of message to read/write;
    uint16_t lenOut;        ///< Length of message to read/write;

} I2C_Data;

/// Structure that describes an I2C bus data, determining the bus and the flags
typedef struct I2C_Bus_State {
    eI2cBusState i2cState;  ///< Holds the state of a I2C_Bus.
    uint32_t txDoneFlag;    ///< Flag that gets set when we have completed a previous SEND command. Useful for function to determine when the system has finished writing data.
    uint32_t rxDoneFlag;    ///< Flag that gets set when we have completed a previous READ command. Useful for function to determine when the system has finished reading data.
    uint16_t currentAddress;
    uint16_t registerRead;
    uint16_t readLen;

} I2C_Bus_State;

int32_t I2cReadDataWait(I2C_Data *data, const TickType_t delay, const TickType_t xMaxBlockTime);
int32_t I2cWriteDataWait(I2C_Data *data, const TickType_t xMaxBlockTime);
int32_t I2cGetMutex(TickType_t waitTime);
int32_t I2cFreeMutex(void);
int32_t I2cReadData(I2C_Data *data);
int32_t I2cWriteData(I2C_Data *data);
int32_t I2cInitializeDriver(void);
void I2cDriverRegisterSensorBusCallbacks(void);
void I2cSensorsError(struct i2c_master_module *const module);
void I2cSensorsRxComplete(struct i2c_master_module *const module);
void I2cSensorsTxComplete(struct i2c_master_module *const module);
int32_t I2cPingAddressWait(I2C_Data *data, const TickType_t delay, const TickType_t xMaxBlockTime);

#ifdef __cplusplus
}
#endif

#endif /* I2C_DRIVER_H_ */
