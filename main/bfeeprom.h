#include <stdint.h>

#ifndef BF_EEPROM_H_
#define BF_EEPROM_H_

void LoadEEPROM(char *pBuffer, uint8_t num_bytes, uint8_t *EE_START_ADR);
void StoreEEPROM(char *pBuffer, uint8_t num_bytes, uint8_t *EE_START_ADR);

#endif
