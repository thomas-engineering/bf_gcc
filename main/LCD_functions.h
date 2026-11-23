#ifndef LCD_FUNCTIONS_H_
#define LCD_FUNCTIONS_H_

#include <stdint.h>
#include <stdbool.h>
//*****************************************************************************
//
//  File........: LCD_functions.h
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Description.: Functions for LCD_functions.c
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - File created                                  - LHM
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//  202611xx          see https://github.com/thomas-engineering/bf_gcc
//
//*****************************************************************************

#include <avr/pgmspace.h>

//Functions
void LCD_puts_f(const char *pFlashStr, uint8_t scrollmode);
void LCD_puts(char *pStr, uint8_t scrollmode);
void LCD_UpdateRequired(bool update, uint8_t scrollmode);
//mt void LCD_putc(char digit, char character);
void LCD_putc(uint8_t digit, char character);
void LCD_Clear(void);
void LCD_Colon(bool show);
void LCD_FlashReset(void);
uint8_t SetContrast(uint8_t input);

#endif /* LCD_FUNCTIONS_H_ */
