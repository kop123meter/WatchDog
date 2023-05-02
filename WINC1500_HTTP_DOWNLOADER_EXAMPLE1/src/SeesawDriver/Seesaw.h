/**************************************************************************/ /**
 * @file      Seesaw.h
 * @brief     Driver for the seesaw 4x4 LED button from Adafruit. See https://cdn-learn.adafruit.com/downloads/pdf/adafruit-seesaw-atsamd09-breakout.pdf
  and https://github.com/adafruit/Adafruit_Seesaw

 * @author    Eduardo Garcia
 * @date      2020-01-01

 ******************************************************************************/

#ifndef _NEO_TRELLIS_H
#define _NEO_TRELLIS_H
#include "I2cDriver/I2cDriver.h"

#define NEO_TRELLIS_ADDR 0x2E

#define NEO_TRELLIS_NEOPIX_PIN 3

#define NEO_TRELLIS_NUM_ROWS 4
#define NEO_TRELLIS_NUM_COLS 4
#define NEO_TRELLIS_NUM_KEYS (NEO_TRELLIS_NUM_ROWS * NEO_TRELLIS_NUM_COLS)

#define NEO_TRELLIS_MAX_CALLBACKS 32

#define NEO_TRELLIS_KEY(x) (((x) / 4) * 8 + ((x) % 4))         ///< Converts a number from Key 0 - Key 15 into the number used by the Neotrellis (0-3, 8-11, 16-19, 24-27)
#define NEO_TRELLIS_SEESAW_KEY(x) (((x) / 8) * 4 + ((x) % 8))  ///< Converts a Neotrellis Key (0-3, 8-11, 16-19, 24-27) into a key number (0 to 15)

#define NEO_TRELLIS_X(k) ((k) % 4)
#define NEO_TRELLIS_Y(k) ((k) / 4)

#define NEO_TRELLIS_XY(x, y) ((y)*NEO_TRELLIS_NUM_ROWS + (x))

// The order of primary colors in the NeoPixel data stream can vary
// among device types, manufacturers and even different revisions of
// the same item.  The third parameter to the seesaw_NeoPixel
// constructor encodes the per-pixel byte offsets of the red, green
// and blue primaries (plus white, if present) in the data stream --
// the following #defines provide an easier-to-use named version for
// each permutation.  e.g. NEO_GRB indicates a NeoPixel-compatible
// device expecting three bytes per pixel, with the first byte
// containing the green value, second containing red and third
// containing blue.  The in-memory representation of a chain of
// NeoPixels is the same as the data-stream order; no re-ordering of
// bytes is required when issuing data to the chain.

// Bits 5,4 of this value are the offset (0-3) from the first byte of
// a pixel to the location of the red color byte.  Bits 3,2 are the
// green offset and 1,0 are the blue offset.  If it is an RGBW-type
// device (supporting a white primary in addition to R,G,B), bits 7,6
// are the offset to the white byte...otherwise, bits 7,6 are set to
// the same value as 5,4 (red) to indicate an RGB (not RGBW) device.
// i.e. binary representation:
// 0bWWRRGGBB for RGBW devices
// 0bRRRRGGBB for RGB

// RGB NeoPixel permutations; white and red offsets are always same
// Offset:         W          R          G          B
#define NEO_RGB ((0 << 6) | (0 << 4) | (1 << 2) | (2))
#define NEO_RBG ((0 << 6) | (0 << 4) | (2 << 2) | (1))
#define NEO_GRB ((1 << 6) | (1 << 4) | (0 << 2) | (2))
#define NEO_GBR ((2 << 6) | (2 << 4) | (0 << 2) | (1))
#define NEO_BRG ((1 << 6) | (1 << 4) | (2 << 2) | (0))
#define NEO_BGR ((2 << 6) | (2 << 4) | (1 << 2) | (0))

// RGBW NeoPixel permutations; all 4 offsets are distinct
// Offset:         W          R          G          B
#define NEO_WRGB ((0 << 6) | (1 << 4) | (2 << 2) | (3))
#define NEO_WRBG ((0 << 6) | (1 << 4) | (3 << 2) | (2))
#define NEO_WGRB ((0 << 6) | (2 << 4) | (1 << 2) | (3))
#define NEO_WGBR ((0 << 6) | (3 << 4) | (1 << 2) | (2))
#define NEO_WBRG ((0 << 6) | (2 << 4) | (3 << 2) | (1))
#define NEO_WBGR ((0 << 6) | (3 << 4) | (2 << 2) | (1))

