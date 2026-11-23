//*****************************************************************************
//
//  File........: LCD_driver.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: avr-gcc 7.3.0; avr-libc 2.0.0
//
//  Description.: Functions used to control the AVR Butterfly LCD
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - Written for STK502                            - JLL
//  20030116 - 2.0  - Code adapted to AVR Butterfly                 - KS
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//  20070122          "updated 2006-10-10" included (from REV07)    - from Atmel
//  20070129          SIGNAL->ISR, gLCD_Start_Scroll_Timer volatile - mt
//  202611xx          see https://github.com/thomas-engineering/bf_gcc
//
//*****************************************************************************


// Include files.
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "main.h"
// mt - for gButtonTimeout
#include "button.h"
#include "LCD_driver.h"

// Variable from "button.c" to prevent button-bouncing

volatile JOYSTICK_AUTO_STATE gAutoPressJoystick = JOY_IDLE;

// Used to indicate when the LCD interrupt handler should update the LCD
// mt jw char gLCD_Update_Required = false;
volatile bool gLCD_Update_Required = false;

// Buffer that contains the text to be displayed as segment-table codes
// (LCD_CODE_*, filled by LCD_CharToCode). Bit 7 indicates that this character
// is flashing. +1: terminator of a full-length text.
volatile uint8_t gTextBuffer[TEXTBUFFER_SIZE + 1];

// Only six letters can be shown on the LCD.
// With the gScroll and gScrollMode variables,
// one can select which part of the buffer to show
volatile int8_t gScroll;
volatile uint8_t gScrollMode;

////Start-up delay before scrolling a string over the LCD
volatile uint8_t gLCD_Start_Scroll_Timer = 0;

// The gFlashTimer is used to determine the on/off
// timing of flashing characters
volatile uint8_t gFlashTimer = 0;

// Turns on/off the colons on the LCD
bool gColon = false;



// Two ideas in this file come from Dean Camera's ("abcminiuser") rewrite of
// the Butterfly LCD driver (ButtLoad, 2007, https://github.com/abcminiuser/buttload,
// GPLv2). They are implemented here independently, no code was copied:
//  1. Characters are translated to segment-table codes when the text is
//     stored (LCD_CharToCode), so the LCD interrupt only does the table lookup.
//  2. The LCD interrupt writes the segments directly to the LCD data
//     registers; there is no RAM copy (LCD_Data) of them any more.

// Look-up table used when converting ASCII to
// LCD display data (segment control)
const __flash uint16_t LCD_character_table[] =
{
    0x0A51,     // '*' (?)
    0x2A80,     // '+'
    0x0000,     // ',' (Not defined)
    0x0A00,     // '-'
    0x0A51,     // '.' Degree sign
    0x0000,     // '/' (Not defined)
    0x5559,     // '0'
    0x0118,     // '1'
    0x1e11,     // '2
    0x1b11,     // '3
    0x0b50,     // '4
    0x1b41,     // '5
    0x1f41,     // '6
    0x0111,     // '7
    0x1f51,     // '8
    0x1b51,     // '9'
    0x0000,     // ':' (Not defined)
    0x0000,     // ';' (Not defined)
    0x0000,     // '<' (Not defined)
    0x0000,     // '=' (Not defined)
    0x0000,     // '>' (Not defined)
    0x0000,     // '?' (Not defined)
    0x0000,     // '@' (Not defined)
    0x0f51,     // 'A' (+ 'a')
    0x3991,     // 'B' (+ 'b')
    0x1441,     // 'C' (+ 'c')
    0x3191,     // 'D' (+ 'd')
    0x1e41,     // 'E' (+ 'e')
    0x0e41,     // 'F' (+ 'f')
    0x1d41,     // 'G' (+ 'g')
    0x0f50,     // 'H' (+ 'h')
    0x2080,     // 'I' (+ 'i')
    0x1510,     // 'J' (+ 'j')
    0x8648,     // 'K' (+ 'k')
    0x1440,     // 'L' (+ 'l')
    0x0578,     // 'M' (+ 'm')
    0x8570,     // 'N' (+ 'n')
    0x1551,     // 'O' (+ 'o')
    0x0e51,     // 'P' (+ 'p')
    0x9551,     // 'Q' (+ 'q')
    0x8e51,     // 'R' (+ 'r')
    0x9021,     // 'S' (+ 's')
    0x2081,     // 'T' (+ 't')
    0x1550,     // 'U' (+ 'u')
    0x4448,     // 'V' (+ 'v')
    0xc550,     // 'W' (+ 'w')
    0xc028,     // 'X' (+ 'x')
    0x2028,     // 'Y' (+ 'y')
    0x5009,     // 'Z' (+ 'z')
    0x0000,     // '[' (Not defined)
    0x0000,     // '\' (Not defined)
    0x0000,     // ']' (Not defined)
    0x0000,     // '^' (Not defined)
    0x0000      // '_'
};


