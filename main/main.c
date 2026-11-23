//***************************************************************************
//
//  File........: main.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: avr-gcc 7.3.0; avr-libc 2.0.0
//
//  Description.: AVR Butterfly main module
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - KS
//  20031009          port to avr-gcc/avr-libc                      - M.Thomas (*)
//  20031204          fixed imcompatibility with sleep-modes        - mt
//  20040218          fixed 'logical and' in calibration            - shc/mt
//  20040827          fixed avr-libc iom169.h compatibility
//                    added keyclick function (from version6)       - mt/v6
//  20050726          minor cleanup                                 - mt
//  20060107          modifications in KeyClick(), cleanups         - mt
//  20070129          update with code from Rev07, using 
//                    avr-gcc 4.1.1/avr-libc 1.4.5 (WinAVR_20070122)- mt
//  20070131          avr-libc sleep-functions/macros               - mt
//  202611xx          see https://github.com/thomas-engineering/bf_gcc
//
//***************************************************************************

// (*) Martin Thomas, Kaiserslautern, Germany
//
// I'm not working for ATMEL.
// The port is based on REV_06 of the ATMEL-Code (for IAR-C) with fixes/updates
// from REV_07.
// Initially I marked my changes with "// mt" or enclosed them with "// mtA" and 
// "// mtE" but forgot this for some changes esp. during debugging. 'diff' 
// against the original code to see everything that has been changed.

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/version.h>
#include <util/delay.h>

#if __AVR_LIBC_VERSION__ < 10405UL
#warning "avr-libc >= version 1.4.5 recommended"
#warning "This code has not been tested with older versions."
#endif

#include "main.h"
#include "board.h"
#include "LCD_functions.h"
#include "LCD_driver.h"
#include "button.h"
#include "RTC.h"
#include "timer0.h"
#include "BCD.h"
#include "usart.h"
#include "sound.h"
#include "ADC.h"
#include "dataflash.h"
// mt Test() is not realy needed 
//- can not be accessed without external hardware
#include "test.h"
#include "vcard.h"
#include "menu.h"

#define pLCDREG_test (*(uint8_t *)(0xEC))

uint8_t PowerSaveTimeout = 30;     // Initial value, enable power save mode after 30 min
bool AutoPowerSave    = true;    // Variable to enable/disable the Auto Power Save func
volatile bool gKeyClickStatus  = false;  // Variable to enable/disable keyclick

bool PowerSave = false;

uint8_t state;  // holds the current state, according to "menu.h"