#define NEO_RWGB ((1 << 6) | (0 << 4) | (2 << 2) | (3))
#define NEO_RWBG ((1 << 6) | (0 << 4) | (3 << 2) | (2))
#define NEO_RGWB ((2 << 6) | (0 << 4) | (1 << 2) | (3))
#define NEO_RGBW ((3 << 6) | (0 << 4) | (1 << 2) | (2))
#define NEO_RBWG ((2 << 6) | (0 << 4) | (3 << 2) | (1))
#define NEO_RBGW ((3 << 6) | (0 << 4) | (2 << 2) | (1))

#define NEO_GWRB ((1 << 6) | (2 << 4) | (0 << 2) | (3))
#define NEO_GWBR ((1 << 6) | (3 << 4) | (0 << 2) | (2))
#define NEO_GRWB ((2 << 6) | (1 << 4) | (0 << 2) | (3))
#define NEO_GRBW ((3 << 6) | (1 << 4) | (0 << 2) | (2))
#define NEO_GBWR ((2 << 6) | (3 << 4) | (0 << 2) | (1))
#define NEO_GBRW ((3 << 6) | (2 << 4) | (0 << 2) | (1))

#define NEO_BWRG ((1 << 6) | (2 << 4) | (3 << 2) | (0))
#define NEO_BWGR ((1 << 6) | (3 << 4) | (2 << 2) | (0))
#define NEO_BRWG ((2 << 6) | (1 << 4) | (3 << 2) | (0))
#define NEO_BRGW ((3 << 6) | (1 << 4) | (2 << 2) | (0))
#define NEO_BGWR ((2 << 6) | (3 << 4) | (1 << 2) | (0))
#define NEO_BGRW ((3 << 6) | (2 << 4) | (1 << 2) | (0))

// If 400 KHz support is enabled, the third parameter to the constructor
// requires a 16-bit value (in order to select 400 vs 800 KHz speed).
// If only 800 KHz is enabled (as is default on ATtiny), an 8-bit value
// is sufficient to encode pixel color order, saving some space.

#define NEO_KHZ800 0x0000  // 800 KHz datastream
#define NEO_KHZ400 0x0100  // 400 KHz datastream

#define SEESAW_HW_ID_CODE 0x55  ///< seesaw HW ID code

enum {
    SEESAW_STATUS_BASE = 0x00,
    SEESAW_GPIO_BASE = 0x01,
    SEESAW_SERCOM0_BASE = 0x02,

    SEESAW_TIMER_BASE = 0x08,
    SEESAW_ADC_BASE = 0x09,
    SEESAW_DAC_BASE = 0x0A,
    SEESAW_INTERRUPT_BASE = 0x0B,
    SEESAW_DAP_BASE = 0x0C,
    SEESAW_EEPROM_BASE = 0x0D,
    SEESAW_NEOPIXEL_BASE = 0x0E,
    SEESAW_TOUCH_BASE = 0x0F,
    SEESAW_KEYPAD_BASE = 0x10,
    SEESAW_ENCODER_BASE = 0x11,
};

/** GPIO module function addres registers
 */
enum {
    SEESAW_GPIO_DIRSET_BULK = 0x02,
    SEESAW_GPIO_DIRCLR_BULK = 0x03,
    SEESAW_GPIO_BULK = 0x04,
    SEESAW_GPIO_BULK_SET = 0x05,
    SEESAW_GPIO_BULK_CLR = 0x06,
    SEESAW_GPIO_BULK_TOGGLE = 0x07,
    SEESAW_GPIO_INTENSET = 0x08,
    SEESAW_GPIO_INTENCLR = 0x09,
    SEESAW_GPIO_INTFLAG = 0x0A,
    SEESAW_GPIO_PULLENSET = 0x0B,
    SEESAW_GPIO_PULLENCLR = 0x0C,
};

/** status module function addres registers
 */
enum {
    SEESAW_STATUS_HW_ID = 0x01,
    SEESAW_STATUS_VERSION = 0x02,
    SEESAW_STATUS_OPTIONS = 0x03,
    SEESAW_STATUS_TEMP = 0x04,
    SEESAW_STATUS_SWRST = 0x7F,
};

/** timer module function addres registers
 */
enum {
    SEESAW_TIMER_STATUS = 0x00,
    SEESAW_TIMER_PWM = 0x01,
    SEESAW_TIMER_FREQ = 0x02,
};

/** ADC module function addres registers
 */
