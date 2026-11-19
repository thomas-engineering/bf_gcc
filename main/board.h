// board.h - hardware configuration of the AVR Butterfly (ATmega169)
//
// Pins, clocks and the UART speed that were spread over several files.
// F_CPU (1 MHz) is set in the Makefile.

#ifndef BOARD_H_
#define BOARD_H_

#include <avr/io.h>

#ifndef F_CPU
#error "F_CPU must be defined (see Makefile)"
#endif

// --- UART (RS232) -----------------------------------------------------------
// The UART is only used while the internal RC oscillator runs at 2 MHz
// (see CLOCK_2MHZ in main.h), in double speed mode (U2X).
#define UART_F_CPU      2000000UL
#define UART_BAUD       19200UL
// UBRR value for U2X = 1, rounded to the nearest integer (as util/setbaud.h)
#define UART_UBRR       ((uint16_t)((UART_F_CPU + 4UL*UART_BAUD) / (8UL*UART_BAUD) - 1))

// --- Speaker (piezo), driven by OC1A / PB5 ----------------------------------
#define SPEAKER_PORT    PORTB
#define SPEAKER_DDR     DDRB
#define SPEAKER_PIN     PB5

// --- Voltage reference for NTC / LDR / VCC measurement ----------------------
// The supply of the sensors is switched by this pin (power saving).
#define VCP_PORT        PORTF
#define VCP_DDR         DDRF
#define VCP_PIN         PF3

#endif /* BOARD_H_ */
