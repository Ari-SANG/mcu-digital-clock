/* Board GPIO, 1 ms timer, display scan, clock timebase and buzzer output. */
//数码管扫描、时钟计时、GPIO、闹钟和蜂鸣器

#include <STC15.H>
#include "board.h"
#include "config.h"

#define T0_RELOAD (65536UL - FOSC / 1000UL)

sbit DIG1 = P4^2;
sbit DIG2 = P4^1;
sbit DIG3 = P3^7;
sbit DIG4 = P3^6;
sbit COLON = P4^4;
sbit VCCD_ENABLE = P4^5;
sbit BUZZER = P3^5;

static unsigned char code SegCode[11] =
{
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00
};

static volatile unsigned char data Disp[4];
static volatile unsigned char data BlinkMask;
static volatile unsigned char data ColonMode;
static unsigned char data ScanIndex;
static unsigned int data BlinkMs;
static bit BlinkVisible;

volatile unsigned char data ClockHour;
volatile unsigned char data ClockMinute;
volatile unsigned char data ClockSecond;
static volatile unsigned int data ClockMillisecond;
static volatile bit Tick10ms;
static volatile bit SecondEvent;
static unsigned char data Divider10ms;

static unsigned char data AlarmHours[ALARM_COUNT];
static unsigned char data AlarmMinutes[ALARM_COUNT];
static unsigned char data SelectedAlarm;
static volatile unsigned int data AlarmMs;
static volatile unsigned char data ClockTickMs;
volatile bit AlarmRinging;

void Board_Init(void)
{
    P2 = 0xFF;
    DIG1 = 1; DIG2 = 1; DIG3 = 1; DIG4 = 1;
    COLON = 1;
    VCCD_ENABLE = 1;
    BUZZER = 1;

    P2M1 = 0x00;
    P2M0 = 0xFF;
    P3M1 &= ~0xE0;
    P3M0 |= 0xE0;
    P4M1 &= ~0x36;
    P4M0 |= 0x36;
    INT_CLKO &= ~0x01;

    ClockHour = CLOCK_START_HOUR;
    ClockMinute = CLOCK_START_MINUTE;
    ClockSecond = CLOCK_START_SECOND;
    ClockMillisecond = 0;
    AlarmHours[0] = ALARM1_START_HOUR;
    AlarmMinutes[0] = ALARM1_START_MINUTE;
    AlarmHours[1] = ALARM2_START_HOUR;
    AlarmMinutes[1] = ALARM2_START_MINUTE;
    AlarmHours[2] = ALARM3_START_HOUR;
    AlarmMinutes[2] = ALARM3_START_MINUTE;
    SelectedAlarm = 0;
    AlarmRinging = 0;
    AlarmMs = 0;
    ClockTickMs = 0;

    Disp[0] = 10U; Disp[1] = 10U; Disp[2] = 10U; Disp[3] = 10U;
    BlinkMask = 0;
    ColonMode = COLON_OFF;
    ScanIndex = 0;
    BlinkMs = 0;
    BlinkVisible = 1;
    Divider10ms = 0;
    Tick10ms = 0;
    SecondEvent = 0;

    TR0 = 0;
    AUXR |= 0x80;
    TMOD &= 0xF0;
    TL0 = (unsigned char)T0_RELOAD;
    TH0 = (unsigned char)(T0_RELOAD >> 8);
    TF0 = 0;
    PT0 = 1;
    ET0 = 1;
    TR0 = 1;
}

bit Board_Take10msTick(void)
{
    if (!Tick10ms) return 0;
    EA = 0;
    Tick10ms = 0;
    EA = 1;
    return 1;
}

bit Board_TakeSecondEvent(void)
{
    if (!SecondEvent) return 0;
    EA = 0;
    SecondEvent = 0;
    EA = 1;
    return 1;
}

void Board_SetDisplay(unsigned char d0, unsigned char d1,
                      unsigned char d2, unsigned char d3,
                      unsigned char blink_mask, unsigned char colon_mode)
{
    EA = 0;
    Disp[0] = d0; Disp[1] = d1; Disp[2] = d2; Disp[3] = d3;
    BlinkMask = blink_mask;
    ColonMode = colon_mode;
    EA = 1;
}

void Board_SetTime(unsigned char hour, unsigned char minute,
                   unsigned char second)
{
    EA = 0;
    ClockHour = hour;
    ClockMinute = minute;
    ClockSecond = second;
    ClockMillisecond = 0;
    EA = 1;
}

