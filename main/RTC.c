//*****************************************************************************
//
//  File........: RTC.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: AVR-GCC 4.1.1; avr-libc 1.4.5
//
//  Description.: Real Time Clock (RTC)
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - Created                                       - LHM
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas
//  20051107          minior correction (volatiles)                 - mt
//  20070129          SIGNAL->ISR                                   - mt
//*****************************************************************************

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "calendar.h"
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include "button.h"
#include "main.h"
#include "RTC.h"
#include "LCD_functions.h"
#include "BCD.h"

//char gSECOND;
//char gMINUTE;
//char gHOUR;
//char gDAY;
//char gMONTH;
volatile uint8_t  gSECOND;
volatile uint8_t  gMINUTE;
volatile uint8_t  gHOUR;
volatile uint8_t  gDAY;
volatile uint8_t  gMONTH;
volatile uint16_t gYEAR;

//char gPowerSaveTimer = 0;
//char dateformat = 0;
volatile uint8_t gPowerSaveTimer = 0;
uint8_t dateformat = 0;

// Lookup table holding the length of each mont. The first element is a dummy.
// mt this could be placed in progmem too, but the arrays are accessed quite
//    often - so leaving them in RAM is better...

char TBL_CLOCK_12[] =   // table used when displaying 12H clock  
{12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

char clockformat = CLOCK_24;    // set initial clock format to 24H

// different date formates (text only)
//__flash char EUROPEAN_DATE_TEXT[] =   "DDMMYY";
//__flash char AMERICAN_DATE_TEXT[] =   "MMDDYY";
//__flash char CANADIAN_DATE_TEXT[] =   "YYMMDD"; 
const char EUROPEAN_DATE_TEXT[] PROGMEM =   "DDMMYY";
const char AMERICAN_DATE_TEXT[] PROGMEM =   "MMDDYY";
const char CANADIAN_DATE_TEXT[] PROGMEM =   "YYMMDD"; 

// different date formates, table for putting DD, MM and YY at the right place
// on the LCD
//__flash char EUROPEAN_DATE_NR[] =   { 4, 5, 2, 3, 0, 1 };
//__flash char AMERICAN_DATE_NR[] =   { 4, 5, 0, 1, 2, 3 };
//__flash char CANADIAN_DATE_NR[] =   { 0, 1, 2, 3, 4, 5 }; 
const __flash uint8_t EUROPEAN_DATE_NR[] =   { 4, 5, 2, 3, 0, 1 };
const __flash uint8_t AMERICAN_DATE_NR[] =   { 4, 5, 0, 1, 2, 3 };
const __flash uint8_t CANADIAN_DATE_NR[] =   { 0, 1, 2, 3, 4, 5 }; 

//__flash char __flash *DATEFORMAT_TEXT[] = {EUROPEAN_DATE_TEXT, AMERICAN_DATE_TEXT, CANADIAN_DATE_TEXT};
//__flash char __flash *DATE_FORMAT_NR[] = {EUROPEAN_DATE_NR, AMERICAN_DATE_NR, CANADIAN_DATE_NR};
PGM_P DATEFORMAT_TEXT[] = {EUROPEAN_DATE_TEXT, AMERICAN_DATE_TEXT, CANADIAN_DATE_TEXT};
const __flash uint8_t * const __flash DATE_FORMAT_NR[] = {EUROPEAN_DATE_NR, AMERICAN_DATE_NR, CANADIAN_DATE_NR};




/******************************************************************************
*
*   Function name:  RTC_init
*
*   returns:        none
*
*   parameters:     none
*
*   Purpose:        Start Timer/Counter2 in asynchronous operation using a
*                   32.768kHz crystal.
*
*******************************************************************************/
void RTC_init(void)
{
    Delay(1000);            // wait for 1 sec to let the Xtal stabilize after a power-on,

    cli(); // mt __disable_interrupt();  // disabel global interrupt

    cbiBF(TIMSK2, TOIE2);             // disable OCIE2A and TOIE2

    ASSR = (1<<AS2);        // select asynchronous operation of Timer2

    TCNT2 = 0;              // clear TCNT2A
    TCCR2A |= (1<<CS22) | (1<<CS20);             // select precaler: 32.768 kHz / 128 = 1 sec between each overflow

    while((ASSR & 0x01) | (ASSR & 0x04));       // wait for TCN2UB and TCR2UB to be cleared

    TIFR2 = 0xFF;           // clear interrupt-flags
    sbiBF(TIMSK2, TOIE2);     // enable Timer2 overflow interrupt

    sei(); // mt __enable_interrupt();                 // enable global interrupt

    // initial time and date setting
    gSECOND  = 0;
    gMINUTE  = 0;
    gHOUR    = 12;
    // mt release timestamp
    gDAY     = 12;
    gMONTH   = 3;
    gYEAR    = 9;
}


// Consistent snapshots of the time/date: Time_update() (Timer2 ISR) changes
// several of these variables, and gYEAR is 16 bit (not atomic on 8-bit AVR).
static void GetTime(uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        *hour   = gHOUR;
        *minute = gMINUTE;
        *second = gSECOND;
    }
}

