#include <stdint.h>
//*****************************************************************************
//
//  File........: RTC.h
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Description.: Functions for RTC.c
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - File created                                  - LHM
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//
//*****************************************************************************


//extern char gSECOND;
//extern char gMINUTE;
//extern char gHOUR;
//extern char gDAY;
//extern char gMONTH;
extern volatile uint8_t  gSECOND;
extern volatile uint8_t  gMINUTE;
extern volatile uint8_t  gHOUR;
extern volatile uint8_t  gDAY;
extern volatile uint8_t  gMONTH;
extern volatile uint16_t gYEAR;

extern volatile uint8_t  gPowerSaveTimer;

//  Function declarations
void RTC_init(void);            //initialize the Timer Counter 2 in asynchron operation
void Time_update(void);        //updates the time and date
uint8_t ShowClock(uint8_t input);
uint8_t SetClock(uint8_t input);
uint8_t SetClockFormat(uint8_t input);
uint8_t ShowDate(uint8_t input);
uint8_t SetDate(uint8_t input);
uint8_t SetDateFormat(uint8_t input);

#define CLOCK_24    1
#define CLOCK_12    0
