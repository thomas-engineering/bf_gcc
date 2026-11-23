//***************************************************************************
//
//  File........: eeprom.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: avr-gcc 7.3.0; avr-libc 2.0.0
//
//  Description.: AVR Butterfly EEPROM routines
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - LHM
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//  20070129          use avr-libc ATmega169-eeprom-support         - mt
//  202611xx          see https://github.com/thomas-engineering/bf_gcc
//
//***************************************************************************

#include <stdint.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include "bfeeprom.h"

/*****************************************************************************
*
*   Function name : StoreEEPROM
*
*   Returns :       None
*
*   Parameters :    Pointer to string, number of bytes to write, adress in EEPROM
*
*   Purpose :       Write byte(s) to the EEPROM
*
*****************************************************************************/
void StoreEEPROM(char *pBuffer, uint8_t num_bytes, uint8_t* ee_start_addr)
{
    uint8_t i;
    for (i=0;i<num_bytes;i++) {
        eeprom_update_byte(ee_start_addr, pBuffer[i]);
        ee_start_addr++;
    }
}


/*****************************************************************************
*
*   Function name : ReadEEPROM
*
*   Returns :       None
*
*   Parameters :    Pointer to string, number of bytes to read, adress in EEPROM
*
*   Purpose :       Write byte(s) to the EEPROM
*
*****************************************************************************/

void LoadEEPROM(char *pBuffer, uint8_t num_bytes, uint8_t* ee_start_addr)
{
    uint8_t i;
    for (i=0;i<num_bytes;i++) {
        pBuffer[i]=eeprom_read_byte(ee_start_addr);
        ee_start_addr++;
    }
}