void Board_AdjustClock(unsigned char field, unsigned char increase)
{
    EA = 0;
    if (increase)
    {
        if (field == FIELD_HOUR) { if (++ClockHour >= 24U) ClockHour = 0; }
        else if (field == FIELD_MINUTE) { if (++ClockMinute >= 60U) ClockMinute = 0; }
        else { if (++ClockSecond >= 60U) ClockSecond = 0; }
    }
    else
    {
        if (field == FIELD_HOUR)
            ClockHour = (ClockHour == 0U) ? 23U : ClockHour - 1U;
        else if (field == FIELD_MINUTE)
            ClockMinute = (ClockMinute == 0U) ? 59U : ClockMinute - 1U;
        else
            ClockSecond = (ClockSecond == 0U) ? 59U : ClockSecond - 1U;
    }
    ClockMillisecond = 0;
    EA = 1;
}

void Board_AdjustAlarm(unsigned char field, unsigned char increase)
{
    if (increase)
    {
        if (field == FIELD_HOUR)
        {
            if (++AlarmHours[SelectedAlarm] >= 24U)
                AlarmHours[SelectedAlarm] = 0;
        }
        else
        {
            if (++AlarmMinutes[SelectedAlarm] >= 60U)
                AlarmMinutes[SelectedAlarm] = 0;
        }
    }
    else
    {
        if (field == FIELD_HOUR)
            AlarmHours[SelectedAlarm] = (AlarmHours[SelectedAlarm] == 0U) ?
                                        23U : AlarmHours[SelectedAlarm] - 1U;
        else
            AlarmMinutes[SelectedAlarm] = (AlarmMinutes[SelectedAlarm] == 0U) ?
                                          59U : AlarmMinutes[SelectedAlarm] - 1U;
    }
}

void Board_SelectNextAlarm(void)
{
    if (++SelectedAlarm >= ALARM_COUNT)
        SelectedAlarm = 0;
}

unsigned char Board_GetAlarmHour(void)
{
    return AlarmHours[SelectedAlarm];
}

unsigned char Board_GetAlarmMinute(void)
{
    return AlarmMinutes[SelectedAlarm];
}

bit Board_IsAlarmTime(unsigned char hour, unsigned char minute)
{
    unsigned char data i;

    for (i = 0; i < ALARM_COUNT; i++)
    {
        if ((hour == AlarmHours[i]) && (minute == AlarmMinutes[i]))
            return 1;
    }
    return 0;
}

void Board_StartAlarm(void)
{
    ClockTickMs = 0;
    AlarmMs = 0;
    AlarmRinging = 1;
}

void Board_StartClockTick(void)
{
    if (!AlarmRinging)
        ClockTickMs = CLOCK_TICK_SOUND_MS;
}

void Board_StopAlarm(void)
{
    AlarmRinging = 0;
    BUZZER = 1;
}

void Timer0_Isr(void) interrupt 1 using 1
{
    unsigned char pattern;

    DIG1 = 1; DIG2 = 1; DIG3 = 1; DIG4 = 1;
    P2 = 0xFF;

    pattern = SegCode[Disp[ScanIndex]];
    if ((!BlinkVisible) && ((BlinkMask & (1U << ScanIndex)) != 0U))
        pattern = 0;
    P2 = ~pattern;

    if (ScanIndex == 0U) DIG1 = 0;
    else if (ScanIndex == 1U) DIG2 = 0;
    else if (ScanIndex == 2U) DIG3 = 0;
    else DIG4 = 0;
    if (++ScanIndex >= 4U) ScanIndex = 0;

    if (++BlinkMs >= 500U)
    {
        BlinkMs = 0;
        BlinkVisible = !BlinkVisible;
    }
    if (ColonMode == COLON_ON) COLON = 0;
    else if (ColonMode == COLON_BLINK) COLON = BlinkVisible ? 0 : 1;
    else COLON = 1;

    if (++ClockMillisecond >= 1000U)
    {
        ClockMillisecond = 0;
        SecondEvent = 1;
        if (++ClockSecond >= 60U)
        {
            ClockSecond = 0;
            if (++ClockMinute >= 60U)
            {
                ClockMinute = 0;
                if (++ClockHour >= 24U) ClockHour = 0;
            }
        }
    }

    if (AlarmRinging)
    {
        BUZZER = !BUZZER;
        if (++AlarmMs >= ALARM_RING_MS) Board_StopAlarm();
    }
    else if (ClockTickMs != 0U)
    {
        BUZZER = !BUZZER;
        if (--ClockTickMs == 0U) BUZZER = 1;
    }
    else BUZZER = 1;

    if (++Divider10ms >= 10U)
    {
        Divider10ms = 0;
        Tick10ms = 1;
    }
}
