/**
* @file      UiHandlerThread.c
* @brief     File that contains the task code and supporting code for the UI
Thread for ESE516 Spring (Online) Edition
* @author    You! :)
* @date      2020-04-09

******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "UiHandlerThread/UiHandlerThread.h"

#include <errno.h>

#include "DistanceDriver/DistanceSensor.h"
#include "IMU/lsm6dso_reg.h"
#include "SeesawDriver/Seesaw.h"
#include "SerialConsole.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "asf.h"
#include "gfx_mono.h"
#include "main.h"
#include "TFTdriver/tft.h"
#include "newservo/newservo.h"
#include "bme680/bme680.h"
#include "speaker/speaker.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define BUTTON_PRESSES_MAX 16  ///< Number of maximum button presses to analize in one go

/******************************************************************************
 * Variables
 ******************************************************************************/
uiStateMachine_state uiState;         ///< Holds the current state of the UI
struct GameDataPacket gamePacketIn;   ///< Holds the game packet to show
struct GameDataPacket gamePacketOut;  ///< Holds the game packet to send back
volatile uint8_t red = 0;             ///< Holds the color of the red LEDs. Can be set by MQTT
volatile uint8_t green = 100;         ///< Holds the color of the green LEDs. Can be set by MQTT
volatile uint8_t blue = 50;           ///< Holds the color of the blue LEDs. Can be set by MQTT

uint8_t pressedKeys = 0;              ///< Variable to count how many presses the player has done
uint8_t keysToPress = 0;              ///< Variable that holds the number of new keypresses the user should do
bool playIsDone = false;              ///< Boolean flag to indicate if the player has finished moving.
                                      ///< Useful for COntrol to determine when to send back a play.
uint8_t buttons[BUTTON_PRESSES_MAX];  ///< Array to hold button presses
/******************************************************************************
 * Forward Declarations
 ******************************************************************************/

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/******************************************************************************
 * Task Function
 ******************************************************************************/

/**
 * @fn		void vUiHandlerTask( void *pvParameters )
 * @brief	STUDENT TO FILL THIS
 * @details 	student to fill this
 * @param[in]	Parameters passed when task is initialized. In this case we can ignore them!
 * @return		Should not return! This is a task defining function.
 * @note
 */
