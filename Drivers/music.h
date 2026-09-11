#ifndef MUSIC_H
#define MUSIC_H

#define MUSIC_COUNT         4U
#define MUSIC_PREVIEW_MS    6000U

extern volatile bit MusicPlaying;

void Music_Init(void);
void Music_Start(unsigned char melody, unsigned int limit_ms);
void Music_StartClockTick(void);
void Music_Stop(void);
void Music_Tick1ms(void);

#endif
