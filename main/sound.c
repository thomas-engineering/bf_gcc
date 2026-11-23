//***************************************************************************
//
//  File........: sound.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: avr-gcc 7.3.0; avr-libc 2.0.0
//
//  Description.: AVR Butterfly sound routines
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - LHM
//  20031009          port to gcc/avr-libc                          - M.Thomas
//  20040123          added temp.-var.                              - n.n./mt
//  20050727          added local helper-function                   - mt
//  20060107          fixed or in pause-if                          - mt
//  20070605          added song                                    - mt
//  202611xx          see https://github.com/thomas-engineering/bf_gcc
//
//***************************************************************************

#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "main.h"
#include "sound.h"
#include "board.h"
#include "timer0.h"
#include "LCD_functions.h"
#include "button.h"

#define DURATION_SEED 32


/*****************************************************************************
*
*   Tone definition. Each tone are set up with a value which will give the
*   right frequency when applied to a 16-bits timer with PWM. These values are based
*   on a CLKcpu running @ 1Mhz.
*
*   First find the frequency for all tones.
*
*   Formula:    ToneX = Bf * 2^(ToneX/12)
*
*   ToneX: the actual tone, e.g. C0 = 3
*   Bf: Basefrequency = 220Hz (A)
*
*
*   E.g: For tone C0 this would be:     C0 = 220 * 2^(3/12)
*                                       C0 = 261,6256...
*
*   Now we must find which value to put in a 16-bits timer with PWM to achieve
*   this frequency
*
*   Formula:    Timer value = 1Mhz / ToneHz / 2
*
*   E.g: For tone C0 this would be:     Timer value = 1000000 / 261,6256... / 2
*                                       Timer value = 1911
*
*   Set up a 16-bits timer to run at Phase/Freq-correct PWM, top value = ICR1,
*   set OC1A when upcounting, clear when downcounting.
*
*****************************************************************************/

#define A   2273        // tone 0
#define xA  2145        // tone 1
#define Ax  2145        // tone 1
#define B   2025        // tone 2
#define C0  1911        // tone 3
#define xC0 1804        // ...
#define Cx0 1804
#define D0  1703
#define xD0 1607
#define Dx0 1607
#define E0  1517
#define F0  1432
#define xF0 1351
#define Fx0 1351
#define G0  1275
#define xG0 1204
#define Gx0 1204
#define A0  1136
#define xA0 1073
#define Ax0 1073
#define B0  1012
#define C1  956
#define xC1 902
#define Cx1 902
#define D1  851
#define xD1 804
#define Dx1 804
#define E1  758
#define F1  716
#define xF1 676
#define Fx1 676
#define G1  638
#define xG1 602
#define Gx1 602
#define A1  568
#define xA1 536
#define Ax1 536
#define B1  506
#define C2  478
#define xC2 451
#define Cx2 451
#define D2  426
#define xD2 402
#define Dx2 402
#define E2  379
#define F2  356
#define xF2 338
#define Fx2 338
#define G2  319
#define xG2 301
#define Gx2 301
#define A2  284
#define xA2 268
#define Ax2 268
#define B2  253
#define C3  239
#define xC3 225
#define Cx3 225
#define D3  213
#define xD3 201
#define Dx3 201
#define E3  190
#define F3  179
#define xF3 169
#define Fx3 169
#define G3  159
#define xG3 150
#define Gx3 150
#define A3  142
#define xA3 134
#define Ax3 134
#define B3  127
#define C4  119


#define P   1           // pause



/******************************************************************************
*
*   The tone definitions are duplicated to accept both upper and lower case
*
******************************************************************************/

#define a   2273        // tone 0
#define xa  2145        // tone 1
#define ax  2145        // tone 1
#define b   2024        // tone 2
#define c0  1911        // tone 3
#define xc0 1804        // ...
#define cx0 1804
#define d0  1703
#define xd0 1607
#define dx0 1607
#define e0  1517
#define f0  1432
#define xf0 1351
#define fx0 1351
#define g0  1275
#define xg0 1204
#define gx0 1204
#define a0  1136
#define xa0 1073
#define ax0 1073
#define b0  1012
#define c1  956
#define xc1 902
#define cx1 902
#define d1  851
#define xd1 804
#define dx1 804
#define e1  758
#define f1  716
#define xf1 676
#define fx1 676
#define g1  638
#define xg1 602
#define gx1 602
#define a1  568
#define xa1 536
#define ax1 536
#define b1  506
#define c2  478
#define xc2 451
#define cx2 451
#define d2  426
#define xd2 402
#define dx2 402
#define e2  379
#define f2  356
#define xf2 338
#define fx2 338
#define g2  319
#define xg2 301
#define gx2 301
#define a2  284
#define xa2 268
#define ax2 268
#define b2  253
#define c3  239
#define xc3 225
#define cx3 225
#define d3  213
#define xd3 201
#define dx3 201
#define e3  190
#define f3  179
#define xf3 169
#define fx3 169
#define g3  159
#define xg3 150
#define gx3 150
#define a3  142
#define xa3 134
#define ax3 134
#define b3  127
#define c4  119



