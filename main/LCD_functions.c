//*****************************************************************************
//
//  File........: LCD_functions.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: avr-gcc 7.3.0; avr-libc 2.0.0
//
//  Description.: Additional LCD functions, scrolling text and write data
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - Created                                       - LHM
//  20030116 - 2.0  - Code adapted to AVR Butterflyup               - KS
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//  20070131          gLCD_Start_Scroll_Timer volatile              - mt
//  20070517          LCDClear, gTextBuffer[0] to 0-char            - mt
//  202611xx          see https://github.com/thomas-engineering/bf_gcc
//*****************************************************************************

//  Include files
#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "LCD_driver.h"
#include "LCD_functions.h"
#include "BCD.h"
// mt only for KEY_* and ST_OPTIONS_DISPLAY* definitions:
#include "main.h"
#include "button.h"


// mt char CONTRAST = LCD_INITIAL_CONTRAST;
uint8_t CONTRAST = LCD_INITIAL_CONTRAST;

// Start-up delay before scrolling a string over the LCD. "LCD_driver.c"

/****************************************************************************
*
*	Function name : LCD_puts_f
*
*	Returns :		None
*
*	Parameters :	pFlashStr: Pointer to the string in flash
*                   scrollmode: Not in use
*
*	Purpose :		Writes a string stored in flash to the LCD
*
*****************************************************************************/

// mt void LCD_puts_f(char __flash *pFlashStr, char scrollmode)
void LCD_puts_f(const char *pFlashStr, uint8_t scrollmode)
{
    (void)scrollmode;   // not used in this port
    // char i;
    uint8_t i;

    while (gLCD_Update_Required);      // Wait for access to buffer

    // mt: for (i = 0; pFlashStr[i] && i < TEXTBUFFER_SIZE; i++)
    for (i = 0; (const char)(pgm_read_byte(&pFlashStr[i])) && i < TEXTBUFFER_SIZE; i++)
    {
        // mt: gTextBuffer[i] = pFlashStr[i];
        gTextBuffer[i] = LCD_CharToCode(pgm_read_byte(&pFlashStr[i]));   // translate once, here
    }

    gTextBuffer[i] = LCD_CODE_END;

    if (i > 6) {
        gScrollMode = 1;        // Scroll if text is longer than display size
        gScroll = 0;
        gLCD_Start_Scroll_Timer = 3;    //Start-up delay before scrolling the text
    }
    else {
        gScrollMode = 0;        
        gScroll = 0;
    }

    gLCD_Update_Required = true;
}


/****************************************************************************
*
*	Function name : LCD_puts
*
*	Returns :		None
*
*	Parameters :	pStr: Pointer to the string
*                   scrollmode: Not in use
*
*	Purpose :		Writes a string to the LCD
*
*****************************************************************************/
void LCD_puts(char *pStr, uint8_t scrollmode)
{
    (void)scrollmode;   // not used in this port
    uint8_t i; // char i;

    while (gLCD_Update_Required);      // Wait for access to buffer

    for (i = 0; pStr[i] && i < TEXTBUFFER_SIZE; i++)
    {
        gTextBuffer[i] = LCD_CharToCode(pStr[i]);   // translate once, here
    }

    gTextBuffer[i] = LCD_CODE_END;

    if (i > 6) {
        gScrollMode = 1;        // Scroll if text is longer than display size
        gScroll = 0;
        gLCD_Start_Scroll_Timer = 3;    //Start-up delay before scrolling the text
    }
    else {
        gScrollMode = 0;        
        gScroll = 0;
    }

    gLCD_Update_Required = true;
}


/****************************************************************************
*
*   Function name: LCD_putc
*
*   Returns :      None
*
*   Parameters :   digit: Which digit to write on the LCD
*                  character: Character to write
*
*   Purpose :      Writes a character to the LCD
*
*****************************************************************************/
// mt void LCD_putc(char digit, char character)
void LCD_putc(uint8_t digit, char character)
{
    if (digit < TEXTBUFFER_SIZE) {
        gTextBuffer[digit] = LCD_CharToCode(character);
    }
}


/****************************************************************************
*
*	Function name : LCD_Clear
*
*	Returns :		None
*
*	Parameters :	None
*
*	Purpose :		Clear the LCD
*
*****************************************************************************/
void LCD_Clear(void)
{
    uint8_t i; // char i;

    for (i=0; i<TEXTBUFFER_SIZE; i++) {
        gTextBuffer[i] = LCD_CODE_BLANK;
    }

    gTextBuffer[0] = LCD_CODE_END; // mt 5/2007
}


/****************************************************************************
*
*	Function name : LCD_Colon
*
*	Returns :		None
*
*	Parameters :	show: Enables the colon if true, disable if false
*
*	Purpose :		Enable/disable colons on the LCD
*
*****************************************************************************/
void LCD_Colon(bool show)
{
    gColon = show;
}


/****************************************************************************
*
*	Function name : LCD_UpdateRequired
*
*	Returns :		None
*
*	Parameters :	update: true/false
*                   scrollmode: not in use
*
*	Purpose :		Tells the LCD that there is new data to be presented
*
*****************************************************************************/
void LCD_UpdateRequired(bool update, uint8_t scrollmode)
{

    while (gLCD_Update_Required);
    
    gScrollMode = scrollmode;
    gScroll = 0;

    gLCD_Update_Required = update;
}


/****************************************************************************
*
*	Function name : LCD_FlashReset
*
*	Returns :		None
*
*	Parameters :	None
*
*	Purpose :		This function resets the blinking cycle of a flashing digit
*
*****************************************************************************/
void LCD_FlashReset(void)
{
    gFlashTimer = 0;
}



/****************************************************************************
*
*	Function name : SetContrast
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*	Purpose :		Adjust the LCD contrast
*
*****************************************************************************/
uint8_t SetContrast(uint8_t input)
{
    static uint8_t enter = 1;
    uint8_t CH, CL;

    if (enter) {
        LCD_Clear();
        enter = 0;
    }

    CH = CHAR2BCD2(CONTRAST);
    CL = (CH & 0x0F) + '0';
    CH = (CH >> 4) + '0';

    LCD_putc(0, 'C');
    LCD_putc(1, 'T');
    LCD_putc(2, 'R');
    LCD_putc(3, ' ');
    LCD_putc(4, CH);
    LCD_putc(5, CL);

    LCD_UpdateRequired(true, 0);

    if (input == KEY_PLUS) {
        CONTRAST++;
    }
    else if (input == KEY_MINUS) {
        CONTRAST--;
    }

    if (CONTRAST == 255) {
        CONTRAST = 0;
    }
    if (CONTRAST > 15) {
        CONTRAST = 15;
    }

    LCD_CONTRAST_LEVEL(CONTRAST);


    if (input == KEY_ENTER) {
        enter = 1;
        return ST_OPTIONS_DISPLAY_CONTRAST;
    }

    return ST_OPTIONS_DISPLAY_CONTRAST_FUNC;
}


