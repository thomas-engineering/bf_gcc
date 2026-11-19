#include <stdint.h>

char RS232(char input);
char vCard(char input);
char EnterName(char input);

#define STRLENGHT    25

// Layout of the stored name in EEPROM. It sits at the fixed address 0x100
// (as in the original ATMEL firmware); an EEMEM variable would be placed at
// address 0 instead and a name already stored on a board would be lost.
typedef struct
{
    uint8_t length;             // number of characters in name[]
    char    name[STRLENGHT];
} VCARD_EEPROM;
#define EEPROM_VCARD ((VCARD_EEPROM *)0x100)

// global variable to prevent entering power save, when using the UART
extern char gUART;