static void GetDate(uint16_t *year, uint8_t *month, uint8_t *day)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        *year  = gYEAR;
        *month = gMONTH;
        *day   = gDAY;
    }
}

/*****************************************************************************
*
*   Function name : ShowClock
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Shows the clock on the LCD
*
*****************************************************************************/
char ShowClock(char input)
{
    //char HH, HL, MH, ML, SH, SL;
    uint8_t HH, HL, MH, ML, SH, SL;
    uint8_t hour, minute, second;

    GetTime(&hour, &minute, &second);

    if (clockformat == CLOCK_12)    // if 12H clock
        HH = CHAR2BCD2(TBL_CLOCK_12[hour]);   
    else
        HH = CHAR2BCD2(hour);
        
    HL = (HH & 0x0F) + '0';
    HH = (HH >> 4) + '0';

    MH = CHAR2BCD2(minute);
    ML = (MH & 0x0F) + '0';
    MH = (MH >> 4) + '0';

    SH = CHAR2BCD2(second);
    SL = (SH & 0x0F) + '0';
    SH = (SH >> 4) + '0';

    LCD_putc(0, HH);
    LCD_putc(1, HL);
    LCD_putc(2, MH);
    LCD_putc(3, ML);
    LCD_putc(4, SH);
    LCD_putc(5, SL);
    LCD_putc(6, '\0');

    LCD_Colon(1);

    LCD_UpdateRequired(TRUE, 0);

    if (input == KEY_PREV)
        return ST_TIME_CLOCK;
    else if (input == KEY_NEXT)
        return ST_TIME_CLOCK_ADJUST;
      
    return ST_TIME_CLOCK_FUNC;
}

#define HOUR       0
#define MINUTE     1
#define SECOND     2


