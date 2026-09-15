/* UART1 command parser and temperature reply. */

#include <STC15.H>
#include "config.h"
#include "music.h"
#include "uart1.h"

static volatile unsigned char data RxState;
static volatile unsigned char data RxCommand;
static volatile unsigned char data RxData[4];
static volatile unsigned char data NewHour;
static volatile unsigned char data NewMinute;
static volatile unsigned char data NewSecond;
static volatile bit NewTimeReady;
static volatile unsigned char data NewAlarmIndex;
static volatile unsigned char data NewAlarmHour;
static volatile unsigned char data NewAlarmMinute;
static volatile unsigned char data NewAlarmMelody;
static volatile bit NewAlarmReady;
static volatile unsigned char data NewSwitchIndex;
static volatile unsigned char data NewSwitchEnabled;
static volatile bit NewSwitchReady;
static volatile unsigned char data NewScreenEnabled;
static volatile bit NewScreenReady;
static volatile bit NewScreenOffRequest;
static volatile bit NewTemperatureRequest;

#define UART_TX_TIMEOUT 50000U

static bit Uart1_SendByte(unsigned char value)
{
    unsigned int data remaining;

    remaining = UART_TX_TIMEOUT;
    TI = 0;
    SBUF = value;
    while (!TI)
    {
        if (--remaining == 0U) return 0;
    }
    TI = 0;
    return 1;
}

static bit Uart1_BcdValid(unsigned char value)
{
    return ((value & 0x0FU) <= 9U) && ((value >> 4) <= 9U);
}

void Uart1_Init(void)
{
    RxState = 0;
    NewTimeReady = 0;
    NewAlarmReady = 0;
    NewSwitchReady = 0;
    NewScreenReady = 0;
    NewScreenOffRequest = 0;
    NewTemperatureRequest = 0;
    SCON = 0x50;
    AUXR |= 0x01;
    AUXR |= 0x04;
    T2L = (65536UL - FOSC / 4UL / UART1_BAUD);
    T2H = (65536UL - FOSC / 4UL / UART1_BAUD) >> 8;
    AUXR |= 0x10;
    ES = 1;
}

bit Uart1_TakeAlarm(unsigned char data *index,
                    unsigned char data *hour,
                    unsigned char data *minute,
                    unsigned char data *melody)
{
    if (!NewAlarmReady) return 0;
    EA = 0;
    *index = NewAlarmIndex;
    *hour = NewAlarmHour;
    *minute = NewAlarmMinute;
    *melody = NewAlarmMelody;
    NewAlarmReady = 0;
    EA = 1;
    return 1;
}

bit Uart1_TakeAlarmSwitch(unsigned char data *index,
                          unsigned char data *enabled)
{
    if (!NewSwitchReady) return 0;
    EA = 0;
    *index = NewSwitchIndex;
    *enabled = NewSwitchEnabled;
    NewSwitchReady = 0;
    EA = 1;
    return 1;
}

bit Uart1_TakeScreenSwitch(unsigned char data *enabled)
{
    if (!NewScreenReady) return 0;
    EA = 0;
    *enabled = NewScreenEnabled;
    NewScreenReady = 0;
    EA = 1;
    return 1;
}

bit Uart1_TakeScreenOffRequest(void)
{
    if (!NewScreenOffRequest) return 0;
    EA = 0;
    NewScreenOffRequest = 0;
    EA = 1;
    return 1;
}

bit Uart1_TakeTemperatureRequest(void)
{
    if (!NewTemperatureRequest) return 0;
    EA = 0;
    NewTemperatureRequest = 0;
    EA = 1;
    return 1;
}

void Uart1_SendTemperature(unsigned char whole, unsigned char decimal)
{
    /* A four-byte reply takes less than 1 ms. Keep Timer1 running, but
       prevent the UART ISR from clearing TI between polling iterations. */
    ES = 0;
    if (Uart1_SendByte(0x05U) &&
        Uart1_SendByte(whole) &&
        Uart1_SendByte(decimal))
        Uart1_SendByte(0xAAU);
    ES = 1;
}

bit Uart1_TakeTime(unsigned char data *hour,
                   unsigned char data *minute,
                   unsigned char data *second)
{
    if (!NewTimeReady) return 0;
    EA = 0;
    *hour = NewHour;
    *minute = NewMinute;
    *second = NewSecond;
    NewTimeReady = 0;
    EA = 1;
    return 1;
}

