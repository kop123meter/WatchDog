/**************************************************************************/ /**
 * @file      I2cDriver.c
 * @brief     FreeRTOS compatible driver for I2C communications
 * @author    Eduardo Garcia
 * @date      2020-04-05

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "I2cDriver.h"

/******************************************************************************
 * Defines
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/
SemaphoreHandle_t sensorI2cMutexHandle;                       ///< Mutex to handle the sensor I2C bus thread access.
SemaphoreHandle_t sensorI2cSemaphoreHandle;                   ///< Binary semaphore to notify task that we have received an I2C interrupt on the Sensor bus
static volatile TaskHandle_t xTaskToNotifySensorDone = NULL;  ///< Stores the handle of the task that will be notified when the SENSOR transmission is complete. */
static uint8_t sensorTransmitError = false;                   ///< Flag used to indicate that there was an I2C transmission error on the SENSOR bus.

struct i2c_master_module i2cSensorBusInstance;
static I2C_Bus_State I2cSensorBusState;  ///< Structure that defines the I2C Bus used for the sensors.

struct i2c_master_packet sensorPacketWrite;
/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static int32_t I2cDriverConfigureSensorBus(void)
{
    int32_t error = STATUS_OK;
    status_code_genare_t errCodeAsf = STATUS_OK;
    /* Initialize config structure and software module */
    struct i2c_master_config config_i2c_master;
    i2c_master_get_config_defaults(&config_i2c_master);

    config_i2c_master.pinmux_pad0 = PINMUX_PA08C_SERCOM0_PAD0;
    config_i2c_master.pinmux_pad1 = PINMUX_PA09C_SERCOM0_PAD1;
    /* Change buffer timeout to something longer */
    config_i2c_master.buffer_timeout = 1000;
    /* Initialize and enable device with config. Try three times to initialize */

    for (uint8_t i = I2C_INIT_ATTEMPTS; i != 0; i--) {
        errCodeAsf = i2c_master_init(&i2cSensorBusInstance, SERCOM0, &config_i2c_master);
        if (STATUS_OK == errCodeAsf) {
            error = errCodeAsf;
            break;
        } else {
            i2c_master_reset(&i2cSensorBusInstance);
        }
    }

    if (STATUS_OK != error) goto exit;

    i2c_master_enable(&i2cSensorBusInstance);

exit:
    return error;
}
/******************************************************************************
 * Callback Functions
 ******************************************************************************/
/*
  * @fn			void I2cSensorsTxComplete(struct i2c_m_async_desc *const i2c)
  * @brief       Callback function for when the SENSORS I2C bus ends transmissions
  * @details     This callback sets a flag that tells us that the bus is not busy, and the last transmission is done. With USE_FREERTOS flag, this
                                 callback notifies the registered thread that the I2C transfer has finished. The registered thread is the thread that initiated the I2C transaction,
                                 and is currently waiting for a notification that it has finished.
  * @param[in]   i2c Pointer to I2C structure used inside the Atmel ASFv3  framework
  * @return      This function is a callback, and it is registered as such when we send an I2C transmission on this I2C bus.
  * @note
  */
