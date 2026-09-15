/* Passive-buzzer sequencer using the Timer0 hardware clock output. */

#include <STC15.H>
#include "config.h"
#include "music.h"

#define NOTE_REST  0U
#define NOTE_C5    1U
#define NOTE_D5    2U
#define NOTE_E5    3U
#define NOTE_F5    4U
#define NOTE_FS5   5U
#define NOTE_G5    6U
#define NOTE_A5    7U
#define NOTE_B5    8U
#define NOTE_C6    9U
#define NOTE_E6    10U

#define NOTE_UNIT_MS 50U
#define NOTE_GAP_MS  25U
#define CLOCK_TICK_HZ 500UL
#define T0_RELOAD(freq) (65536UL - (FOSC / 2UL / (freq)))

sbit MUSIC_BUZZER = P3^5;

/* Each pair is note, duration in 50 ms units. */
static unsigned char code MelodyData[] =
{
    /* 0: Ode to Joy. */
    NOTE_E5,8, NOTE_E5,8, NOTE_F5,8, NOTE_G5,8,
    NOTE_G5,8, NOTE_F5,8, NOTE_E5,8, NOTE_D5,8,
    NOTE_C5,8, NOTE_C5,8, NOTE_D5,8, NOTE_E5,8,
    NOTE_E5,12, NOTE_D5,4, NOTE_D5,16, NOTE_REST,4,
    NOTE_E5,8, NOTE_E5,8, NOTE_F5,8, NOTE_G5,8,
    NOTE_G5,8, NOTE_F5,8, NOTE_E5,8, NOTE_D5,8,
    NOTE_C5,8, NOTE_C5,8, NOTE_D5,8, NOTE_E5,8,
    NOTE_D5,12, NOTE_C5,4, NOTE_C5,16, NOTE_REST,6,

    /* 1: Castle in the Sky, main vocal-theme excerpt. */
    NOTE_A5,4, NOTE_B5,4, NOTE_C6,8, NOTE_B5,4,
    NOTE_C6,8, NOTE_E6,8, NOTE_B5,12,
    NOTE_E5,4, NOTE_A5,4, NOTE_G5,8, NOTE_A5,4,
    NOTE_C6,8, NOTE_G5,12,
    NOTE_E5,4, NOTE_F5,4, NOTE_E5,8, NOTE_F5,4,
    NOTE_C6,8, NOTE_E5,12,
    NOTE_C6,4, NOTE_C6,4, NOTE_B5,8, NOTE_FS5,8,
    NOTE_FS5,4, NOTE_B5,8, NOTE_B5,12, NOTE_REST,4,

    /* 2: JJ Lin's Cao Cao, opening of the chorus, transposed to C. */
    NOTE_C6,8, NOTE_B5,4, NOTE_A5,4, NOTE_A5,4,
    NOTE_E5,12, NOTE_G5,4, NOTE_A5,8, NOTE_F5,8,
    NOTE_F5,8, NOTE_E5,4,
    NOTE_D5,8, NOTE_E5,8, NOTE_F5,8, NOTE_F5,8,
    NOTE_G5,8, NOTE_E5,8, NOTE_E5,4, NOTE_F5,4,
    NOTE_E5,4, NOTE_D5,4,
    NOTE_REST,4
};

static unsigned char code MelodyOffsets[4] =
{
    0U, 64U, 118U, 160U
};

static unsigned int code ToneReloads[10] =
{
    T0_RELOAD(523UL),  T0_RELOAD(587UL),  T0_RELOAD(659UL),
    T0_RELOAD(698UL),  T0_RELOAD(740UL),  T0_RELOAD(784UL),
    T0_RELOAD(880UL),  T0_RELOAD(988UL),  T0_RELOAD(1047UL),
    T0_RELOAD(1319UL)
};

static unsigned char data MelodyStart;
static unsigned char data MelodyEnd;
static unsigned char data MelodyPosition;
static unsigned int data NoteRemainingMs;
static unsigned int data PlayRemainingMs;
static unsigned char data ClockTickRemainingMs;
static volatile bit ToneActive;
volatile bit MusicPlaying;

static void Music_StopTone(void)
{
    INT_CLKO &= ~0x01U;
    TR0 = 0;
    TF0 = 0;
    ToneActive = 0;
    MUSIC_BUZZER = 1;
}

static void Music_StartTone(unsigned int reload)
{
    Music_StopTone();
    TL0 = (unsigned char)reload;
    TH0 = (unsigned char)(reload >> 8);
    ToneActive = 1;
    TR0 = 1;
    INT_CLKO |= 0x01U;
}

static void Music_LoadNextNote(void)
{
    unsigned char data note;

    if (MelodyPosition >= MelodyEnd) MelodyPosition = MelodyStart;
    note = MelodyData[MelodyPosition++];
    NoteRemainingMs = (unsigned int)MelodyData[MelodyPosition++] * NOTE_UNIT_MS;

    Music_StopTone();
    if (note != NOTE_REST)
        Music_StartTone(ToneReloads[note - 1U]);
}

void Music_Init(void)
{
    ET0 = 0;
    TR0 = 0;
    /* STC15 Timer0 mode 0: 16-bit auto-reload, 1T hardware output. */
    TMOD &= 0xF0U;
    AUXR |= 0x80U;
    TF0 = 0;
    INT_CLKO &= ~0x01U;
    ToneActive = 0;
    MusicPlaying = 0;
    ClockTickRemainingMs = 0;
    MUSIC_BUZZER = 1;
}

void Music_Start(unsigned char melody, unsigned int limit_ms)
{
    if (melody >= MUSIC_COUNT) melody = 0;
    ET1 = 0;
    Music_StopTone();
    MusicPlaying = 0;
    ClockTickRemainingMs = 0;
    MelodyStart = MelodyOffsets[melody];
    MelodyEnd = MelodyOffsets[melody + 1U];
    MelodyPosition = MelodyStart;
    PlayRemainingMs = limit_ms;
    MusicPlaying = 1;
    Music_LoadNextNote();
    ET1 = 1;
}

void Music_StartClockTick(void)
{
    if (MusicPlaying) return;
    ET1 = 0;
    ClockTickRemainingMs = CLOCK_TICK_SOUND_MS;
    Music_StartTone((unsigned int)T0_RELOAD(CLOCK_TICK_HZ));
    ET1 = 1;
}

void Music_Stop(void)
{
    ET1 = 0;
    Music_StopTone();
    MusicPlaying = 0;
    ClockTickRemainingMs = 0;
    MUSIC_BUZZER = 1;
    ET1 = 1;
}

void Music_Tick1ms(void)
{
    if (!MusicPlaying)
    {
        if ((ClockTickRemainingMs != 0U) &&
            (--ClockTickRemainingMs == 0U))
            Music_StopTone();
        return;
    }

    if (PlayRemainingMs != 0U)
    {
        if (--PlayRemainingMs == 0U)
        {
            Music_Stop();
            return;
        }
    }

    if (NoteRemainingMs != 0U)
    {
        NoteRemainingMs--;
        if ((NoteRemainingMs == NOTE_GAP_MS) && ToneActive)
            Music_StopTone();
        if (NoteRemainingMs == 0U) Music_LoadNextNote();
    }
}
