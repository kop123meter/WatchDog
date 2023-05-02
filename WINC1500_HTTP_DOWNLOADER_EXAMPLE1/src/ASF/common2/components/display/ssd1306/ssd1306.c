/**
 * \file
 *
 * \brief SSD1306 OLED display controller driver.
 *
 * Copyright (c) 2012-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include "ssd1306.h"
#include "I2cDriver/I2cDriver.h"

struct spi_module ssd1306_master;
struct spi_slave_inst ssd1306_slave;


I2C_Data i2cOled; ///<Global variable to use for I2C communications with the Seesaw Device
/**
 * \internal
 * \brief Initialize the hardware interface
 *
 * Depending on what interface used for interfacing the OLED controller this
 * function will initialize the necessary hardware.
 */
static void ssd1306_interface_init(void)
{

}

/**
 * \brief Initialize the OLED controller
 *
 * Call this function to initialize the hardware interface and the OLED
 * controller. When initialization is done the display is turned on and ready
 * to receive data.
 */
void ssd1306_init(void)
{


	// Initialize the interface
	ssd1306_interface_init();

	// Do a hard reset of the OLED display controller
	ssd1306_hard_reset();

	// Set the reset pin to the default state
	//port_pin_set_output_level(SSD1306_RES_PIN, true);
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_OFF); //0xAE
	
	// Set Display Clock Divide Ratio / Oscillator Frequency (Default => 0x80)
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO); //0XD5
	ssd1306_write_command(0x80);
	
	
	// 1/32 Duty (0x0F~0x3F)
	ssd1306_write_command(SSD1306_CMD_SET_MULTIPLEX_RATIO); //0XA8
	ssd1306_write_command(0x2F); //2F
	
	// Shift Mapping RAM Counter (0x00~0x3F)
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_OFFSET); //D3
	ssd1306_write_command(0x00);
	
	// Set Mapping RAM Display Start Line (0x00~0x3F)
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_START_LINE(0x00));	
	
	// Enable charge pump regulator
	ssd1306_write_command(SSD1306_CMD_SET_CHARGE_PUMP_SETTING); //8D
	ssd1306_write_command(0x14);

	ssd1306_write_command(SSD1306_CMD_SET_NORMAL_DISPLAY); //A6
	ssd1306_write_command(SSD1306_CMD_ENTIRE_DISPLAY_AND_GDDRAM_ON); //0xA4
		
	ssd1306_write_command(SSD1306_CMD_SET_SEGMENT_RE_MAP_COL0_SEG0| 0x1); //0xA0
	ssd1306_write_command(SSD1306_CMD_SET_COM_OUTPUT_SCAN_DOWN); //0xC8
	
	// Set COM Pins hardware configuration
	ssd1306_write_command(SSD1306_CMD_SET_COM_PINS); //0xDA
	ssd1306_write_command(0x12);
		
	ssd1306_write_command(SSD1306_CMD_SET_CONTRAST_CONTROL_FOR_BANK0); //0x81
	ssd1306_write_command(0x8F);
	
	ssd1306_write_command(SSD1306_CMD_SET_PRE_CHARGE_PERIOD); //0xD9
	ssd1306_write_command(0xF1);	
	
	ssd1306_write_command(SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL); //DB
	ssd1306_write_command(0x40);

	
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_ON);

}

/**
 * \brief Writes a command to the display controller
 *
 * This functions pull pin D/C# low before writing to the controller. Different
 * data write function is called based on the selected interface.
 *
 * \param command the command to write
 */


void ssd1306_write_command(uint8_t command)
{
	//Send 0 command byte
	uint8_t data[2];
	data[0] = I2C_COMMAND_SSD1306;
	data[1] = command;
	i2cOled.address = SSD1306_I2C_ADDRESS;
	i2cOled.msgOut = (const uint8_t*) &data[0];
	i2cOled.lenOut = sizeof(data);
	i2cOled.msgIn = 0;
	i2cOled.lenIn = 0;
	
	I2cWriteDataWait(&i2cOled, 100);
}

/**
 * \brief Write data to the display controller
 *
 * This functions sets the pin D/C# before writing to the controller. Different
 * data write function is called based on the selected interface.
 *
 * \param data the data to write
 */
void ssd1306_write_data(uint8_t data)
{
	uint8_t msg[2];
	msg[0] = I2C_DATA_SSD1306;
	msg[1] = data;
	i2cOled.address = SSD1306_I2C_ADDRESS;
	i2cOled.msgOut = (const uint8_t*) &msg[0];
	i2cOled.lenOut = sizeof(msg);
	i2cOled.msgIn = 0;
	i2cOled.lenIn = 0;
	
	I2cWriteDataWait(&i2cOled, 100);
}