/*****************************************************************************
*
*   Function name : main
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Contains the main loop of the program
*
*****************************************************************************/
int main(void)
{    
//  uint8_t state, nextstate;
    uint8_t nextstate;
    // mt static char __flash *statetext;
    PGM_P statetext;
    uint8_t (*pStateFunc)(uint8_t);
    uint8_t input;
    uint8_t i, j; // char i;
    uint8_t buttons;
    uint8_t last_buttons;

    last_buttons='\0';  // mt

    // Initial state variables
    state = ST_AVRBF;
    nextstate = ST_AVRBF;
    statetext = MT_AVRBF;
    pStateFunc = NULL;


    // Program initalization
    Initialization();
    sei(); // mt __enable_interrupt();

    for (;;)            // Main loop
    {
        Button_ServiceClick();  // key click requested by the pin change ISR
        ADC_Service();          // measurement requested by the Timer0 ISR

        if(!PowerSave) { // Do not enter main loop in power-save
            // Plain menu text
            if (statetext) {
                LCD_puts_f(statetext, 1);
                LCD_Colon(0);
                statetext = NULL;
            }
 
            input = getkey();           // Read buttons
    
            if (pStateFunc) {
                // When in this state, we must call the state function
                nextstate = pStateFunc(input);
            }
            else if (input != KEY_NULL) {
                // Plain menu, clock the state machine
                nextstate = StateMachine(state, input);
            }
    
            if (nextstate != state) {
                state = nextstate;
                // mt: for (i=0; menu_state[i].state; i++)
                for (i=0; (j=menu_state[i].state); i++)
                {
                    //mt: if (menu_state[i].state == state)
                    //mt 1/06 if (menu_state[i].state == state)
                    if (j == state) {
                        // mt - original: statetext =  menu_state[i].pText;
                        // mt - original: pStateFunc = menu_state[i].pFunc;
                        /// mt this is like the example from an avr-gcc guru (mailing-list):
                        statetext =  menu_state[i].pText;
                        // mt - store pointer to function from menu_state[i].pFunc in pStateFunc
                        //// pStateFunc = pmttemp;	// oh je - wie soll ich das jemals debuggen - ?
                        pStateFunc = menu_state[i].pFunc;
                        break;
                    }
                }
            }
        }
        
        
        //enable ATmega169 power save modus if autopowersave
        if(AutoPowerSave) {
            if(gPowerSaveTimer >= PowerSaveTimeout) {
                state = ST_AVRBF;
                gPowerSaveTimer = 0;
                PowerSave = true;
            }
        }
        
        
        // If the joystick is held in the UP and DOWN position at the same time,
        // activate test-mode
        // if( !(PINB & (1<<PORTB7)) && !(PINB & (1<<PORTB6)) )    
        if( !(PINB & (1<<PINB7)) && !(PINB & (1<<PINB6)) ) {
            Test();
        }
        
        // Check if the joystick has been in the same position for some time, 
        // then activate auto press of the joystick
        buttons = (~PINB) & PINB_MASK;
        buttons |= (~PINE) & PINE_MASK;
        
        if( buttons != last_buttons ) {
            last_buttons = buttons;
            gAutoPressJoystick = JOY_IDLE;
        }
        else if( buttons ) {
            if( gAutoPressJoystick == JOY_PRESSED) {
                PinChangeInterrupt();
                gAutoPressJoystick = JOY_AUTO;
            }
            else {
                gAutoPressJoystick = JOY_AUTO;
            }
        }

        
        
        // go to SLEEP
        if(!gPlaying && !gUART) { // Do not enter Power save if using UART or playing tunes
            if(PowerSave) {
                cbiBF(LCDCRA, 7);             // disable LCD
            }

            // "Manual" Power-Save sleep - replaced by avr-libc functions
            // SMCR = (3<<SM0) | (1<<SE);   // Enable Power-save mode
            // asm volatile ("sleep"::);    // __sleep();  // Go to sleep
            // mt 20070131 - avr-libc 1.4.x supports ATmega169 sleep
            set_sleep_mode(SLEEP_MODE_PWR_SAVE);
            sleep_mode();

            if(PowerSave) {
                if(!(PINB & 0x40)) { // press UP to wake from SLEEP
                    PowerSave = false;
                    
                    for(i = 0; i < 20; i++ ) // set all LCD segment register to the variable ucSegments
                    {
                        *(&pLCDREG_test + i) = 0x00;
                    }
                    
                    sbiBF(LCDCRA, 7);           // enable LCD
                    input = getkey();           // Read buttons
                }
            }
        }
        else {
           // "Manual" Power-Save sleep - replaced by avr-libc functions
           // SMCR = (1<<SE);            // Enable idle mode
           // asm volatile ("sleep"::);  // __sleep();  // Go to sleep
           // mt 20070131 - avr-libc 1.4.x supports ATmega169 sleep
           set_sleep_mode(SLEEP_MODE_IDLE);
           sleep_mode();
        }

        // SMCR-reset is not needed since (from avr-libc Manual): 
        // This [the sleep_mode] macro automatically takes care to 
        // enable the sleep mode in the CPU before going to sleep, 
        // and disable it again afterwards
        // SMCR = 0;                       // Just woke, disable sleep

    } //End Main loop

    return 0; // mt
}




