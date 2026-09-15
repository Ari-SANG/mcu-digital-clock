#include "stc15w4k32s4.h"
#include "board.h"
#include "config.h"
#include "clock.h"
#include "alarm.h"

static volatile u8 data g_alarm_hour;
static volatile u8 data g_alarm_minute;
static volatile bit g_alarm_enabled;
static volatile bit g_alarm_ringing;
static volatile u16 data g_ring_ms;
static volatile u8 data g_tone_divider;
static bit g_trigger_armed;

void alarm_init(void)
{
    g_alarm_hour = ALARM_START_HOUR;
    g_alarm_minute = ALARM_START_MINUTE;
    g_alarm_enabled = 1;
    g_alarm_ringing = 0;
    g_ring_ms = 0;
    g_tone_divider = 0;
    g_trigger_armed = 1;
    BUZZER_PIN = 1;
}

void alarm_tick_1ms_isr(void)
{
    if (!g_alarm_ringing)
    {
        BUZZER_PIN = 1;
        return;
    }

    /* A 1 ms toggle period gives a 500 Hz square wave for the passive buzzer. */
    g_tone_divider++;
    if (g_tone_divider >= 1U)
    {
        g_tone_divider = 0;
        BUZZER_PIN = !BUZZER_PIN;
    }

    g_ring_ms++;
    if (g_ring_ms >= ALARM_RING_MS)
    {
        g_alarm_ringing = 0;
        BUZZER_PIN = 1;
    }
}

void alarm_task_second(void)
{
    ClockTime now;

    clock_get(&now);
    if (now.second != 0U)
    {
        g_trigger_armed = 1;
        return;
    }

    if (g_alarm_enabled && g_trigger_armed &&
        (now.hour == g_alarm_hour) && (now.minute == g_alarm_minute))
    {
        g_trigger_armed = 0;
        g_ring_ms = 0;
        g_tone_divider = 0;
        g_alarm_ringing = 1;
    }
}

void alarm_get(AlarmConfig *value)
{
    bit old_ea;

    old_ea = EA;
    EA = 0;
    value->hour = g_alarm_hour;
    value->minute = g_alarm_minute;
    value->enabled = g_alarm_enabled;
    EA = old_ea;
}

bit alarm_set(u8 hour, u8 minute, bit enabled)
{
    bit old_ea;

    if ((hour >= 24U) || (minute >= 60U))
    {
        return 0;
    }

    old_ea = EA;
    EA = 0;
    g_alarm_hour = hour;
    g_alarm_minute = minute;
    g_alarm_enabled = enabled;
    g_trigger_armed = 1;
    EA = old_ea;
    return 1;
}

bit alarm_is_ringing(void)
{
    return g_alarm_ringing;
}

void alarm_stop(void)
{
    bit old_ea;

    old_ea = EA;
    EA = 0;
    g_alarm_ringing = 0;
    g_ring_ms = 0;
    BUZZER_PIN = 1;
    EA = old_ea;
}