#define p   1


/******************************************************************************
*
*   A song is defined by a table of notes. The first byte sets the tempo. A 
*   high byte will give a low tempo, and opposite. Each tone consists of two 
*   bytes. The first gives the length of the tone, the other gives the frequency. 
*   The frequencies for each tone are defined above. Timer0 controls 
*   the tempo and the length of each tone, while Timer1 with PWM gives the 
*   frequency. The second last byte is a "0" which indicates the end, and the
*   very last byte makes the song loop if it's "1", and not loop if it's "0".
*
******************************************************************************/

const char TEXT_SONG1[] PROGMEM      = "Fur Elise";

// __flash int FurElise[] =   
const uint16_t FurElise[] PROGMEM =   
        {
            3, 
            8,e2, 8,xd2, 8,e2, 8,xd2, 8,e2, 8,b1, 8,d2, 8,c2, 4,a1, 8,p, 
            8,c1, 8,e1, 8,a1, 4,b1, 8,p, 8,e1, 8,xg1, 8,b1, 4,c2, 8,p, 8,e1, 
            8,e2, 8,xd2, 8,e2, 8,xd2, 8,e2, 8,b1, 8,d2, 8,c2, 4,a1, 8,p, 8,c1, 
            8,e1, 8,a1, 4,b1, 8,p, 8,e1, 8,c2, 8,b1, 4,a1, 
            0, 1
        };


//__flash char TEXT_SONG2[]       = "Turkey march";
const char TEXT_SONG2[] PROGMEM  = "Turkey march";

//__flash int Mozart[] = 
const uint16_t Mozart[] PROGMEM = 
        {
            3, 
            16,xf1, 16,e1, 16,xd1, 16,e1, 4,g1, 16,a1, 16,g1, 16,xf1, 16,g1,
            4,b1, 16,c2, 16,b1, 16,xa1, 16,b1, 16,xf2, 16,e2, 16,xd2, 16,e2, 
            16,xf2, 16,e2, 16,xd2, 16,e2, 4,g2, 8,e2, 8,g2, 32,d2, 32,e2, 
            16,xf2, 8,e2, 8,d2, 8,e2, 32,d2, 32,e2, 16,xf2, 8,e2, 8,d2, 8,e2, 
            32,d2, 32,e2, 16,xf2, 8,e2, 8,d2, 8,xc2, 4,b1, 
            0, 1
        };

// mt song 3 & 4 where commented out by ATMEL - see their readme
// well, the gcc-geek wants all the songs ;-)
const char TEXT_SONG3[] PROGMEM      = "Minuet";

const uint16_t Minuet[] PROGMEM = 
        {
            2, 
            4,d2, 8,g1, 8,a1, 8,b1, 8,c2, 4,d2, 4,g1, 4,g1, 4,e2, 8,c2, 
            8,d2, 8,e2, 8,xf2, 4,g2, 4,g1, 4,g1, 4,c2, 8,d2, 8,c2, 8,b1, 
            8,a1, 4,b1, 8,c2, 8,b1, 8,a1, 8,g1, 4,xf1, 8,g1, 8,a1, 8,b1, 
            8,g1, 4,b1, 2,a1, 
            0, 1
        };


const char TEXT_SONG4[] PROGMEM    = "Auld Lang Syne";

const uint16_t AuldLangSyne[] PROGMEM = 
        {  
            3, 
            4,g2, 2,c3, 8,c3, 4,c3, 4,e3, 2,d3, 8,c3, 4,d3, 8,e3, 8,d3, 2,c3, 
            8,c3, 4,e3, 4,g3, 2,a3, 8,p, 4,a3, 2,g3, 8,e3, 4,e3, 4,c3, 2,d3, 
            8,c3, 4,d3, 8,e3, 8,d3, 2,c3, 8,a2, 4,a2, 4,g2, 2,c3, 4,p,
            0, 1
        };