/*****************************************************************************
*
*   Function name : StateMachine
*
*   Returns :       nextstate
*
*   Parameters :    cur_state, stimuli
*
*   Purpose :       Shifts between the different states
*
*****************************************************************************/
uint8_t StateMachine(uint8_t cur_state, uint8_t stimuli)
{
    uint8_t nextstate = cur_state;    // Default stay in same state
    uint8_t i, j;

    // mt: for (i=0; menu_nextstate[i].state; i++)
    for (i=0; ( j=menu_nextstate[i].state ); i++ )
    {
        // mt: if (menu_nextstate[i].state == state && menu_nextstate[i].input == stimuli)
        // mt 1/06 : if (menu_nextstate[i].state == state && 
        if ( j == cur_state && 
             menu_nextstate[i].input == stimuli) {
            // This is the one!
            // mt: nextstate = menu_nextstate[i].nextstate;
            nextstate = menu_nextstate[i].nextstate;
            break;
        }
    }

    return nextstate;
}




/*****************************************************************************
*
*   Function name : Initialization
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Initializate the different modules
*
*****************************************************************************/
void Initialization(void)
{
    uint8_t tst;           // dummy

    (void)tst;          // avoid warning
    
    OSCCAL_calibration();       // calibrate the OSCCAL byte
        
    // set prescaler = 8, Inter RC 8Mhz / 8 = 1Mhz
    SetClockPrescaler(CLOCK_1MHZ);

    // Disable Analog Comparator (power save)
    ACSR = (1<<ACD);

    // Disable Digital input on PF0-2 (power save)
    DIDR0 = (7<<ADC0D);

    // mt PORTB = (15<<PORTB0);       // Enable pullup on 
    PORTB = (15<<PB0);       // Enable pullup on 
    // mt PORTE = (15<<PORTE4);
    PORTE = (15<<PE4);

    sbiBF(SPEAKER_DDR, SPEAKER_PIN);    // set OC1A as output
    sbiBF(SPEAKER_PORT, SPEAKER_PIN);   // set OC1A high
            
    Button_Init();              // Initialize pin change interrupt on joystick
    
    RTC_init();                 // Start timer2 asynchronous, used for RTC clock

    Timer0_Init();              // Used when playing music etc.

    USART_Init(UART_UBRR);      // 19200 bps (at 2 MHz, see board.h)
    
    DF_SPI_init();              // init the SPI interface to communicate with the DataFlash
    
    tst = Read_DF_status();

    DF_CS_inactive;             // disable DataFlash
        
    LCD_Init();                 // initialize the LCD
}





/*****************************************************************************
*
*   Function name : BootFunc
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Reset the ATmega169 which will cause it to start up in the 
*                   Bootloader-section. (the BOOTRST-fuse must be programmed)
*
*****************************************************************************/
// mt - as in jw-patch: const char TEXT_BOOT[] PROGMEM	= "Jump to bootloader";

uint8_t BootFunc(uint8_t input)
{
    static uint8_t enter = 1;
    
    if(enter) {
        enter = 0;
        // mt jw LCD_puts_f(TEXT_BOOT, 1);
        LCD_puts_f(PSTR("Jump to bootloader"), 1);
    }
    else if(input == KEY_ENTER) {
        WDTCR = (1<<WDCE) | (1<<WDE);     //Enable Watchdog Timer to give reset
        while(1);   // wait for watchdog-reset, since the BOOTRST-fuse is 
                    // programmed, the Boot-section will be entered upon reset.
    }
    else if (input == KEY_PREV) {
        enter = 1;
        return ST_OPTIONS_BOOT;
    }
    
    return ST_OPTIONS_BOOT_FUNC;
}





/*****************************************************************************
*
*   Function name : PowerSaveFunc
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Enable power save
*
*****************************************************************************/
// mt jw const char TEXT_POWER[]  PROGMEM  = "Press enter to sleep";

