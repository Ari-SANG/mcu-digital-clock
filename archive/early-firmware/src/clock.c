#include "stc15w4k32s4.h"
#include "config.h"
#include "clock.h"

static volatile u8 data g_hour;
static volatile u8 data g_minute;
static volatile u8 data g_second;
static volatile u16 data g_millisecond;
static volatile bit g_second_event;

void clock_init(void)
{
    g_hour = CLOCK_START_HOUR;
    g_minute = CLOCK_START_MINUTE;
    g_second = CLOCK_START_SECOND;
    g_millisecond = 0;
    g_second_event = 0;
}

void clock_tick_1ms_isr(void)
{
    g_millisecond++;
    if (g_millisecond < 1000U)
    {
        return;
    }

    g_millisecond = 0;
    g_second_event = 1;
    g_second++;
    if (g_second >= 60U)
    {
        g_second = 0;
        g_minute++;
        if (g_minute >= 60U)
        {
            g_minute = 0;
            g_hour++;
            if (g_hour >= 24U)
            {
                g_hour = 0;
            }
        }
    }
}

void clock_get(ClockTime *value)
{
    bit old_ea;

    old_ea = EA;
    EA = 0;
    value->hour = g_hour;
    value->minute = g_minute;
    value->second = g_second;
    EA = old_ea;
}

bit clock_set(u8 hour, u8 minute, u8 second)
{
    bit old_ea;

    if ((hour >= 24U) || (minute >= 60U) || (second >= 60U))
    {
        return 0;
    }

    old_ea = EA;
    EA = 0;
    g_hour = hour;
    g_minute = minute;
    g_second = second;
    g_millisecond = 0;
    g_second_event = 0;
    EA = old_ea;
    return 1;
}

bit clock_take_second_event(void)
{
    bit event;
    bit old_ea;

    old_ea = EA;
    EA = 0;
    event = g_second_event;
    g_second_event = 0;
    EA = old_ea;
    return event;
}