enum {
    SEESAW_ADC_STATUS = 0x00,
    SEESAW_ADC_INTEN = 0x02,
    SEESAW_ADC_INTENCLR = 0x03,
    SEESAW_ADC_WINMODE = 0x04,
    SEESAW_ADC_WINTHRESH = 0x05,
    SEESAW_ADC_CHANNEL_OFFSET = 0x07,
};

/** Sercom module function addres registers
 */
enum {
    SEESAW_SERCOM_STATUS = 0x00,
    SEESAW_SERCOM_INTEN = 0x02,
    SEESAW_SERCOM_INTENCLR = 0x03,
    SEESAW_SERCOM_BAUD = 0x04,
    SEESAW_SERCOM_DATA = 0x05,
};

/** neopixel module function addres registers
 */
enum {
    SEESAW_NEOPIXEL_STATUS = 0x00,
    SEESAW_NEOPIXEL_PIN = 0x01,
    SEESAW_NEOPIXEL_SPEED = 0x02,
    SEESAW_NEOPIXEL_BUF_LENGTH = 0x03,
    SEESAW_NEOPIXEL_BUF = 0x04,
    SEESAW_NEOPIXEL_SHOW = 0x05,
};

/** touch module function addres registers
 */
enum {
    SEESAW_TOUCH_CHANNEL_OFFSET = 0x10,
};

/** keypad module function addres registers
 */
enum {
    SEESAW_KEYPAD_STATUS = 0x00,
    SEESAW_KEYPAD_EVENT = 0x01,
    SEESAW_KEYPAD_INTENSET = 0x02,
    SEESAW_KEYPAD_INTENCLR = 0x03,
    SEESAW_KEYPAD_COUNT = 0x04,
    SEESAW_KEYPAD_FIFO = 0x10,
};

/** keypad module edge definitions
 */
enum {
    SEESAW_KEYPAD_EDGE_HIGH = 0,
    SEESAW_KEYPAD_EDGE_LOW,
    SEESAW_KEYPAD_EDGE_FALLING,
    SEESAW_KEYPAD_EDGE_RISING,
};

/** encoder module edge definitions
 */
enum {
    SEESAW_ENCODER_STATUS = 0x00,
    SEESAW_ENCODER_INTENSET = 0x02,
    SEESAW_ENCODER_INTENCLR = 0x03,
    SEESAW_ENCODER_POSITION = 0x04,
    SEESAW_ENCODER_DELTA = 0x05,
};

#define ADC_INPUT_0_PIN 2  ///< default ADC input pin
#define ADC_INPUT_1_PIN 3  ///< default ADC input pin
#define ADC_INPUT_2_PIN 4  ///< default ADC input pin
#define ADC_INPUT_3_PIN 5  ///< default ADC input pin

#define PWM_0_PIN 4  ///< default PWM output pin
#define PWM_1_PIN 5  ///< default PWM output pin
#define PWM_2_PIN 6  ///< default PWM output pin
#define PWM_3_PIN 7  ///< default PWM output pin

#ifndef INPUT_PULLDOWN
#define INPUT_PULLDOWN \
    0x03  ///< for compatibility with platforms that do not already define
          ///< INPUT_PULLDOWN
#endif

/** raw key event stucture for keypad module */
union keyEventRaw {
    struct {
        uint8_t EDGE : 2;  ///< the edge that was triggered
        uint8_t NUM : 6;   ///< the event number
    } bit;                 ///< bitfield format
    uint8_t reg;           ///< register format
};

/** extended key event stucture for keypad module */
union keyEvent {
    struct {
        uint8_t EDGE : 2;   ///< the edge that was triggered
        uint16_t NUM : 14;  ///< the event number
    } bit;                  ///< bitfield format
    uint16_t reg;           ///< register format
};

/** key state struct that will be written to seesaw chip keypad module */
union keyState {
    struct {
        uint8_t STATE : 1;   ///< the current state of the key
        uint8_t ACTIVE : 4;  ///< the registered events for that key
    } bit;                   ///< bitfield format
    uint8_t reg;             ///< register format
};

int InitializeSeesaw(void);
uint8_t SeesawGetKeypadCount(void);
int32_t SeesawReadKeypad(uint8_t *buffer, uint8_t count);
int32_t SeesawSetLed(uint8_t key, uint8_t red, uint8_t green, uint8_t blue);
int32_t SeesawOrderLedUpdate(void);
int32_t SeesawActivateKey(uint8_t key, uint8_t edge, bool enable);
#endif