//__flash char TEXT_SONG5[]      =   "Sirene1";
const char TEXT_SONG5[] PROGMEM =   "Sirene1";

//__flash int Sirene1[] = 
const uint16_t Sirene1[] PROGMEM = 
        {
            0,
            32,400, 32,397, 32,394, 32,391, 32,388, 32,385, 32,382, 32,379,
            32,376, 32,373, 32,370, 32,367, 32,364, 32,361, 32,358, 32,355,
            32,352, 32,349, 32,346, 32,343, 32,340, 32,337, 32,334, 32,331, 
            32,328, 32,325, 32,322, 32,319, 32,316, 32,313, 32,310, 32,307, 
            32,304, 32,301, 32,298, 32,298, 32,301, 32,304, 32,307, 32,310, 
            32,313, 32,316, 32,319, 32,322, 32,325, 32,328, 32,331, 32,334, 
            32,337, 32,340, 32,343, 32,346, 32,349, 32,352, 32,355, 32,358, 
            32,361, 32,364, 32,367, 32,370, 32,373, 32,376, 32,379, 32,382, 
            32,385, 32,388, 32,391, 32,394, 32,397, 32,400,
            0, 1
        };

//__flash char TEXT_SONG6[]      =   "Sirene2";
const char TEXT_SONG6[] PROGMEM =   "Sirene2";

//__flash int Sirene2[] = 
const uint16_t Sirene2[] PROGMEM = 
        {
            3, 
            4,c2, 4,g2, 
            0, 1
        };


//__flash char TEXT_SONG7[]      =   "Whistle";
const char TEXT_SONG7[] PROGMEM      =   "Whistle";

//__flash int Whistle[] = 
const uint16_t Whistle[] PROGMEM = 
        {
            0, 
            32,200, 32,195, 32,190, 32,185, 32,180, 32,175, 32,170, 32,165,  
            32,160, 32,155, 32,150, 32,145, 32,140, 32,135, 32,130, 32,125,              
            32,120, 32,115, 32,110, 32,105, 32,100, 8,p, 32,200, 32,195, 
            32,190, 32,185, 32,180, 32,175, 32,170, 32,165, 32,160, 32,155, 
            32,150, 32,145, 32,140, 32,135, 32,130, 32,125, 32,125, 32,130, 
            32,135, 32,140, 32,145, 32,150, 32,155, 32,160, 32,165, 32,170, 
            32,175, 32,180, 32,185, 32,190, 32,195, 32,200, 
            0, 0
        };

// "Exorzist" added by Martin Thomas (based on demo from the AT90CAN-Library 3.1)
const char TEXT_SONG8[] PROGMEM   =   "Exorzist";
const uint16_t Exorcist[] PROGMEM =
        {
        21,
        144,E2, 144,A2, 144,E2, 144,B2, 144,E2, 144,G2, 144,A2, 144,E2,
        144,C3, 144,A2, 144,D3, 144,E2, 144,B2, 144,C3, 144,E2, 144,B2,
        144,E2, 144,A2, 144,E2, 144,B2, 144,G2, 144,A2, 144,E2, 144,C3,
        144,E2, 144,D3, 144,E2, 144,B2, 144,C3, 144,E2, 144,B2, 144,E2,
        144,A2, 144,E2, 144,B2, 144,E2, 144,G2, 576,A2,
        0, 1
        };

// pointer-array with pointers to the song arrays
const uint16_t * const Songs[] PROGMEM = { FurElise, Mozart, Minuet, AuldLangSyne, Sirene1, Sirene2, Whistle, Exorcist, 0 };

PGM_P const TEXT_SONG_TBL[] PROGMEM   = { TEXT_SONG1, TEXT_SONG2, TEXT_SONG3, TEXT_SONG4, TEXT_SONG5, TEXT_SONG6, TEXT_SONG7, TEXT_SONG8, 0 };
// const char *TEXT_SONG_TBL[]    = { TEXT_SONG1, TEXT_SONG2, TEXT_SONG3, TEXT_SONG4, TEXT_SONG5, TEXT_SONG6, TEXT_SONG7, 0};

//__flash char PLAYING[]          = "PLAYING";
const char PLAYING[] PROGMEM   = "PLAYING";

// mt: int __flash *pSong;     // pointer to the different songs in flash
const uint16_t *pSong;	// points to a song table in FLASH: read only with pgm_read_word()