uint8_t PowerSaveFunc(uint8_t input)
{
    static uint8_t enter = 1;    
    
    if(enter) {
        enter = 0;
        //mt jw LCD_puts_f(TEXT_POWER, 1);
        LCD_puts_f(PSTR("Press enter to sleep"), 1);
    }
    else if(input == KEY_ENTER) {
        PowerSave = true;
        enter = 1;
        return ST_AVRBF;
    }
    else if (input == KEY_PREV) {
        enter = 1;
        return ST_OPTIONS_POWER_SAVE;
    }
        
    return ST_OPTIONS_POWER_SAVE_FUNC;

}




/*****************************************************************************
*
*   Function name : AutoPower
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Enable/Disable auto power save
*
*****************************************************************************/

static void AutoPowerShowMin(void)
{
	uint8_t PH,PL; 
	
	PH = CHAR2BCD2(PowerSaveTimeout);
	PL = (PH & 0x0F) + '0';
	PH = (PH >> 4) + '0';
	
	LCD_putc(0, 'M');
	LCD_putc(1, 'I');
	LCD_putc(2, 'N');
	LCD_putc(3, ' ');
	LCD_putc(4, PH);
	LCD_putc(5, PL);
	LCD_putc(6, '\0');
	
	LCD_UpdateRequired(true, 0);
}

uint8_t AutoPower(uint8_t input)
{
    static uint8_t enter = 1;    
    
    if(enter) {
        enter = 0;
        
        if(AutoPowerSave) {
            AutoPowerShowMin();
        }
        else {
            LCD_puts_f(PSTR("Off"),1);	// mt LCD_puts("Off", 1);        
        }
    }
    else if(input == KEY_ENTER) {
         enter = 1;

         return ST_OPTIONS_AUTO_POWER_SAVE;
    }
    else if (input == KEY_PLUS) {

        PowerSaveTimeout += 5;
         
        if(PowerSaveTimeout > 90) {
            PowerSaveTimeout = 90;
        }
        else {    
            AutoPowerSave = true;
            AutoPowerShowMin();
        }
    }
    else if (input == KEY_MINUS) {
        if(PowerSaveTimeout) {
            PowerSaveTimeout -= 5;
        }

        if(PowerSaveTimeout < 5) {
            AutoPowerSave = false;
            PowerSaveTimeout = 0;
            LCD_puts_f(PSTR("Off"),1);	// mt LCD_puts("Off", 1);
        }
        else {   
            AutoPowerSave = true;
            AutoPowerShowMin();
        }
    }
        
    return ST_OPTIONS_AUTO_POWER_SAVE_FUNC;    
}




/*****************************************************************************
*
*   Function name : KeyClick
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Enable/Disable keyclick
*
*****************************************************************************/
uint8_t KeyClick(uint8_t input)
{
	static uint8_t enter = 1;
	uint8_t show;

	if ( enter ) {
		enter = 0;
		show  = 1;
	}
	else {
		show = 0;
	}
		
	if (input == KEY_ENTER) {
		enter = 1;
		return ST_OPTIONS_KEYCLICK;
	}

	if ( (input == KEY_PLUS) || (input == KEY_MINUS) ) {
		gKeyClickStatus = !gKeyClickStatus;
		show = 1;
	}

	if ( show ) {
		if ( gKeyClickStatus ) {
			LCD_puts_f(PSTR("On"),1);
		}
		else {
			LCD_puts_f(PSTR("Off"),1);
		}
		LCD_UpdateRequired(true, 0);
	}

	return ST_OPTIONS_KEYCLICK_FUNC;
}




/*****************************************************************************
*
*   Function name : Delay
*
*   Returns :       None
*
*   Parameters :    uint16_t millisec
*
*   Purpose :       Delay-loop
*
*****************************************************************************/
void Delay(uint16_t millisec)
{
	// busy-wait, calibrated by the compiler from F_CPU (1 MHz in this project).
	// Must not be called while the clock prescaler is changed (RS232 mode, 2 MHz).
	while (millisec--) {
		_delay_ms(1);
	}
}



