#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
//*****************************************************************************
//
//  File........: LCD_Driver.h
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Description.: Defines and prototypes for LCD_Driver.c
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20020606 - 0.10 - File created                                  - RM
//  20021010 - 1.0  - Clean up                                      - JLL
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//  20070129          LCD_CONTRAST_LEVEL from Atmel's REV07-code    - mt
//
//*****************************************************************************


/************************************************************************/
// Definitions
/************************************************************************/
#define LCD_INITIAL_CONTRAST    0x0F
#define LCD_TIMER_SEED		    3
#define LCD_FLASH_SEED          10
#define LCD_REGISTER_COUNT      20
#define TEXTBUFFER_SIZE         25

// Codes stored in gTextBuffer (bit 7 = flashing, bits 0-6 = code)
#define LCD_CODE_END            0x00    // end of the text
#define LCD_CODE_BLANK          0x7F    // space or a character without a segment pattern
//                              1..54   index into LCD_character_table + 1 ('*'..'_')
#define LCD_CODE_FLASH          0x80

#define SCROLLMODE_ONCE         0x01
#define SCROLLMODE_LOOP         0x02
#define SCROLLMODE_WAVE         0x03

/************************************************************************/
//MACROS
/************************************************************************/
// DEVICE SPECIFIC!!! (ATmega169)
#define pLCDREG ((volatile uint8_t *)(0xEC))

// DEVICE SPECIFIC!!! (ATmega169) First LCD segment register
#define LCD_CONTRAST_LEVEL(level) LCDCCR=((LCDCCR&0xF0)|(0x0F & level))


/************************************************************************/
// Global variables
/************************************************************************/
// mt: volatiles added (based on jw patch)
extern volatile bool gLCD_Update_Required;
// Text as segment-table codes (see LCD_CharToCode), not as ASCII. Bit 7 marks a
// flashing character. +1: room for the terminator of a full-length text.
extern volatile uint8_t gTextBuffer[TEXTBUFFER_SIZE + 1];
extern volatile uint8_t gScrollMode;
extern volatile uint8_t gFlashTimer;
extern bool gColon;
extern volatile int8_t gScroll;
// State of the joystick auto-repeat. Packed to one byte so that the access
// from the LCD interrupt and from the main loop stays atomic on the AVR.
typedef enum __attribute__((packed))
{
    JOY_IDLE,       // button state changed, nothing pending
    JOY_PRESSED,    // button held, auto press armed
    JOY_AUTO        // auto press active (repeat counter running)
} JOYSTICK_AUTO_STATE;

extern volatile JOYSTICK_AUTO_STATE gAutoPressJoystick;
extern volatile uint8_t gLCD_Start_Scroll_Timer;

/************************************************************************/
// Global functions
/************************************************************************/
void LCD_Init (void);
uint8_t LCD_CharToCode(char c);
void LCD_AllSegments(bool show);

#endif /* LCD_DRIVER_H_ */
