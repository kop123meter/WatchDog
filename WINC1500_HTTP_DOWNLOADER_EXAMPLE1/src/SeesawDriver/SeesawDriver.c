/**
 * @file      Seesaw.c
 * @brief     Driver for the seesaw 4x4 LED button from Adafruit. See https://cdn-learn.adafruit.com/downloads/pdf/adafruit-seesaw-atsamd09-breakout.pdf
  and https://github.com/adafruit/Adafruit_Seesaw

 * @author    Eduardo Garcia
 * @date      2020-01-01

 ******************************************************************************/

#include "I2cDriver/I2cDriver.h"
#include "Seesaw.h"
#include "SerialConsole.h"

/******************************************************************************
 * Includes
 ******************************************************************************/

/******************************************************************************
 * Defines
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/
I2C_Data seesawData;  ///< Global variable to use for I2C communications with the Seesaw Device
/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
const uint8_t msgBaseGetHWID[2] = {SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID};                                              ///< Message to get HW ID from
const uint8_t msgNeopixelPin[3] = {SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_PIN, NEO_TRELLIS_NEOPIX_PIN};                    ///< Message to set Neopixel to a PORTA pin
const uint8_t msgNeopixelSpeed[3] = {SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SPEED, 0x01};                                  ///< Message to set neopixel speed
const uint8_t msgNeopixelBufLength[4] = {SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF_LENGTH, 0, NEO_TRELLIS_NUM_KEYS * 3};  ///< Message to tell Seesaw the number of neopixels

const uint8_t msgKeypadEnableInt[3] = {SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_INTENSET, 0x01};  ///< Message to enable Keypad Interrupts
const uint8_t msgKeypadGetCount[2] = {SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_COUNT};            ///< Get count of active events
/******************************************************************************
 * Callback Functions
 ******************************************************************************/

static void SeesawTurnOnLedTest(void);
static void SeesawInitializeKeypad(void);
/******************************************************************************
 * Functions
 ******************************************************************************/

/**
 * @fn		int InitializeSeesaw(void)
 * @brief	Initializes the Seesaw 4x4
 * @details 	Assumes I2C is already initialized

 * @return		Returns 0 if no errors.
 * @note
 */
int InitializeSeesaw(void)
{
    uint8_t readData[2];
    seesawData.address = NEO_TRELLIS_ADDR;
    seesawData.msgOut = &msgBaseGetHWID[0];
    seesawData.lenOut = sizeof(msgBaseGetHWID);
    seesawData.msgIn = &readData[0];
    seesawData.lenIn = 1;

    // Check if device is on the line - it should answer with its HW ID

    int error = I2cReadDataWait(&seesawData, 0, 100);

    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Error initializing Seesaw!/r/n");
    } else {
        if (readData[0] != SEESAW_HW_ID_CODE) {
            SerialConsoleWriteString("Error/r/n");
        } else {
            SerialConsoleWriteString("Found Seesaw!/r/n");
        }
    }

    // Tell the Seesaw which pins to use
    seesawData.msgOut = &msgNeopixelPin[0];
    seesawData.lenOut = sizeof(msgNeopixelPin);

    error = I2cWriteDataWait(&seesawData, 100);
    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Could not write Seesaw pin!/r/n");
    }

    // Set seesaw Neopixel speed
    seesawData.msgOut = &msgNeopixelSpeed[0];
    seesawData.lenOut = sizeof(msgNeopixelSpeed);

    error = I2cWriteDataWait(&seesawData, 100);
    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Could not set seesaw Neopixel speed!/r/n");
    }

    // Set seesaw Neopixel number of devices
    seesawData.msgOut = &msgNeopixelBufLength[0];
    seesawData.lenOut = sizeof(msgNeopixelBufLength);

    error = I2cWriteDataWait(&seesawData, 100);
    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Could not set seesaw Neopixel number of devices/r/n");
    }

    SeesawTurnOnLedTest();

    SeesawInitializeKeypad();
    return error;
}

/**
 * @fn		uint8_t SeesawGetKeypadCount(void)
 * @brief	Returns the number of key events currently on the Seesaw Keypad
 * @details 	Assumes Seesaw is already initialized

 * @return		Returns the number of events in the buffer. Use SeesawReadKeypad to read these events.
 * @note
*/
uint8_t SeesawGetKeypadCount(void)
{
    uint8_t count = 0;
    seesawData.address = NEO_TRELLIS_ADDR;
    seesawData.msgOut = &msgKeypadGetCount[0];
    seesawData.lenOut = sizeof(msgKeypadGetCount);
    seesawData.msgIn = &count;
    seesawData.lenIn = 1;

    int error = I2cReadDataWait(&seesawData, 0, 100);

    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Error reading Seesaw counts!/r/n");
    }
    return count;
}