void Uart1_Isr(void) interrupt 4 using 2
{
    unsigned char value;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;

    if (RI)
    {
        RI = 0;
        value = SBUF;
        if (RxState == 0U)
        {
            if ((value >= 0x01U) && (value <= 0x07U))
            {
                RxCommand = value;
                RxState = 1U;
            }
        }
        else if ((RxCommand == 0x05U) || (RxCommand == 0x07U))
        {
            RxState = 0;
            if (value == 0xAAU)
            {
                if (RxCommand == 0x05U) NewTemperatureRequest = 1;
                else
                {
                    NewScreenOffRequest = 1;
                    SBUF = 0x06U;
                }
            }
        }
        else if (RxState <= ((RxCommand == 0x06U) ? 1U :
                            ((RxCommand == 0x04U) ? 2U :
                            ((RxCommand == 0x03U) ? 4U : 3U))))
        {
            RxData[RxState - 1U] = value;
            RxState++;
        }
        else
        {
            RxState = 0;
            if ((value == 0xAAU) && (RxCommand == 0x01U) &&
                Uart1_BcdValid(RxData[0]) &&
                Uart1_BcdValid(RxData[1]) &&
                Uart1_BcdValid(RxData[2]))
            {
                hour = (RxData[0] >> 4) * 10U + (RxData[0] & 0x0FU);
                minute = (RxData[1] >> 4) * 10U + (RxData[1] & 0x0FU);
                second = (RxData[2] >> 4) * 10U + (RxData[2] & 0x0FU);
                if ((hour < 24U) && (minute < 60U) && (second < 60U))
                {
                    NewHour = hour;
                    NewMinute = minute;
                    NewSecond = second;
                    NewTimeReady = 1;
                    SBUF = 0x06U;
                }
            }
            else if ((value == 0xAAU) && (RxCommand == 0x02U) &&
                     (RxData[0] >= 1U) && (RxData[0] <= ALARM_COUNT) &&
                     Uart1_BcdValid(RxData[1]) &&
                     Uart1_BcdValid(RxData[2]))
            {
                hour = (RxData[1] >> 4) * 10U + (RxData[1] & 0x0FU);
                minute = (RxData[2] >> 4) * 10U + (RxData[2] & 0x0FU);
                if ((hour < 24U) && (minute < 60U))
                {
                    NewAlarmIndex = RxData[0] - 1U;
                    NewAlarmHour = hour;
                    NewAlarmMinute = minute;
                    NewAlarmMelody = 0xFFU;
                    NewAlarmReady = 1;
                    SBUF = 0x06U;
                }
            }
            else if ((value == 0xAAU) && (RxCommand == 0x03U) &&
                     (RxData[0] >= 1U) && (RxData[0] <= ALARM_COUNT) &&
                     (RxData[3] >= 1U) && (RxData[3] <= MUSIC_COUNT) &&
                     Uart1_BcdValid(RxData[1]) &&
                     Uart1_BcdValid(RxData[2]))
            {
                hour = (RxData[1] >> 4) * 10U + (RxData[1] & 0x0FU);
                minute = (RxData[2] >> 4) * 10U + (RxData[2] & 0x0FU);
                if ((hour < 24U) && (minute < 60U))
                {
                    NewAlarmIndex = RxData[0] - 1U;
                    NewAlarmHour = hour;
                    NewAlarmMinute = minute;
                    NewAlarmMelody = RxData[3] - 1U;
                    NewAlarmReady = 1;
                    SBUF = 0x06U;
                }
            }
            else if ((value == 0xAAU) && (RxCommand == 0x04U) &&
                     (RxData[0] >= 1U) && (RxData[0] <= ALARM_COUNT) &&
                     (RxData[1] <= 1U))
            {
                NewSwitchIndex = RxData[0] - 1U;
                NewSwitchEnabled = RxData[1];
                NewSwitchReady = 1;
                SBUF = 0x06U;
            }
            else if ((value == 0xAAU) && (RxCommand == 0x06U) &&
                     (RxData[0] <= 1U))
            {
                NewScreenEnabled = RxData[0];
                NewScreenReady = 1;
                SBUF = 0x06U;
            }
        }
    }
    if (TI) TI = 0;
}