/*****************************************************************************
*
*   Function name : SetClock
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Adjusts the clock
*
*****************************************************************************/
char SetClock(char input)
{
    static char enter_function = 1;
    // static char time[3];    // table holding the temporary clock setting
    // static char mode = HOUR;
    // char HH, HL, MH, ML, SH, SL;
    static uint8_t time[3];
    static uint8_t mode = HOUR;
    uint8_t HH, HL, MH, ML, SH, SL;

    if (enter_function)
    {
        GetTime(&time[HOUR], &time[MINUTE], &time[SECOND]);
    }

    if (clockformat == CLOCK_12)    // if 12H clock
        HH = CHAR2BCD2(TBL_CLOCK_12[time[HOUR]]);
    else
        HH = CHAR2BCD2(time[HOUR]);
        
    HL = (HH & 0x0F) + '0';
    HH = (HH >> 4) + '0';

    MH = CHAR2BCD2(time[MINUTE]);
    ML = (MH & 0x0F) + '0';
    MH = (MH >> 4) + '0';

    SH = CHAR2BCD2(time[SECOND]);
    SL = (SH & 0x0F) + '0';
    SH = (SH >> 4) + '0';

    LCD_putc(0, HH | ((mode == HOUR) ? 0x80 : 0x00));
    LCD_putc(1, HL | ((mode == HOUR) ? 0x80 : 0x00));
    LCD_putc(2, MH | ((mode == MINUTE) ? 0x80 : 0x00));
    LCD_putc(3, ML | ((mode == MINUTE) ? 0x80 : 0x00));
    LCD_putc(4, SH | ((mode == SECOND) ? 0x80 : 0x00));
    LCD_putc(5, SL | ((mode == SECOND) ? 0x80 : 0x00));
    LCD_putc(6, '\0');

    LCD_Colon(1);

    if (input != KEY_NULL)
        LCD_FlashReset();

    LCD_UpdateRequired(TRUE, 0);
    
    enter_function = 1;

    // Increment/decrement hours, minutes or seconds
    if (input == KEY_PLUS)
        time[mode]++;
    else if (input == KEY_MINUS)
        time[mode]--;
    else if (input == KEY_PREV)
    {
        if (mode == HOUR)
            mode = SECOND;
        else
            mode--;
    }
    else if (input == KEY_NEXT)
    {
        if (mode == SECOND)
            mode = HOUR;
        else
            mode++;
    }
    else if (input == KEY_ENTER)
    {
        // store the temporary adjusted values to the global variables
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            gHOUR = time[HOUR];
            gMINUTE = time[MINUTE];
            gSECOND = time[SECOND];
        }
        mode = HOUR;
        return ST_TIME_CLOCK_FUNC;
    }

    /* OPTIMIZE: Can be solved by using a modulo operation */
    if (time[HOUR] == 255)
        time[HOUR] = 23;
    if (time[HOUR] > 23)
        time[HOUR] = 0;

    if (time[MINUTE] == 255)
        time[MINUTE] = 59;
    if (time[MINUTE] > 59)
        time[MINUTE] = 0;

    if (time[SECOND] == 255)
        time[SECOND] = 59;
    if (time[SECOND] > 59)
        time[SECOND] = 0;

    enter_function = 0;
    return ST_TIME_CLOCK_ADJUST_FUNC;
}




/*****************************************************************************
*
*   Function name : SetClockFormat
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Adjusts the Clockformat
*
*****************************************************************************/
char SetClockFormat(char input)
{
    static char enter = 1;
    
    if(enter)
    {
        enter = 0;
        
        if(clockformat == CLOCK_24)
            LCD_puts_f(PSTR("24H"), 1);	 // mt LCD_puts("24H", 1);            
        else
            LCD_puts_f(PSTR("12H"), 1);	// mt LCD_puts("12H", 1);		

    }
    if (input == KEY_PLUS)
    {
        if(clockformat == CLOCK_24)
        {
            clockformat = CLOCK_12;
            LCD_puts_f(PSTR("12H"), 1); // mt LCD_puts("12H", 1);
        }
        else
        {
            clockformat = CLOCK_24;
            LCD_puts_f(PSTR("24H"), 1); // mt LCD_puts("24H", 1);            
        }
    }
    else if (input == KEY_MINUS)
    {
        if(clockformat == CLOCK_12)
        {
            clockformat = CLOCK_24;
            LCD_puts_f(PSTR("24H"), 1);	// mt LCD_puts("24H", 1);
        }
        else
        {
            clockformat = CLOCK_12;
            LCD_puts_f(PSTR("12H"), 1);   // mt LCD_puts("12H", 1);            
        }
    }
    else if (input == KEY_ENTER)    
    {
        enter = 1;
        return ST_TIME_CLOCK_FUNC;
    }        
    return ST_TIME_CLOCKFORMAT_ADJUST_FUNC;
}




