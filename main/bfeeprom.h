#ifndef BF_EEPROM_H_
#define BF_EEPROM_H_

#include <stdint.h>

void LoadEEPROM(char *pBuffer, uint8_t num_bytes, uint8_t *ee_start_addr);
void StoreEEPROM(char *pBuffer, uint8_t num_bytes, uint8_t *ee_start_addr);

#endif
