/* UART1 initialization and packed-BCD time frame parser. */

#include <STC15.H>
#include "config.h"
#include "uart1.h"

static volatile unsigned char data RxState;
static volatile unsigned char data RxCommand;
static volatile unsigned char data RxData[3];
static volatile unsigned char data NewHour;
static volatile unsigned char data NewMinute;
static volatile unsigned char data NewSecond;
static volatile bit NewTimeReady;
static volatile unsigned char data NewAlarmIndex;
static volatile unsigned char data NewAlarmHour;
static volatile unsigned char data NewAlarmMinute;
static volatile bit NewAlarmReady;

void Uart1_Init(void)
{
    RxState = 0;
    NewTimeReady = 0;
    NewAlarmReady = 0;
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
                    unsigned char data *minute)
{
    if (!NewAlarmReady) return 0;
    EA = 0;
    *index = NewAlarmIndex;
    *hour = NewAlarmHour;
    *minute = NewAlarmMinute;
    NewAlarmReady = 0;
    EA = 1;
    return 1;
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
            if ((value == 0x01U) || (value == 0x02U))
            {
                RxCommand = value;
                RxState = 1U;
            }
        }
        else if (RxState <= 3U)
        {
            RxData[RxState - 1U] = value;
            RxState++;
        }
        else
        {
            RxState = 0;
            if ((value == 0xAAU) && (RxCommand == 0x01U) &&
                ((RxData[0] & 0x0FU) <= 9U) && ((RxData[0] >> 4) <= 9U) &&
                ((RxData[1] & 0x0FU) <= 9U) && ((RxData[1] >> 4) <= 9U) &&
                ((RxData[2] & 0x0FU) <= 9U) && ((RxData[2] >> 4) <= 9U))
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
                     ((RxData[1] & 0x0FU) <= 9U) && ((RxData[1] >> 4) <= 9U) &&
                     ((RxData[2] & 0x0FU) <= 9U) && ((RxData[2] >> 4) <= 9U))
            {
                hour = (RxData[1] >> 4) * 10U + (RxData[1] & 0x0FU);
                minute = (RxData[2] >> 4) * 10U + (RxData[2] & 0x0FU);
                if ((hour < 24U) && (minute < 60U))
                {
                    NewAlarmIndex = RxData[0] - 1U;
                    NewAlarmHour = hour;
                    NewAlarmMinute = minute;
                    NewAlarmReady = 1;
                    SBUF = 0x06U;
                }
            }
        }
    }
    if (TI) TI = 0;
}
