/* Passive-buzzer melody sequencer and Timer1 tone generator. */

#include <STC15.H>
#include "config.h"
#include "music.h"

#define NOTE_REST  0U
#define NOTE_C4    1U
#define NOTE_D4    2U
#define NOTE_E4    3U
#define NOTE_F4    4U
#define NOTE_G4    5U
#define NOTE_A4    6U
#define NOTE_B4    7U
#define NOTE_C5    8U
#define NOTE_D5    9U
#define NOTE_E5    10U
#define NOTE_F5    11U
#define NOTE_G5    12U

#define NOTE_UNIT_MS 50U
#define T1_RELOAD(freq) (65536UL - (FOSC / 12UL / 2UL / (freq)))

sbit MUSIC_BUZZER = P3^5;

/* Each pair is note, duration in 50 ms units. */
static unsigned char code MelodyData[] =
{
    /* 0: Twinkle Twinkle Little Star */
    NOTE_C4,6, NOTE_C4,6, NOTE_G4,6, NOTE_G4,6,
    NOTE_A4,6, NOTE_A4,6, NOTE_G4,12, NOTE_REST,2,
    NOTE_F4,6, NOTE_F4,6, NOTE_E4,6, NOTE_E4,6,
    NOTE_D4,6, NOTE_D4,6, NOTE_C4,12, NOTE_REST,4,

    /* 1: Ode to Joy */
    NOTE_E4,6, NOTE_E4,6, NOTE_F4,6, NOTE_G4,6,
    NOTE_G4,6, NOTE_F4,6, NOTE_E4,6, NOTE_D4,6,
    NOTE_C4,6, NOTE_C4,6, NOTE_D4,6, NOTE_E4,6,
    NOTE_E4,9, NOTE_D4,3, NOTE_D4,12, NOTE_REST,2,
    NOTE_E4,6, NOTE_E4,6, NOTE_F4,6, NOTE_G4,6,
    NOTE_G4,6, NOTE_F4,6, NOTE_E4,6, NOTE_D4,6,
    NOTE_C4,6, NOTE_C4,6, NOTE_D4,6, NOTE_E4,6,
    NOTE_D4,9, NOTE_C4,3, NOTE_C4,12, NOTE_REST,4,

    /* 2: Happy Birthday */
    NOTE_G4,4, NOTE_G4,4, NOTE_A4,8, NOTE_G4,8,
    NOTE_C5,8, NOTE_B4,16, NOTE_G4,4, NOTE_G4,4,
    NOTE_A4,8, NOTE_G4,8, NOTE_D5,8, NOTE_C5,16,
    NOTE_G4,4, NOTE_G4,4, NOTE_G5,8, NOTE_E5,8,
    NOTE_C5,8, NOTE_B4,8, NOTE_A4,16,
    NOTE_F5,4, NOTE_F5,4, NOTE_E5,8, NOTE_C5,8,
    NOTE_D5,8, NOTE_C5,16, NOTE_REST,4
};

static unsigned char code MelodyOffsets[4] = { 0U, 32U, 96U, 148U };

static unsigned int code ToneReloads[12] =
{
    T1_RELOAD(262UL), T1_RELOAD(294UL), T1_RELOAD(330UL),
    T1_RELOAD(349UL), T1_RELOAD(392UL), T1_RELOAD(440UL),
    T1_RELOAD(494UL), T1_RELOAD(523UL), T1_RELOAD(587UL),
    T1_RELOAD(659UL), T1_RELOAD(698UL), T1_RELOAD(784UL)
};

static volatile unsigned char data Timer1ReloadHigh;
static volatile unsigned char data Timer1ReloadLow;
static unsigned char data MelodyStart;
static unsigned char data MelodyEnd;
static unsigned char data MelodyPosition;
static unsigned int data NoteRemainingMs;
static unsigned int data PlayRemainingMs;
volatile bit MusicPlaying;

static void Music_LoadNextNote(void)
{
    unsigned char data note;
    unsigned int data reload;

    if (MelodyPosition >= MelodyEnd) MelodyPosition = MelodyStart;
    note = MelodyData[MelodyPosition++];
    NoteRemainingMs = (unsigned int)MelodyData[MelodyPosition++] * NOTE_UNIT_MS;

    TR1 = 0;
    MUSIC_BUZZER = 1;
    if (note != NOTE_REST)
    {
        reload = ToneReloads[note - 1U];
        Timer1ReloadHigh = (unsigned char)(reload >> 8);
        Timer1ReloadLow = (unsigned char)reload;
        TH1 = Timer1ReloadHigh;
        TL1 = Timer1ReloadLow;
        TF1 = 0;
        TR1 = 1;
    }
}

void Music_Init(void)
{
    TR1 = 0;
    TMOD = (TMOD & 0x0FU) | 0x10U;
    AUXR &= ~0x40U;
    TF1 = 0;
    PT1 = 0;
    ET1 = 1;
    MusicPlaying = 0;
    MUSIC_BUZZER = 1;
}

void Music_Start(unsigned char melody, unsigned int limit_ms)
{
    if (melody >= MUSIC_COUNT) melody = 0;
    ET1 = 0;
    MelodyStart = MelodyOffsets[melody];
    MelodyEnd = MelodyOffsets[melody + 1U];
    MelodyPosition = MelodyStart;
    PlayRemainingMs = limit_ms;
    MusicPlaying = 1;
    Music_LoadNextNote();
    ET1 = 1;
}

void Music_Stop(void)
{
    ET1 = 0;
    TR1 = 0;
    TF1 = 0;
    MusicPlaying = 0;
    MUSIC_BUZZER = 1;
    ET1 = 1;
}

void Music_Tick1ms(void)
{
    if (!MusicPlaying) return;

    if (PlayRemainingMs != 0U)
    {
        if (--PlayRemainingMs == 0U)
        {
            Music_Stop();
            return;
        }
    }

    if ((NoteRemainingMs != 0U) && (--NoteRemainingMs == 0U))
        Music_LoadNextNote();
}

void Timer1_Isr(void) interrupt 3 using 3
{
    TR1 = 0;
    if (!MusicPlaying)
    {
        MUSIC_BUZZER = 1;
        return;
    }
    TH1 = Timer1ReloadHigh;
    TL1 = Timer1ReloadLow;
    MUSIC_BUZZER = !MUSIC_BUZZER;
    TR1 = 1;
}