/*****************************************************************************
*
*   Function name : Revision
*
*   Returns :       None
*
*   Parameters :    char input
*
*   Purpose :       Display the software revision
*
*****************************************************************************/
uint8_t Revision(uint8_t input)
{
    static uint8_t enter = 1;
    
    if(enter) {
        enter = 0;
        
        LCD_putc(0, 'R');
        LCD_putc(1, 'E');
        LCD_putc(2, 'V');
        // LCD_putc(3, ' ');
        LCD_putc(3, (SWHIGH + 0x30)); // LCD_putc(4, (SWHIGH + 0x30));       //SWHIGH/LOW are defined in "main.h"
        LCD_putc(4, (SWLOW + 0x30)); // LCD_putc(5, (SWLOW + 0x30));
        LCD_putc(5, (SWLOWLOW + 0x30)); // LCD_putc(5, (SWLOW + 0x30));
        LCD_putc(6, '\0');
        
        LCD_UpdateRequired(true, 0);          
    }
    else if (input == KEY_PREV) {
        enter = 1;
        return ST_AVRBF;
    }
    
    return ST_AVRBF_REV;
}




/*****************************************************************************
*
*   Function name : OSCCAL_calibration
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Calibrate the internal OSCCAL byte, using the external 
*                   32,768 kHz crystal as reference
*
*****************************************************************************/
void OSCCAL_calibration(void)
{
    bool calibrate = false;
    uint16_t temp;
    uint8_t tempL;

    SetClockPrescaler(CLOCK_1MHZ);

    TIMSK2 = 0;             //disable OCIE2A and TOIE2

    ASSR = (1<<AS2);        //select asynchronous operation of timer2 (32,768kHz)
    
    OCR2A = 200;            // set timer2 compare value 

    TIMSK0 = 0;             // delete any interrupt sources
        
    TCCR1B = (1<<CS10);     // start timer1 with no prescaling
    TCCR2A = (1<<CS20);     // start timer2 with no prescaling

    while((ASSR & 0x01) | (ASSR & 0x04));       //wait for TCN2UB and TCR2UB to be cleared

    Delay(1000);    // wait for external crystal to stabilise
    
    while(!calibrate)
    {
        cli(); // mt __disable_interrupt();  // disable global interrupt
        
        TIFR1 = 0xFF;   // delete TIFR1 flags
        TIFR2 = 0xFF;   // delete TIFR2 flags
        
        TCNT1H = 0;     // clear timer1 counter
        TCNT1L = 0;
        TCNT2 = 0;      // clear timer2 counter
           
        // shc/mt while ( !(TIFR2 && (1<<OCF2A)) );   // wait for timer2 compareflag    
        while ( !(TIFR2 & (1<<OCF2A)) );   // wait for timer2 compareflag

        TCCR1B = 0; // stop timer1

        sei(); // __enable_interrupt();  // enable global interrupt
    
        // shc/mt if ( (TIFR1 && (1<<TOV1)) )
        if ( (TIFR1 & (1<<TOV1)) ) {
            temp = 0xFFFF;      // if timer1 overflows, set the temp to 0xFFFF
        }
        else {   // read out the timer1 counter value
            tempL = TCNT1L;
            temp = TCNT1H;
            temp = (temp << 8);
            temp += tempL;
        }
    
        if (temp > 6250) {
            OSCCAL--;   // the internRC oscillator runs to fast, decrease the OSCCAL
        }
        else if (temp < 6120) {
            OSCCAL++;   // the internRC oscillator runs to slow, increase the OSCCAL
        }
        else {
            calibrate = true;   // the interRC is correct
        }

        TCCR1B = (1<<CS10); // start timer1
    }
}

#if 0
/*****************************************************************************
*
*   Function name : watchdog_off_at_init
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Disable the Watchdog
*                   Function disabled by default.
*                   Might be interesting for special bootloaders
*                   (not the default Butterfly bootloader)
*                   This function has been added by Martin Thomas
*
*****************************************************************************/
void watchdog_off_at_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void watchdog_off_at_init(void)
{
	wdt_reset();
	MCUSR = 0;
	wdt_disable();
}
#endif
