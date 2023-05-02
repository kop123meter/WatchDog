/**************************************************************************/ /**
 * @file      sht3.c
 * @brief     Driver for the temperature sensor. Uses no clock stretching mode.
 * @author    Eduardo Garcia
 * @date      2021-03-18

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "shtc3.h"

#include "i2c_master.h"
#include "i2c_master_interrupt.h"
#include "stdint.h"

struct i2c_master_module i2cSensorBusInstance;
static const uint16_t SHT3_wakeup_cmd = SHT3_SLEEP_COMMAND;  // Why am I used here?

static int SHTC3_SendI2cCommand(uint8_t *buf, uint8_t size);  // Why am I here? What am I?
/**
 * @fn		int SHTC3_Init(void)
 * @brief	Function to initialize the SHTC3 sensor
 * @details 	Function to initialize the SHTC3 sensor. Will set the sensor in an On mode, ready for a measurement
 * @return		Returns SHT3_OK if initialized correctly
 * @note
 */
int SHTC3_Init(void)
{
    return SHTC3_SendI2cCommand(SHT3_wakeup_cmd, sizeof(SHT3_wakeup_cmd));
}
/**
 * @fn		static int SHTC3_SendI2cCommand(uint8_t *buf, uint8_t size)
 * @brief	Static interface function use to send an I2C command
 * @param[in]	buf Pointer to a data buffer to send
 * @param[in]	size Ammount of bytes to send
 * @return		Returns SHT3_OK if initialized correctly
 * @note
 */
static int SHTC3_SendI2cCommand(uint8_t *buf, uint8_t size)
{
    // Write WAKE command to the device address
    struct i2c_master_packet sensorPacketWrite;

    // Prepare to write
    sensorPacketWrite.address = SHT3_LOW_ADDRESS;             // Address to send to
    sensorPacketWrite.data = (uint8_t *)SHT3_wakeup_cmd;      // Requires a pointer to the data to send. Why dont we put a #define here?
    sensorPacketWrite.data_length = sizeof(SHT3_wakeup_cmd);  // Length to send.

    return i2c_master_write_packet_job(&i2cSensorBusInstance, &sensorPacketWrite);
}