/*****************************************************************************
*
*   Function name : LCD_Init
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Clear the LCD data registers.
*                   Set up the LCD (timing, contrast, etc.)
*
*****************************************************************************/
void LCD_Init(void)
{
    LCD_AllSegments(false);                     // Clear the LCD registers.

    LCD_CONTRAST_LEVEL(LCD_INITIAL_CONTRAST);    //Set the LCD contrast level

    // Select asynchronous clock source, enable all COM pins and enable all
    // segment pins.
    LCDCRB = (1<<LCDCS) | (3<<LCDMUX0) | (7<<LCDPM0);

    // Set LCD prescaler to give a framerate of 32,0 Hz
    LCDFRR = (0<<LCDPS0) | (7<<LCDCD0);    

    LCDCRA = (1<<LCDEN) | (1<<LCDAB);           // Enable LCD and set low power waveform

    //Enable LCD start of frame interrupt
    LCDCRA |= (1<<LCDIE);

    //updated 2006-10-10, setting LCD drive time to 1150us in FW rev 07, 
    //instead of previous 300us in FW rev 06. Due to some variations on the LCD
    //glass provided to the AVR Butterfly production.
    LCDCCR |= (1<<LCDDC2) | (1<<LCDDC1) | (1<<LCDDC0);

    gLCD_Update_Required = false;


}


/*****************************************************************************
*
*   Function name : LCD_CharToCode(char c)
*
*   Returns :       The code stored in gTextBuffer for this character
*
*   Parameters :    c: ASCII character; bit 7 set = flashing character
*
*   Purpose :       Translates a character once, when the text is stored, so
*                   that the LCD interrupt does not have to (idea from
*                   Dean Camera's driver, see the top of this file).
*                   '*'..'_' and 'a'..'z' (as upper case) get table index + 1,
*                   '\0' ends the text, everything else is a blank.
*
*****************************************************************************/
uint8_t LCD_CharToCode(char c)
{
    uint8_t flash = (uint8_t)c & LCD_CODE_FLASH;

    c &= 0x7F;

    if (c == '\0') {
        return LCD_CODE_END;
    }

    if (c >= 'a' && c <= 'z') { // Convert to upper case
        c &= ~0x20;
    }

    if (c >= '*' && c <= '_') {
        return (uint8_t)(c - '*' + 1) | flash;
    }

    return LCD_CODE_BLANK | flash;
}


/*****************************************************************************
*
*   Function name : LCD_WriteCode(uint8_t code, uint8_t digit)
*
*   Returns :       None
*
*   Parameters :    code: LCD_CODE_* / table index + 1 (bit 7 is ignored)
*                   digit: In which digit (0-5) the symbol should be displayed
*                   Note: Digit 0 is the first used digit on the LCD,
*                   i.e LCD digit 2
*
*   Purpose :       Writes the segments of one digit directly to the LCD data
*                   registers (no RAM copy, idea from Dean Camera's driver).
*                   Called from the LCD start of frame interrupt.
*
*****************************************************************************/
static inline void LCD_WriteCode(uint8_t code, uint8_t digit)
{
    uint16_t seg = 0x0000;                  // Holds the segment pattern
    uint8_t mask, nibble;
    volatile uint8_t *ptr;
    uint8_t i;

    code &= 0x7F;
    if (code != LCD_CODE_END && code != LCD_CODE_BLANK) {
        seg = LCD_character_table[code - 1];
    }

    // Adjust mask according to LCD segment mapping
    if (digit & 0x01) {
        mask = 0x0F;                // Digit 1, 3, 5
    }
    else {
        mask = 0xF0;                // Digit 0, 2, 4
    }

    ptr = pLCDREG + (digit >> 1);   // digit = {0,0,1,1,2,2}

    for (i = 0; i < 4; i++)
    {
        nibble = seg & 0x000F;
        seg >>= 4;
        if (digit & 0x01) {
            nibble <<= 4;
        }
        *ptr = (*ptr & mask) | nibble;
        ptr += 5;
    }
}