/*****************************************************************************
*
*   Function name : ShowDate
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Shows the date on the LCD
*
*****************************************************************************/
char ShowDate(char input)
{
    char YH, YL, MH, ML, DH, DL;
    uint16_t year;
    uint8_t month, day;

    GetDate(&year, &month, &day);

    YH = CHAR2BCD2(year);
    YL = (YH & 0x0F) + '0';
    YH = (YH >> 4) + '0';

    MH = CHAR2BCD2(month);
    ML = (MH & 0x0F) + '0';
    MH = (MH >> 4) + '0';

    DH = CHAR2BCD2(day);
    DL = (DH & 0x0F) + '0';
    DH = (DH >> 4) + '0';

	/*
    LCD_putc( *(DATE_FORMAT_NR[dateformat] + 0), YH);
    LCD_putc( *(DATE_FORMAT_NR[dateformat] + 1), YL);
    LCD_putc( *(DATE_FORMAT_NR[dateformat] + 2), MH);
    LCD_putc( *(DATE_FORMAT_NR[dateformat] + 3), ML);
    LCD_putc( *(DATE_FORMAT_NR[dateformat] + 4), DH);
    LCD_putc( *(DATE_FORMAT_NR[dateformat] + 5), DL);
	*/
   	const __flash uint8_t *pDateFormatNr = DATE_FORMAT_NR[dateformat];
    LCD_putc( *pDateFormatNr++, YH);
    LCD_putc( *pDateFormatNr++, YL);
    LCD_putc( *pDateFormatNr++, MH);
    LCD_putc( *pDateFormatNr++, ML);
	LCD_putc( *pDateFormatNr++, DH);
    LCD_putc( *pDateFormatNr, DL);

    LCD_putc(6, '\0');

    LCD_Colon(1);

    LCD_UpdateRequired(TRUE, 0);


    if (input == KEY_PREV)
        return ST_TIME_DATE;
    else if (input == KEY_NEXT)
        return ST_TIME_DATE_ADJUST;
    else   
        return ST_TIME_DATE_FUNC;
}

#define YEAR        0
#define MONTH       1
#define DAY         2


/*****************************************************************************
*
*   Function name : SetDate
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Adjusts the date
*
*****************************************************************************/
char SetDate(char input)
{
	static char enter_function = 1;
	// static char date[3];    // table holding the temporary date setting
	// static char mode = DAY;
	// char YH, YL, MH, ML, DH, DL;
	// char MonthLength_temp;
	// char LeapMonth;
	static uint8_t date[3];    // table holding the temporary date setting
	static uint8_t mode = DAY;
	uint8_t YH, YL, MH, ML, DH, DL;
	uint8_t MonthLength_temp;

    if (enter_function)
    {
        uint16_t year;

        GetDate(&year, &date[MONTH], &date[DAY]);
        date[YEAR] = year;
    }

    if (mode == YEAR)
    {
        YH = CHAR2BCD2(date[YEAR]);
        YL = (YH & 0x0F) + '0';
        YH = (YH >> 4) + '0';
        
        LCD_putc( 0, ' ');
        LCD_putc( 1, ' ');   
        LCD_putc( 2, 'Y');
        LCD_putc( 3, 'Y');        
        LCD_putc( 4, YH);
        LCD_putc( 5, YL);
    }
    else if (mode == MONTH)
    {
        MH = CHAR2BCD2(date[MONTH]);
        ML = (MH & 0x0F) + '0';
        MH = (MH >> 4) + '0';

        LCD_putc( 0, ' ');
        LCD_putc( 1, ' ');   
        LCD_putc( 2, 'M');
        LCD_putc( 3, 'M');        
        LCD_putc( 4, MH);
        LCD_putc( 5, ML);
    }
    else if (mode == DAY)
    {
        DH = CHAR2BCD2(date[DAY]);
        DL = (DH & 0x0F) + '0';
        DH = (DH >> 4) + '0';

        LCD_putc( 0, ' ');
        LCD_putc( 1, ' ');   
        LCD_putc( 2, 'D');
        LCD_putc( 3, 'D');        
        LCD_putc( 4, DH);
        LCD_putc( 5, DL);
    }

    LCD_putc(6, '\0');

    LCD_Colon(0);

    if (input != KEY_NULL)
        LCD_FlashReset();

    LCD_UpdateRequired(TRUE, 0);


    enter_function = 1;

    // Increment/decrement years, months or days
    if (input == KEY_PLUS)
        date[mode]++;
    else if (input == KEY_MINUS)
        date[mode]--;
    else if (input == KEY_PREV)
    {
        if (mode == YEAR)
            mode = DAY;
        else
            mode--;
    }
    else if (input == KEY_NEXT)
    {
        if (mode == DAY)
            mode = YEAR;
        else
            mode++;
    }
    else if (input == KEY_ENTER)
    {
        // store the temporary adjusted values to the global variables
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            gYEAR = date[YEAR];
            gMONTH = date[MONTH];
            gDAY = date[DAY];
        }
        mode = YEAR;
        return ST_TIME_DATE_FUNC;
    }

    /* OPTIMIZE: Can be solved by using a modulo operation */
    if (date[YEAR] == 255)
        date[YEAR] = 99;
    if (date[YEAR] > 99)
        date[YEAR] = 0;

    if (date[MONTH] == 0)
        date[MONTH] = 12;
    if (date[MONTH] > 12)
        date[MONTH] = 1;

    // number of days of the month that is being set (leap years included)
    MonthLength_temp = days_in_month(date[YEAR], date[MONTH]);
    
    if (date[DAY] == 0)
        date[DAY] = MonthLength_temp;
    if (date[DAY] > MonthLength_temp)
        date[DAY] = 1;

    enter_function = 0;
    
    return ST_TIME_DATE_ADJUST_FUNC;
}




