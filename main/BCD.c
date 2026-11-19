#include <stdint.h>
#include "BCD.h"
//***************************************************************************
//
//  File........: BCD.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: IAR EWAAVR 2.28a
//
//  Description.: AVR Butterfly BCD conversion algorithms
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - KS
//
//***************************************************************************



/*****************************************************************************
*
*   Function name : CHAR2BCD2
*
*   Returns :       Binary coded decimal value of the input (2 digits)
*
*   Parameters :    Value between (0-99) to be encoded into BCD 
*
*   Purpose :       Convert a character into a BCD encoded character.
*                   The input must be in the range 0 to 99.
*                   The result is byte where the high and low nibbles
*                   contain the tens and ones of the input.
*
*****************************************************************************/
uint8_t CHAR2BCD2(uint8_t input)
{
    uint8_t high = 0;
    
    
    while (input >= 10)                 // Count tens
    {
        high++;
        input -= 10;
    }

    return  (high << 4) | input;        // Add ones and return answer
}

/*****************************************************************************
*
*   Function name : CHAR2BCD3
*
*   Returns :       Binary coded decimal value of the input (3 digits)
*
*   Parameters :    Value between (0-255) to be encoded into BCD 
*
*   Purpose :       Convert a character into a BCD encoded character.
*                   The input must be in the range 0 to 255.
*                   The result is an integer where the three lowest nibbles
*                   contain the ones, tens and hundreds of the input.
*
*****************************************************************************/
uint16_t CHAR2BCD3(uint8_t input)
{
    int high = 0;
        
    while (input >= 100)                // Count hundreds
    {
        high++;
        input -= 100;
    }

    high <<= 4;
    
    while (input >= 10)                 // Count tens
    {
        high++;
        input -= 10;
    }

    return  (high << 4) | input;        // Add ones and return answer
}
