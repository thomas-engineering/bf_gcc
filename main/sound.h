#ifndef SOUND_H_
#define SOUND_H_

#include <stdbool.h>

void Sound_Init(void);
uint8_t SelectSound(uint8_t);
uint8_t Sound(uint8_t);
void Play_Tune(void);

void PlayClick(void); // mt

// global variable to prevent entering power save, when playing.
extern volatile bool gPlaying; 


#endif /* SOUND_H_ */