/*****************************************************************************
*
*   Function name : SetDateFormat
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Adjusts the Dateformat
*
*****************************************************************************/
char SetDateFormat(char input)
{
    static char enter = 1;

    if(enter)
    {
        enter = 0;

        LCD_puts_f(DATEFORMAT_TEXT[dateformat], 1);
    }
    if (input == KEY_PLUS)
    {
        if(dateformat >= 2)
            dateformat = 0;
        else
            dateformat++;

        LCD_puts_f(DATEFORMAT_TEXT[dateformat], 1);        
    }
    else if (input == KEY_MINUS)
    {
        if(dateformat == 0)
            dateformat = 2;
        else
            dateformat--;
            
        LCD_puts_f(DATEFORMAT_TEXT[dateformat], 1);            
    }
    else if (input == KEY_ENTER)    
    {
        enter = 1;
        return ST_TIME_DATE_FUNC;
    }        
    return ST_TIME_DATEFORMAT_ADJUST_FUNC;
}

/******************************************************************************
*
*   Timer/Counter2 Overflow Interrupt Routine
*
*   Purpose: Increment the real-time clock
*            The interrupt occurs once a second (running from the 32kHz crystal)
*
*******************************************************************************/
// #pragma vector = TIMER2_OVF_vect
// __interrupt void TIMER2_OVF_interrupt(void)
// SIGNAL(SIG_OVERFLOW2)
ISR(TIMER2_OVF_vect)
{
    gSECOND++;               // increment second

    if (gSECOND == 60)
    {
        gSECOND = 0;
        gMINUTE++;
        
        gPowerSaveTimer++;
        
        if (gMINUTE > 59)
        {
            gMINUTE = 0;
            gHOUR++;
            
            if (gHOUR > 23)
            {
                
                gHOUR = 0;
                gDAY++;

                // Now, we can check for month length
                if (gDAY > days_in_month(gYEAR, gMONTH))
                {
                    gDAY = 1;
                    gMONTH++;

                    if (gMONTH > 12)
                    {
                        gMONTH = 1;
                        gYEAR++;
                    }
                }
            }
        }
    }
}
