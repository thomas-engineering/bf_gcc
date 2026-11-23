#ifndef BCD_H_
#define BCD_H_

#include <stdint.h>
//***************************************************************************
//
//  File........: BCD.h
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: IAR EWAAVR 2.28a
//
//  Description.: AVR Butterfly main module
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - KS
//
//***************************************************************************

// Function declarations
uint8_t CHAR2BCD2(uint8_t input);
uint16_t CHAR2BCD3(uint8_t input);

#endif /* BCD_H_ */