volatile bool gPlaying = false;

static uint8_t Volume   = 80;
static uint8_t Duration = 0;
static uint8_t Tone  = 0;
static uint8_t Tempo;


/*****************************************************************************
*
*   Function name : Sound_Init
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Set up Timer1 with PWM
*
*****************************************************************************/
void Sound_Init(void)
{
    TCCR1A = (1<<COM1A1);// | (1<<COM1A0); // Set OC1A when upcounting, clear when downcounting
    TCCR1B = (1<<WGM13);        // Phase/Freq-correct PWM, top value = ICR1
    
    sbiBF(TCCR1B, CS10);             // start Timer1, prescaler(1)    
    
    OCR1AH = 0;        // Set a initial value in the OCR1A-register
    OCR1AL = Volume;   // This will adjust the volume on the buzzer, lower value => higher volume
    
    Timer0_RegisterCallbackFunction(Play_Tune);     // Timer/Counter0 keeps the right beat
}

/*****************************************************************************
*
*   Function name : SelectSound
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Select song/tune
*
*****************************************************************************/
// mt inserted local helper to save some flash-space
static void showSongName(uint8_t songnum)
{
    LCD_puts_f((PGM_P)pgm_read_word(&TEXT_SONG_TBL[songnum]), 1);  // mt   // Set up the a song in the LCD
}

uint8_t SelectSound(uint8_t input)
{
    static uint8_t enter = 1;
    // mt static char song = 0;
    static uint8_t song = 0;
    
    
    if (enter) {
        enter = 0;
        
        showSongName(song);

        // mt pSong = Songs[song];            // point to this song             
        pSong = (const uint16_t *)pgm_read_word(&Songs[song]);   // pointer to the song table in flash

    }      
        
    if (input == KEY_PLUS) { // shift to next song
        if(!song) { // wrap around the table
            for(song=1; pgm_read_word(&TEXT_SONG_TBL[song]); song++){};
            song--;
        }    
        else {
            song--;
        }

        // mt LCD_puts_f((PGM_P)pgm_read_word(&TEXT_SONG_TBL[song]), 1); // mt
        showSongName(song);
        
        // mt pSong = Songs[song];
        pSong = (const uint16_t *)pgm_read_word(&Songs[song]);

    }    
    else if (input == KEY_MINUS) { // shift to next song
        song++;

        if( !(pgm_read_word(&TEXT_SONG_TBL[song])) ) { // wrap around the table
            song = 0;
        }
        
        // mt LCD_puts_f((PGM_P)pgm_read_word(&TEXT_SONG_TBL[song]), 1);
        showSongName(song);
        
        // mt pSong = Songs[song];
        pSong = (const uint16_t *)pgm_read_word(&Songs[song]);
    }  
    else if(input == KEY_ENTER) { // start playing
        enter = 1;
        return ST_MUSIC_PLAY;
    }
    else if (input == KEY_PREV) {
        enter = 1;
        return ST_MUSIC;
    }    
        
    return ST_MUSIC_SELECT;
}

/*****************************************************************************
*
*   Function name : Sound
*
*   Returns :       char ST_state (to the state-machine)
*
*   Parameters :    char input (from joystick)
*
*   Purpose :       Start/stop timers, adjust volume 
*
*****************************************************************************/
uint8_t Sound(uint8_t input)
{
    static uint8_t enter = 1;

    if (enter) {
        enter = 0;
        Tone = 0;
        Sound_Init();           // start playing
        LCD_puts_f(PLAYING, 1);
        gPlaying = true;
    }        
    else if (!gPlaying) {
        Timer0_RemoveCallbackFunction(Play_Tune);
        TCCR1A = 0;
        TCCR1B = 0;
        enter = 1;
        return ST_MUSIC_SELECT;
    }
    
    if (input == KEY_PLUS) { // increase the volum
        if(Volume >= 80) {
            Volume = 80;
        }
        else {
            Volume += 5;
        }

        OCR1AH = 0;
        OCR1AL = Volume;
    }
    else if (input == KEY_MINUS) { // decrease the volum
        if(Volume < 11) {
            Volume = 6;
        }
        else {
            Volume -= 5;   
        }
        
        OCR1AH = 0;
        OCR1AL = Volume;
    }         
    
    if (input == KEY_ENTER) { // start/stop playing   
        if (gPlaying) {
            gPlaying = false;
            cbiBF(TCCR1B, 0);                     // stop Playing
            Timer0_RemoveCallbackFunction(Play_Tune);
            TCCR1A = 0;
            TCCR1B = 0;
            sbiBF(SPEAKER_PORT, SPEAKER_PIN);              // set OC1A high
            enter = 1;
            return ST_MUSIC_SELECT;
        }
        else {   
            Duration = 0;                       // start Playing
            Tone = 1;
            Sound_Init();
            LCD_puts_f(PLAYING, 1);            
            gPlaying = true;
        }
    }
    else if (input == KEY_PREV) {
        gPlaying = false;
        cbiBF(TCCR1B, 0);                     // stop Playing
        Timer0_RemoveCallbackFunction(Play_Tune);
        TCCR1A = 0;
        TCCR1B = 0;
        sbiBF(SPEAKER_PORT, SPEAKER_PIN);              // set OC1A high
        enter = 1;
        return ST_MUSIC_SELECT;
    }
        
    return ST_MUSIC_PLAY;
}