void I2cSensorsTxComplete(struct i2c_master_module *const module)
{
    I2cSensorBusState.i2cState = I2C_BUS_READY;
    I2cSensorBusState.rxDoneFlag = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(sensorI2cSemaphoreHandle, &xHigherPriorityTaskWoken);
    sensorTransmitError = false;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
  * @fn				void I2cSensorRxComplete(struct i2c_m_async_desc *const i2c)
  * @brief			Callback function for when the SENSOR I2C bus ends data reception
  * @details			This callback sets a flag that tells us that the bus is not busy, and the last reception is done. With USE_FREERTOS flag, this
                                         callback notifies the registered thread that the I2C transfer has finished. The registered thread is the thread that initiated the I2C transaction,
                                         and is currently waiting for a notification that it has finished.
  * @param[in]		i2c Pointer to I2C structure used inside the Atmel ASFv3  framework
  * @return			This function is a callback, and it is registered as such when we send an I2C reception on this I2C bus.
  * @note
  */
void I2cSensorsRxComplete(struct i2c_master_module *const module)
{
    I2cSensorBusState.i2cState = I2C_BUS_READY;
    I2cSensorBusState.rxDoneFlag = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(sensorI2cSemaphoreHandle, &xHigherPriorityTaskWoken);
    sensorTransmitError = false;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
  * @fn				void I2cSensorError(struct i2c_m_async_desc *const i2c)
  * @brief			Callback function for when the LED I2C bus encounters an error while transmitting/receiving
  * @details			This callback sets a flag that tells us that the bus is not busy, and the last reception is done. With USE_FREERTOS flag, this
                                         callback notifies the registered thread that the I2C transfer has finished. The registered thread is the thread that initiated the I2C transaction,
                                         and is currently waiting for a notification that it has finished.
  * @param[in]		i2c Pointer to I2C structure used inside the Atmel ASFv3  framework
  * @return			This function is a callback, and it is registered as such when we send an I2C reception on this I2C bus.
  * @note
  */
void I2cSensorsError(struct i2c_master_module *const module)
{
    I2cSensorBusState.i2cState = I2C_BUS_READY;
    I2cSensorBusState.txDoneFlag = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(sensorI2cSemaphoreHandle, &xHigherPriorityTaskWoken);
    sensorTransmitError = true;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void I2cDriverRegisterSensorBusCallbacks(void)
{
    /* Register callback function. */
    i2c_master_register_callback(&i2cSensorBusInstance, I2cSensorsTxComplete, I2C_MASTER_CALLBACK_WRITE_COMPLETE);
    i2c_master_enable_callback(&i2cSensorBusInstance, I2C_MASTER_CALLBACK_WRITE_COMPLETE);

    i2c_master_register_callback(&i2cSensorBusInstance, I2cSensorsRxComplete, I2C_MASTER_CALLBACK_READ_COMPLETE);
    i2c_master_enable_callback(&i2cSensorBusInstance, I2C_MASTER_CALLBACK_READ_COMPLETE);

    i2c_master_register_callback(&i2cSensorBusInstance, I2cSensorsError, I2C_MASTER_CALLBACK_ERROR);
    i2c_master_enable_callback(&i2cSensorBusInstance, I2C_MASTER_CALLBACK_ERROR);
}

/**
 * @fn			int32_t I2cInitializeDriver(void)
 * @brief       Function call to initialize the I2C driver\
 * @details     This function must be called from an RTOS thread if using RTOS, and must be called before any I2C call
 * @note
 */
int32_t I2cInitializeDriver(void)
{
    int32_t error = STATUS_OK;

    error = I2cDriverConfigureSensorBus();
    if (STATUS_OK != error) goto exit;

    I2cDriverRegisterSensorBusCallbacks();

    sensorI2cMutexHandle = xSemaphoreCreateMutex();

    sensorI2cSemaphoreHandle = xSemaphoreCreateBinary();
    // xSemaphoreGive(sensorI2cSemaphoreHandle);

    if (NULL == sensorI2cMutexHandle || NULL == sensorI2cSemaphoreHandle) {
        error = STATUS_SUSPEND;  // Could not initialize mutex!
        goto exit;
    }

exit:
    return error;
}

/**
 * @fn    int32_t I2cWriteData(I2C_Data *data)
 * @brief       Function call to write an specified number of bytes on the given I2C bus
 * @details
 * @param[in]   data Pointer to I2C data structure which has all the information needed to send an I2C message
 * @return      Returns an error message in case of error. See ErrCodes.h
 * @note
 */

int32_t I2cWriteData(I2C_Data *data)
{
    int32_t error = ERROR_NONE;
    enum status_code hwError;

    // Check parameters
    if (data == NULL || data->msgOut == NULL) {
        error = ERR_INVALID_ARG;
        goto exit;
    }

    // Prepare to write
    sensorPacketWrite.address = data->address;
    sensorPacketWrite.data = (uint8_t *)data->msgOut;
    sensorPacketWrite.data_length = data->lenOut;

    // Write

    hwError = i2c_master_write_packet_job(&i2cSensorBusInstance, &sensorPacketWrite);

    if (STATUS_OK != hwError) {
        error = ERROR_IO;
        goto exit;
    }

exit:
    return error;
}

/**
 * @fn    int32_t I2cReadData(I2C_Data *data)
 * @brief       Function call to read an specified number of bytes on the given I2C bus
 * @details
 * @param[in]   data Pointer to I2C data structure which has all the information needed to send an I2C message
 * @return      Returns an error message in case of error. See ErrCodes.h
 * @note
 */
int32_t I2cReadData(I2C_Data *data)
{
    int32_t error = ERROR_NONE;
    enum status_code hwError;

    // Check parameters
    if (data == NULL || data->msgOut == NULL) {
        error = ERR_INVALID_ARG;
        goto exit;
    }

    // Prepare to read
    sensorPacketWrite.address = data->address;
    sensorPacketWrite.data = data->msgIn;
    sensorPacketWrite.data_length = data->lenIn;

    // Read

    hwError = i2c_master_read_packet_job(&i2cSensorBusInstance, &sensorPacketWrite);

    if (STATUS_OK != hwError) {
        error = ERROR_IO;
        goto exit;
    }

exit:
    return error;
}

/**
 * @fn			int32_t I2cFreeMutex(eI2cBuses bus)
 * @brief       Frees the mutex of the given I2C bus
 * @details
 * @param[in]   bus Enum that represents the bus in which we are interested to free the mutex of.
 * @return      Returns (0) if the bus is ready, (1) if it is busy.
 * @note
 */
int32_t I2cFreeMutex(void)
{
    int32_t error = ERROR_NONE;

    if (xSemaphoreGive(sensorI2cMutexHandle) != pdTRUE) {
        error = ERROR_NOT_INITIALIZED;  // We could not return the mutex! We must not have it!
    }
    return error;
}

/**
 * @fn			int32_t I2cGetMutex(TickType_t waitTime)
 * @brief       Frees the mutex of the given I2C bus
 * @details
 * @param[in]   waitTime Time to wait for the mutex to be freed.
 * @return      Returns (0) if the bus is ready, (1) if it is busy.
 * @note
 */
int32_t I2cGetMutex(TickType_t waitTime)
{
    int32_t error = ERROR_NONE;
    if (xSemaphoreTake(sensorI2cMutexHandle, waitTime) != pdTRUE) {
        error = ERROR_NOT_READY;
    }
    return error;
}

static int32_t I2cGetSemaphoreHandle(SemaphoreHandle_t *handle)
{
    int32_t error = ERROR_NONE;
    *handle = sensorI2cSemaphoreHandle;
    return error;
}

/**
 * @fn			static uint8_t I2cGetTaskErrorStatus(I2C_Data *data)
 * @brief       Sets the error state of the latest I2C bus transaction for a given I2C data, which holds which physical I2C
 *bus we are using.
 * @details     This error flag is set to indicate when a I2C transaction has failed on a given I2C bus (encapsulated in
 *the I2C data structure).
 * @return      Returns an error message in case of error. See ErrCodes.h
 * @note
 */
static uint8_t I2cGetTaskErrorStatus(void)
{
    return sensorTransmitError;
}

/**
 * @fn			static uint8_t I2cSetTaskErrorStatus(I2C_Data *data, uint8_t value)
 * @brief       Sets the error state of the latest I2C bus transaction for a given I2C data, which holds which physical I2C
 *bus we are using.
 * @details     This error flag is set to indicate when a I2C transaction has failed on a given I2C bus (encapsulated in
 *the I2C data structure).
 * @param[in]   value Value to which to set the error flag to.
 * @return      Returns an error message in case of error. See ErrCodes.h
 * @note
 */
static void I2cSetTaskErrorStatus(uint8_t value)
{
    sensorTransmitError = value;
}

/**
  * @fn			int32_t I2cWriteDataWait(I2C_Data *data, const TickType_t delay, const TickType_t xMaxBlockTime)
  * @brief       This is the main function to use to write data from an I2C device on a given I2C Bus. This function is blocking.
  * @details     This function writes data from an I2C device, by writing the requested bytes.This function is blocking (bare-metal) or it
                                 makes the current thread sleep until the I2C bus has finished the transaction (FREERTOS version).
                                 On FreeRtos, this function gets the mutex for the respective I2C bus.
  * @param[in]   data Pointer to I2C data structure which has all the information needed to send an I2C message
  * @param[in]   delay Delay that the I2C device needs to return the response. Can be 0 if the response is ready instantly. It can be
                                 the delay an I2C device needs to make a measurement.
  * @param[in]   xMaxBlockTime Maximum time for the thread to wait until the I2C mutex is free.
  * @return      Returns an error message in case of error.
  * @note
  */
int32_t I2cWriteDataWait(I2C_Data *data, const TickType_t xMaxBlockTime)
{
    int32_t error = ERROR_NONE;
    SemaphoreHandle_t semHandle = NULL;

    //---0. Get Mutex
    error = I2cGetMutex(WAIT_I2C_LINE_MS);
    if (ERROR_NONE != error) goto exit;

    //---1. Get Semaphore Handle
    error = I2cGetSemaphoreHandle(&semHandle);
    if (ERROR_NONE != error) goto exit;

    //---2. Initiate sending data

    error = I2cWriteData(data);
    if (ERROR_NONE != error) {
        goto exitError0;
    }

    //---2. Wait for binary semaphore to tell us that we are done!
    if (xSemaphoreTake(semHandle, xMaxBlockTime) == pdTRUE) {
        /* The transmission ended as expected. We now delay until the I2C sensor is finished */
        if (I2cGetTaskErrorStatus()) {
            I2cSetTaskErrorStatus(false);
            if (error != ERROR_NONE) {
                error = ERROR_I2C_HANG_RESET;
            } else {
                error = ERROR_ABORTED;
            }
            goto exitError0;
        }
    } else {
        /* The call to ulTaskNotifyTake() timed out. */
        error = ERR_TIMEOUT;
        goto exitError0;
    }

    //---8. Release Mutex
    error |= I2cFreeMutex();
// xSemaphoreGive(semHandle);
exit:
    return error;

exitError0:
    error |= I2cFreeMutex();
    // xSemaphoreGive(semHandle);
    return error;
}

/**
  * @fn			int32_t I2cReadDataWait(I2C_Data *data, const TickType_t delay, const TickType_t xMaxBlockTime)
  * @brief       This is the main function to use to read data from an I2C device on a given I2C Bus. This function is blocking.
  * @details     This function reads data from an I2C device, by first writing to the address (I2C device address + register) and then reading the requested bytes. This
                                 function is blocking (bare-metal) or it makes the current thread sleep until the I2C bus has finished the transaction (FREERTOS version).
                                 On FreeRtos, this function gets the mutex for the respective I2C bus.
  * @param[in]   data Pointer to I2C data structure which has all the information needed to send an I2C message
  * @param[in]   delay Delay that the I2C device needs to return the response. Can be 0 if the response is ready instantly. It can be the delay an I2C device needs to make a measurement.
  * @param[in]   xMaxBlockTime Maximum time for the thread to wait until the I2C mutex is free.
  * @return      Returns an error message in case of error. See ErrCodes.h
  * @note        THIS IS THE FREERTOS VERSION! DO NOT Declare #define USE_FREERTOS if you wish to use the baremetal version!
  */
int32_t I2cReadDataWait(I2C_Data *data, const TickType_t delay, const TickType_t xMaxBlockTime)
{
    int32_t error = ERROR_NONE;
    SemaphoreHandle_t semHandle = NULL;

    //---0. Get Mutex
    error = I2cGetMutex(WAIT_I2C_LINE_MS);
    if (ERROR_NONE != error) goto exit;

    //---1. Get Semaphore Handle
    error = I2cGetSemaphoreHandle(&semHandle);
    if (ERROR_NONE != error) goto exit;

    //---2. Initiate sending data

    error = I2cWriteData(data);
    if (ERROR_NONE != error) {
        goto exitError0;
    }

    //---2. Wait for binary semaphore to tell us that we are done!
    if (xSemaphoreTake(semHandle, xMaxBlockTime) == pdTRUE) {
        /* The transmission ended as expected. We now delay until the I2C sensor is finished */
        if (I2cGetTaskErrorStatus()) {
            I2cSetTaskErrorStatus(false);
            error = ERROR_ABORTED;
            goto exitError0;
        }
        vTaskDelay(delay);
    } else {
        /* The call to ulTaskNotifyTake() timed out. */
        error = ERR_TIMEOUT;
        goto exitError0;
    }

    //---6. Initiate Read data
    error = I2cReadData(data);
    if (ERROR_NONE != error) {
        goto exitError0;
    }
    //---7. Wait for notification
    if (xSemaphoreTake(semHandle, xMaxBlockTime) == pdTRUE) {
        /* The transmission ended as expected. We now delay until the I2C sensor is finished */
        if (I2cGetTaskErrorStatus()) {
            I2cSetTaskErrorStatus(false);
            error = ERROR_ABORTED;
            goto exitError0;
        }
    } else {
        /* The call to ulTaskNotifyTake() timed out. */
        error = ERR_TIMEOUT;
        goto exitError0;
    }

    //---8. Release Mutex
    error = I2cFreeMutex();
// xSemaphoreGive(semHandle);
exit:
    return error;

exitError0:
    I2cFreeMutex();
    // xSemaphoreGive(semHandle);
    return error;
}


/**
  * @fn			int32_t I2cPingAddressWait(I2C_Data *data, const TickType_t delay, const TickType_t xMaxBlockTime)
  * @brief       Pings an address.
  * @details     
  * @param[in]   data Pointer to I2C data structure which has all the information needed to send an I2C message
  * @param[in]   delay Delay that the I2C device needs to return the response. Can be 0 if the response is ready instantly. It can be the delay an I2C device needs to make a measurement.
  * @param[in]   xMaxBlockTime Maximum time for the thread to wait until the I2C mutex is free.
  * @return      Returns an error message in case of error. See ErrCodes.h
  * @note        THIS IS THE FREERTOS VERSION! DO NOT Declare #define USE_FREERTOS if you wish to use the baremetal version!
  */
int32_t I2cPingAddressWait(I2C_Data *data, const TickType_t delay, const TickType_t xMaxBlockTime)
{
    int32_t error = ERROR_NONE;
    SemaphoreHandle_t semHandle = NULL;

    //---0. Get Mutex
    error = I2cGetMutex(WAIT_I2C_LINE_MS);
    if (ERROR_NONE != error) goto exit;

    //---1. Get Semaphore Handle
    error = I2cGetSemaphoreHandle(&semHandle);
    if (ERROR_NONE != error) goto exit;

    //---2. Initiate sending data

    error = I2cWriteData(data);
    if (ERROR_NONE != error) {
        goto exitError0;
    }

    //---2. Wait for binary semaphore to tell us that we are done!
    if (xSemaphoreTake(semHandle, xMaxBlockTime) == pdTRUE) {
        /* The transmission ended as expected. We now delay until the I2C sensor is finished */
        if (I2cGetTaskErrorStatus()) {
            I2cSetTaskErrorStatus(false);
            error = ERROR_ABORTED;
            goto exitError0;
        }
        vTaskDelay(delay);
    } else {
        /* The call to ulTaskNotifyTake() timed out. */
        error = ERR_TIMEOUT;
        goto exitError0;
    }


    //---8. Release Mutex
    error = I2cFreeMutex();
exit:
    return error;

exitError0:
    I2cFreeMutex();
    // xSemaphoreGive(semHandle);
    return error;
}