void vUiHandlerTask(void *pvParameters)
{
    // Do initialization code here
    SerialConsoleWriteString("UI Task Started!");
    uiState = UI_STATE_IGNORE_PRESSES;  // Initial state
   
   configure_port_speaker_pins();
	configure_port_servo_pins();
	init_bme680();
	read_sensor_data();
		//read sensor data

		
		
		
		
	configure_port_pins();
		//servopulse(200);
		//configure_tcc();
		//setPulse(2.4);
		//lock();
		
	LCD_init();
	LCD_clearScreen(WHITE);
	 
	 
    // Graphics Test - Students to uncomment to test out the OLED driver if you are using it! 
	/*
    gfx_mono_init();
    gfx_mono_draw_line(0, 0, 64, 48, GFX_PIXEL_SET);
    gfx_mono_draw_filled_circle(54, 24, 10, GFX_PIXEL_SET, GFX_WHOLE);
	gfx_mono_draw_string("ESE516",0,0, &sysfont);
	*/

    // Here we start the loop for the UI State Machine
	
	
    while (1) {
	

	  // warning_speaker();
        switch (uiState) {
            case (UI_STATE_IGNORE_PRESSES): {
           
                break;
            }

            case (UI_STATE_SHOW_MOVES): {
                // Set initial state variable that will be used on the
                // UI_STATE_Handle_Buttons and need to be initialized once
                pressedKeys = 0;  // Set number of keys pressed by player to 0.
                keysToPress = 1;  // Set as an example to 1. STUDENTS should change this
                                  // to the number of key presses needed.
                memset(gamePacketOut.game, 0xff,
                       sizeof(gamePacketOut.game));  // Erase gamePacketOut to an initial state
                playIsDone = false;                  // Set play to false
                uint8_t presses = SeesawGetKeypadCount();
                if (presses >= BUTTON_PRESSES_MAX) presses = BUTTON_PRESSES_MAX;
                if (presses != 0)
                    SeesawReadKeypad(buttons,
                                     presses);  // Empty Seesaw buffer just in case
                                                // it has latent presses on it!
                memset(buttons, 0, BUTTON_PRESSES_MAX);
                // STUDENTS: Make this function show the moves of the gamePacketIn.
                // You can use a static delay to show each move but a quicker delay as
                // the message gets longer might be more fun! After you finish showing
                // the move should go to state UI_STATE_HANDLE_BUTTONS

                // In the beginner example we turn LED0 and LED15 will turn on for 500
                // ms then we go to UI_STATE_HANDLE_BUTTONS
                SeesawSetLed(0, red, green, blue);  // Turn button 1 on
                SeesawOrderLedUpdate();
                vTaskDelay(10);
                SeesawSetLed(0, 0, 0, 0);            // Turn button 0 off
                SeesawSetLed(15, red, green, blue);  // Turn button 15 on
                SeesawOrderLedUpdate();
                vTaskDelay(10);
                SeesawSetLed(15, 0, 0, 0);  // Turn button 15 off
                SeesawOrderLedUpdate();
                vTaskDelay(10);
                uiState = UI_STATE_HANDLE_BUTTONS;

                break;
            }

            case (UI_STATE_HANDLE_BUTTONS): {
                // This state should accept (gamePacketIn length + 1) moves from the
                // player (capped to maximum 19 + new move) The moves by the player
                // should be stored on "gamePacketOut". The keypresses that should count
                // are when the player RELEASES the button.

                // In this example, we return after only one button press!

                uint8_t numPresses = SeesawGetKeypadCount();
                memset(buttons, 0, BUTTON_PRESSES_MAX);

                if (numPresses >= BUTTON_PRESSES_MAX) {
                    numPresses = BUTTON_PRESSES_MAX;
                }
                if (numPresses != 0 && ERROR_NONE == SeesawReadKeypad(buttons, numPresses)) {
                    // Process Buttons
                    for (int iter = 0; iter < numPresses; iter++) {
                        uint8_t keynum = NEO_TRELLIS_SEESAW_KEY((buttons[iter] & 0xFD) >> 2);
                        uint8_t actionButton = buttons[iter] & 0x03;
                        if (actionButton == 0x03) {
                            SeesawSetLed(keynum, red, green, blue);
                        } else {
                            SeesawSetLed(keynum, 0, 0, 0);
                            // Button released! Count this into the buttons pressed by user.
                            gamePacketOut.game[pressedKeys] = keynum;
                            pressedKeys++;
                        }
                    }
                    SeesawOrderLedUpdate();
                }

                // Check if we are done!
                if (pressedKeys >= keysToPress || pressedKeys >= GAME_SIZE) {
                    // Tell control gamePacketOut is ready to be send out AND go back to
                    // UI_STATE_IGNORE_PRESSES
                    playIsDone = true;
                    uiState = UI_STATE_IGNORE_PRESSES;
                }

                break;
            }

            default:  // In case of unforseen error, it is always good to sent state
                      // machine to an initial state.
                uiState = UI_STATE_HANDLE_BUTTONS;
                break;
        }

        // After execution, you can put a thread to sleep for some time.
        vTaskDelay(50);
    }
}

/******************************************************************************
 * Functions
 ******************************************************************************/
void UiOrderShowMoves(struct GameDataPacket *packetIn)
{
    memcpy(&gamePacketIn, packetIn, sizeof(gamePacketIn));
    uiState = UI_STATE_SHOW_MOVES;
    playIsDone = false;  // Set play to false
}

bool UiPlayIsDone(void)
{
    return playIsDone;
}

struct GameDataPacket *UiGetGamePacketOut(void)
{
    return &gamePacketOut;
}

/**
 int UIChangeColors(uint8_t r, uint8_t g, uint8_t b);
 * @brief	Changes the LED colors
 * @param [in]
 * @return
 * @note

*/
void UIChangeColors(uint8_t r, uint8_t g, uint8_t b)
{
    red = r;
    green = g;
    blue = b;
}