/*****************************************************************************
*
*   Function name : Play_Tune
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Plays the song
*
*****************************************************************************/
void Play_Tune(void)
{
    uint16_t temp_tone;	// mt 200301
    uint16_t temp_hi;
    
    uint8_t loop;
    
    if(!Tone) {
        Duration = 0;   
        // mt Tempo = *(pSong + 0);
        Tempo = (uint8_t)pgm_read_word(&pSong[0]);
        Tone = 1;   //Start the song from the beginning
    }
    
    if(!Tempo) {
        if(Duration) { // Check if the lenght of the tone has "expired"   
            Duration--;
        }
        else if(pgm_read_word(&pSong[Tone])) { // If not the end of the song
            // mt: Duration = ( DURATION_SEED / *(pSong + Tone) );  // store the duration
            Duration = ( DURATION_SEED / pgm_read_word(&pSong[Tone]) );  // store the duration
        
            Tone++;                     // point to the next tone in the Song-table        

            temp_tone = pgm_read_word(&pSong[Tone]); // mt 200301
            if( (temp_tone == p) || (temp_tone == P) ) { // if pause
                cbiBF(TCCR1B, CS10);             // stop Timer1, prescaler(1)    
            }
            else {
                sbiBF(TCCR1B, CS10);             // start Timer1, prescaler(1)  
            }
                
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                // mt temp_hi = *(pSong + Tone);      // read out the PWM-value
                // temp_hi = (uint16_t)pSong[Tone];      // read out the PWM-value
                temp_hi = temp_tone;   // mt 200301
                temp_hi >>= 8;                  // move integer 8 bits to the rigth
                
                TCNT1H = 0;                     // reset TCNT1H/L
                TCNT1L = 0;
            
                ICR1H = temp_hi;                // load ICR1H/L
                // mt: ICR1L = *(pSong + Tone);        
                // ICR1L = (uint16_t)pSong[Tone];
                ICR1L = temp_tone;
            }
            
            Tone++;                     // point to the next tone in the Song-table
        }
        else { // the end of song
            Tone++;         // point to the next tone in the Song-table        
            
            // mt: loop = *(pSong + Tone); // get the byte that tells if the song should loop or not
            loop = (uint8_t)pgm_read_word(&pSong[Tone]); // get the byte that tells if the song should loop or not
            
            if( loop ) {
                Tone = 1;
            }
            else { // if not looping the song
                Tone = 0;
                gPlaying = false;
                cbiBF(TCCR1B, 0);                     // stop Playing
                Timer0_RemoveCallbackFunction(Play_Tune);
                TCCR1A = 0;
                TCCR1B = 0;
                sbiBF(SPEAKER_PORT, SPEAKER_PIN);              // set OC1A high
            }
        }
        
        // mt: Tempo = *(pSong + 0);
        Tempo = (uint8_t)pgm_read_word(&pSong[0]);
    }
    else {
        Tempo--;
    }
 
}    

/*****************************************************************************
*
*   Function name : PlayClick
*
*   Returns :       None
*
*   Parameters :    None
*
*   Purpose :       Plays the click-sound
*
*****************************************************************************/
void PlayClick(void)
{
	uint8_t i;
	for (i = 0; i < 10; i++) {
		cbiBF(SPEAKER_PORT, SPEAKER_PIN);
		Delay(1);
		sbiBF(SPEAKER_PORT, SPEAKER_PIN);
		Delay(1);
	}
}