/**
 * @fn		int32_t SeesawReadKeypad(uint8_t *buffer, uint8_t count)
 * @brief	Returns the number of requested events in the Seesaw FIFO buffer into the buffer variable
 * @param[out] buffer  Pointer to a buffer where the function will write the events in the seesaw buffer to.
 * @param[in]  count  Number of events to read from the Seesaw FIFO

 * @return		Returns zero if no I2C errors occurred. Other number in case of error
 * @note         Use SeesawGetKeypadCount to know how many events are in buffer.
*/
int32_t SeesawReadKeypad(uint8_t *buffer, uint8_t count)
{
    if (count == 0) return ERROR_NONE;
    uint8_t cmd[] = {SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_FIFO};
    seesawData.address = NEO_TRELLIS_ADDR;
    seesawData.msgOut = (const uint8_t*) &cmd[0];
    seesawData.lenOut = sizeof(cmd);
    seesawData.msgIn = buffer;
    seesawData.lenIn = count;

    int error = I2cReadDataWait(&seesawData, 0, 100);

    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Error reading Seesaw counts!/r/n");
    }
    return error;
}

/**
 int32_t SeesawActivateKey(uint8_t key, uint8_t edge, bool enable)
 * @brief	Activates a given key to react to a certain event. Will tell the Seesaw to add events to the FIFO buffer for that key/event pair.
 * @param[in] key  Key number (0 to 15)
 * @param[in] edge Event to listen to. SEESAW_KEYPAD_EDGE_RISING or SEESAW_KEYPAD_EDGE_FALLING
 * @param[in]  enable  Boolean. If true, add event to listener.

 * @return		Returns zero if no I2C errors occurred. Other number in case of error
 * @note
*/
int32_t SeesawActivateKey(uint8_t key, uint8_t edge, bool enable)
{
    union keyState ks;
    ks.bit.STATE = enable;
    ks.bit.ACTIVE = (1 << edge);
    uint8_t cmd[] = {SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_EVENT, key, ks.reg};

    seesawData.address = NEO_TRELLIS_ADDR;
    seesawData.msgOut = (const uint8_t *) &cmd[0];
    seesawData.lenOut = sizeof(cmd);
    seesawData.lenIn = 0;
    int32_t error = I2cWriteDataWait(&seesawData, 100);
    return error;
}

/**
 int32_t SeesawSetLed(uint8_t key, uint8_t red, uint8_t green, uint8_t blue)
 * @brief	Turns on a given LED with the given RGB colors.
 * @param[in] key  Key number (0 to 15)
 * @param[in] red Red color. 0 to 255.
 * @param[in] green Red color. 0 to 255.
 * @param[in] blue Red color. 0 to 255.

 * @return		Returns zero if no I2C errors occurred. Other number in case of error
 * @note         Note that the LEDs wont turn on until you send a "SeesawOrderLedUpdate" command.
         FOR ESE516 Board, please do not turn ALL the LEDs to maximum brightness (255,255,255)!
*/
int32_t SeesawSetLed(uint8_t key, uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t write_buffer1[7] = {SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF, 0, 0, green, red, blue};

    uint16_t offset = 3 * key;  // RGB LED
    write_buffer1[2] = (offset >> 8);
    write_buffer1[3] = (offset);

    seesawData.msgOut = (const uint8_t *)&write_buffer1[0];
    seesawData.lenOut = sizeof(write_buffer1);
    int error = I2cWriteDataWait(&seesawData, 100);
    return error;
}

/**
 int32_t SeesawOrderLedUpdate(void)
 * @brief	Orders the Seesaw driver to update the LEDs (turn on with new information given before).

 * @return		Returns zero if no I2C errors occurred. Other number in case of error
 * @note         Use "SeesawSetLed" to send LED Data. The data will not be use to update the display until this function has been called!

*/
int32_t SeesawOrderLedUpdate(void)
{
    uint8_t orderBuffer[2] = {SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SHOW};

    seesawData.msgOut = (const uint8_t *) &orderBuffer[0];
    seesawData.lenOut = sizeof(orderBuffer);
    int error = I2cWriteDataWait(&seesawData, 100);
    return error;
}

/*****************************************************************************************
 *  @brief     Activates a given key on the keypad
 *  @return     Returns any error code found when executing task.
 ****************************************************************************************/
static void SeesawInitializeKeypad(void)
{
    seesawData.address = NEO_TRELLIS_ADDR;
    seesawData.msgOut = &msgKeypadEnableInt[0];
    seesawData.lenOut = sizeof(msgKeypadEnableInt);
    seesawData.lenIn = 0;

    int32_t error = I2cWriteDataWait(&seesawData, 100);
    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Could not initialize Keypad!/r/n");
    }

    // Initialize all buttons to register an event for both press and release
    for (int i = 0; i < 16; i++) {
        error = SeesawActivateKey(NEO_TRELLIS_KEY(i), SEESAW_KEYPAD_EDGE_RISING, true);
        error |= SeesawActivateKey(NEO_TRELLIS_KEY(i), SEESAW_KEYPAD_EDGE_FALLING, true);
        if (ERROR_NONE != error) {
            SerialConsoleWriteString("Could not initialize Keypad!/r/n");
        }
    }
}

static void SeesawTurnOnLedTest(void)
{
    SeesawSetLed(15, 255, 255, 255);
    SeesawOrderLedUpdate();
    vTaskDelay(400);

    SeesawSetLed(15, 0, 0, 0);
    SeesawOrderLedUpdate();
}