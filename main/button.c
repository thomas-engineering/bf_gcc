//***************************************************************************
//
//  File........: button.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: AVR-GCC 4.1.1; avr-libc 1.4.5
//
//  Description.: AVR Butterfly button handling routines
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - KS
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//  20070129          SIGNAL->ISR                                   - mt
//
//***************************************************************************

//#include <inavr.h>
//#include "iom169.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "main.h"
#include "button.h"
#include "timer0.h"
// for keyclick:
#include "sound.h"
// for gPowerSaveTimer:
#include "RTC.h"

// mt  uint8_t gButtonTimeout = FALSE;
volatile uint8_t gButtonTimeout = FALSE;

//char KEY = NULL;
//char KEY_VALID = FALSE;
volatile uint8_t KEY = KEY_NULL;
volatile uint8_t KEY_VALID = FALSE;

// Set by PinChangeInterrupt(), cleared by Button_ServiceClick(): the click takes
// about 20 ms and must not be played with interrupts disabled.
static volatile uint8_t ClickRequest = FALSE;

uint8_t CountdownTimerHandle;

/*****************************************************************************
*
*   Function name : Button_Init
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Initializes the five button pin
*
*****************************************************************************/
void Button_Init(void)
{
    // Init port pins
    cbiBF(DDRB,7);
    cbiBF(DDRB,6);
    cbiBF(DDRB,4);
    PORTB |= PINB_MASK;
    DDRE = 0x00;
    PORTE |= PINE_MASK;

    // Enable pin change interrupt on PORTB and PORTE
    PCMSK0 = PINE_MASK;
    PCMSK1 = PINB_MASK;
    EIFR = (1<<PCIF0)|(1<<PCIF1);
    EIMSK = (1<<PCIE0)|(1<<PCIE1);

    CountdownTimerHandle = Timer0_AllocateCountdownTimer();
}

// #pragma vector = PCINT0_vect
// __interrupt void PCINT0_interrupt(void)
ISR(PCINT0_vect)
{
    PinChangeInterrupt();
}

// Same handler for both pin-change vectors. An alias keeps GCC from folding
// the two identical bodies into a nested call (double prologue, early reti).
ISR_ALIAS(PCINT1_vect, PCINT0_vect);


/*****************************************************************************
*
*   Function name : PinChangeInterrupt
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Check status on the joystick
*
*****************************************************************************/
void PinChangeInterrupt(void)
{
    uint8_t buttons;

    uint8_t key;

/*
    Read the buttons:

    Bit             7   6   5   4   3   2   1   0
    ---------------------------------------------
    PORTB           B   A       O
    PORTE                           D   C
    ---------------------------------------------
    PORTB | PORTE   B   A       O   D   C
    =============================================
*/


    buttons = (~PINB) & PINB_MASK;
    buttons |= (~PINE) & PINE_MASK;

    // Output virtual keys
    if (buttons & (1<<BUTTON_A))
        key = KEY_PLUS;
    else if (buttons & (1<<BUTTON_B))
        key = KEY_MINUS;
    else if (buttons & (1<<BUTTON_C))
        key = KEY_PREV;
    else if (buttons & (1<<BUTTON_D))
        key = KEY_NEXT;
    else if (buttons & (1<<BUTTON_O))
        key = KEY_ENTER;
    else
        key = KEY_NULL;

    
    if(key != KEY_NULL)
    {
        if(gButtonTimeout)  // gButtonTimeout is set in the LCD_SOF_interrupt in LCD_driver.c
        {
            if (!KEY_VALID)
            {
                KEY = key;          // Store key in global key buffer
                KEY_VALID = TRUE;
                if (gKeyClickStatus)
                    ClickRequest = TRUE;
            }

         gButtonTimeout = FALSE;
    
        }
    }
    
    EIFR = (1<<PCIF1) | (1<<PCIF0);     // Delete pin change interrupt flags

    gPowerSaveTimer = 0;                // Reset the Auto Power Down timer
    
}


/*****************************************************************************
*
*   Function name : getkey
*
*   Returns :       The valid key
*
*   Parameters :    None
*
*   Purpose :       Get the valid key 
*
*****************************************************************************/
uint8_t getkey(void)
{
    uint8_t k;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (KEY_VALID)              // Check for unread key in buffer
        {
            k = KEY;
            KEY_VALID = FALSE;
        }
        else
            k = KEY_NULL;           // No key stroke available
    }

    return k;
}


/*****************************************************************************
*
*   Function name : Button_ServiceClick
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Play the key click that PinChangeInterrupt() asked for.
*                   Call from the main loop, never from an interrupt.
*
*****************************************************************************/
void Button_ServiceClick(void)
{
    uint8_t request;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        request = ClickRequest;
        ClickRequest = FALSE;
    }

    if (request)
        PlayClick();
}
