/**************************************************************************/ /**
 * @file      DistanceSensor.c
 * @brief     Driver for the US-100 Distance Sensor, in SERIAL MODE. This means the jumper must remain on!
 In this mode, it will work with an UART driver. The MCU can send 0x55 and should receive two bytes indicating the distance in mm (high byte, low byte)
 If you send 0x50, it will return the temperature in Degrees C.

 This criver will be written compatible to be run from RTOS thread, with non-blocking commands in mind.
 See https://www.bananarobotics.com/shop/US-100-Ultrasonic-Distance-Sensor-Module for more information
 * @author    Eduardo Garcia
 * @date      2020-04-08

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "DistanceDriver/DistanceSensor.h"

#include "I2cDriver/I2cDriver.h"
#include "SerialConsole/SerialConsole.h"

/******************************************************************************
 * Variables
 ******************************************************************************/

struct usart_module usart_instance_dist;  ///< Distance sensor UART module

SemaphoreHandle_t sensorDistanceMutexHandle;      ///< Mutex to handle the sensor I2C bus thread access.
SemaphoreHandle_t sensorDistanceSemaphoreHandle;  ///< Binary semaphore to notify task that we have received an I2C interrupt on the Sensor bus

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/
uint8_t distTx;
uint8_t latestRxDistance[2];
/******************************************************************************
 *  Callback Declaration
 ******************************************************************************/