/*****************************************************************************
*
*   Function name : LCD_AllSegments(uint8_t input)
*
*   Returns :       None
*
*   Parameters :    show -  [true;false]
*
*   Purpose :       shows or hide all all LCD segments on the LCD
*
*****************************************************************************/
void LCD_AllSegments(bool show)
{
    uint8_t i;
    uint8_t value = show ? 0xFF : 0x00;

    // Set/clear all bits in all LCD registers
    for (i=0; i < LCD_REGISTER_COUNT; i++) {
        *(pLCDREG + i) = value;
    }
}


/*****************************************************************************
*
*   LCD Interrupt Routine
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose: Latch the LCD_displayData and Set LCD_status.updateComplete
*
*****************************************************************************/

ISR(LCD_vect)
{
    static uint8_t LCD_timer = LCD_TIMER_SEED;
    uint8_t c;                      // segment code, see LCD_CharToCode()
    uint8_t c_flash;
    uint8_t flash;

    uint8_t EOL;
    uint8_t i;

    static uint8_t timeout_count;
    static uint8_t auto_joystick_count;

    c_flash=0; // mt
    
/**************** Button timeout for the button.c, START ****************/
    if(!gButtonTimeout) {
        timeout_count++;
        
        if(timeout_count > 3) {
            gButtonTimeout = true;
            timeout_count = 0;
        }
    }

/**************** Button timeout for the button.c, END ******************/

/**************** Auto press joystick for the main.c, START *************/

    if(gAutoPressJoystick == JOY_AUTO) {
        auto_joystick_count++;
        
        if(auto_joystick_count > 16) {
            gAutoPressJoystick = JOY_PRESSED;
            auto_joystick_count = 15;
        }
    }
    else {
        auto_joystick_count = 0;
    }


/**************** Auto press joystick for the main.c, END ***************/    

    LCD_timer--;                    // Decreased every LCD frame

    if (gScrollMode) {
        // If we are in scroll mode, and the timer has expired,
        // we will update the LCD
        if (LCD_timer == 0) {
            if (gLCD_Start_Scroll_Timer == 0) {
                gLCD_Update_Required = true;
            }
            else {
                gLCD_Start_Scroll_Timer--;
            }
        }
    }
    else {   // if not scrolling,
        // disble LCD start of frame interrupt
        // cbi(LCDCRA, LCDIE);   //DEBUG
        gScroll = 0;
    }


    EOL = 0;
    if (gLCD_Update_Required) {
        // Duty cycle of flashing characters
        if (gFlashTimer < (LCD_FLASH_SEED >> 1)) {
            flash = 0;
        }
        else {
            flash = 1;
        }

        // Repeat for the six LCD characters
        for (i = 0; i < 6; i++)
        {
            if ((gScroll+i) >= 0 && (!EOL)) {
                // We have some visible characters
                c = gTextBuffer[i + gScroll];
                c_flash = c & LCD_CODE_FLASH ? 1 : 0;
                c = c & 0x7F;

                if (c == LCD_CODE_END) {
                    EOL = i+1;      // End of character data
                }
            }
            else {
                c = LCD_CODE_BLANK;
            }

            // Check if this character is flashing

            if (c_flash && flash) {
                LCD_WriteCode(LCD_CODE_BLANK, i);
            }
            else {
                LCD_WriteCode(c, i);
            }
        }

        // Handle colon
        if (gColon) {
            *(pLCDREG + 8) = 0x01;
        }
        else {
            *(pLCDREG + 8) = 0x00;
        }

        // If the text scrolled off the display,
        // we have to start over again.
        if (EOL == 1) {
            gScroll = -6;
        }
        else {
            gScroll++;
        }

        // No need to update anymore
        gLCD_Update_Required = false;
    }


    // LCD_timer is used when scrolling text
    if (LCD_timer == 0) {
/*        if ((gScroll <= 0) || EOL)
            LCD_timer = LCD_TIMER_SEED/2;
        else*/
            LCD_timer = LCD_TIMER_SEED;
    }

    // gFlashTimer is used when flashing characters
    if (gFlashTimer == LCD_FLASH_SEED) {
        gFlashTimer= 0;
    }
    else {
        gFlashTimer++;
    }

}