// Callback for when we finish writing characters to UART
void distUsartWritecallback(struct usart_module *const usart_module)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(sensorDistanceSemaphoreHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
// Callback for when we finish writing characters to UART

void distUsartReadcallback(struct usart_module *const usart_module)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(sensorDistanceSemaphoreHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/******************************************************************************
 * Local Function Declaration
 ******************************************************************************/
static void configure_usart(void);
static void configure_usart_callbacks(void);
static int32_t DistanceSensorFreeMutex(void);
static int32_t DistanceSensorGetMutex(TickType_t waitTime);
/******************************************************************************
 * Global Local Variables
 ******************************************************************************/

/******************************************************************************
 * Global Functions
 ******************************************************************************/

/**
 * @fn			void InitializeSerialConsole(void)
 * @brief		Initializes the UART - sets up the SERCOM to act as UART and registers the callbacks for
 *				asynchronous reads and writes.
 * @details		Initializes the UART - sets up the SERCOM to act as UART and registers the callbacks for
 *				asynchronous reads and writes.
 * @note			Call from main once to initialize Hardware.
 */

void InitializeDistanceSensor(void)
{
    // Configure USART and Callbacks
    configure_usart();
    configure_usart_callbacks();

    sensorDistanceMutexHandle = xSemaphoreCreateMutex();
    sensorDistanceSemaphoreHandle = xSemaphoreCreateBinary();

    if (NULL == sensorDistanceMutexHandle || NULL == sensorDistanceSemaphoreHandle) {
        SerialConsoleWriteString((char *)"Could not initialize Distance Sensor!");
    }
}

/**
 * @fn			void DeinitializeSerialConsole(void)
 * @brief		Deinitialises the UART
 * @note
 */
void DeinitializeDistanceSerial(void)
{
    usart_disable(&usart_instance_dist);
}

/**
 * @fn			int32_t DistanceSensorGetDistance (uint16_t *distance)
 * @brief		Gets the distance from the distance sensor.
 * @note			Returns 0 if successful. -1 if an error occurred
 */
int32_t DistanceSensorGetDistance(uint16_t *distance, const TickType_t xMaxBlockTime)
{
    int error = ERROR_NONE;

    // 1. Get MUTEX. DistanceSensorGetMutex. If we cant get it, goto
    error = DistanceSensorGetMutex(WAIT_I2C_LINE_MS);
    if (ERROR_NONE != error) goto exitf;

    //---2. Initiate sending data. First populate TX with the distance command. Use usart_write_buffer_job to transmit 1 character
    distTx = DISTANCE_US_100_CMD_READ_DISTANCE;
    if (STATUS_OK != usart_write_buffer_job(&usart_instance_dist, (uint8_t *)&distTx, 1)) {
        goto exitf;
    }

    // 3. )Wait until the TX finished. TX should release the binary semaphore - so wait until semaphore
    if (xSemaphoreTake(sensorDistanceSemaphoreHandle, xMaxBlockTime) == pdTRUE) {
        /* The transmission ended as expected. We now delay until the I2C sensor is finished */

    } else {
        /* The call to ulTaskNotifyTake() timed out. */
        error = ERR_TIMEOUT;
        goto exitf;
    }

    // 4. Initiate an rx job - usart_read_buffer_job - to read two characters. Read into variable latestRxDistance
    usart_read_buffer_job(&usart_instance_dist, (uint8_t *)&latestRxDistance, 2);  // Kicks off constant reading of characters

    //---7. Wait for notification
    if (xSemaphoreTake(sensorDistanceSemaphoreHandle, xMaxBlockTime) == pdTRUE) {
        /* The transmission ended as expected. We now delay until the I2C sensor is finished */
        *distance = (latestRxDistance[0] << 8) + latestRxDistance[1];
    } else {
        /* The call to ulTaskNotifyTake() timed out. */
        error = ERR_TIMEOUT;
        goto exitf;
    }

exitf:
    // Release mutex and return error
    DistanceSensorFreeMutex();

    return error;
}

/**
 * @fn			static void configure_usart(void)
 * @brief		Code to configure the SERCOM "EDBG_CDC_MODULE" to be a UART channel running at 115200 8N1
 * @note
 */
static void configure_usart(void)
{
    struct usart_config config_usart;
    usart_get_config_defaults(&config_usart);

    config_usart.baudrate = 9600;
    config_usart.mux_setting = USART_RX_1_TX_0_XCK_1;
    config_usart.pinmux_pad0 = PINMUX_PB02D_SERCOM5_PAD0;
    config_usart.pinmux_pad1 = PINMUX_PB03D_SERCOM5_PAD1;
    config_usart.pinmux_pad2 = PINMUX_UNUSED;
    config_usart.pinmux_pad3 = PINMUX_UNUSED;

    while (usart_init(&usart_instance_dist, SERCOM5, &config_usart) != STATUS_OK) {
    }

    usart_enable(&usart_instance_dist);
}

/**
 * @fn			static void configure_usart_callbacks(void)
 * @brief		Code to register callbacks
 * @note
 */
static void configure_usart_callbacks(void)
{
    usart_register_callback(&usart_instance_dist, distUsartWritecallback, USART_CALLBACK_BUFFER_TRANSMITTED);
    usart_register_callback(&usart_instance_dist, distUsartReadcallback, USART_CALLBACK_BUFFER_RECEIVED);
    usart_enable_callback(&usart_instance_dist, USART_CALLBACK_BUFFER_TRANSMITTED);
    usart_enable_callback(&usart_instance_dist, USART_CALLBACK_BUFFER_RECEIVED);
}

/**
 * @fn			int32_t DistanceSensorFreeMutex(eI2cBuses bus)
 * @brief       Frees the mutex of the given UART bus
 * @details
 * @param[in]   bus Enum that represents the bus in which we are interested to free the mutex of.
 * @return      Returns (0) if the bus is ready, (1) if it is busy.
 * @note
 */
static int32_t DistanceSensorFreeMutex(void)
{
    int32_t error = ERROR_NONE;

    if (xSemaphoreGive(sensorDistanceMutexHandle) != pdTRUE) {
        error = ERROR_NOT_INITIALIZED;  // We could not return the mutex! We must not have it!
    }
    return error;
}

/**
 * @fn			int32_t I2cGetMutex(TickType_t waitTime)
 * @brief       Frees the mutex of the given UART bus
 * @details
 * @param[in]   waitTime Time to wait for the mutex to be freed.
 * @return      Returns (0) if the bus is ready, (1) if it is busy.
 * @note
 */
static int32_t DistanceSensorGetMutex(TickType_t waitTime)
{
    int32_t error = ERROR_NONE;
    if (xSemaphoreTake(sensorDistanceMutexHandle, waitTime) != pdTRUE) {
        error = ERROR_NOT_READY;
    }
    return error;
}