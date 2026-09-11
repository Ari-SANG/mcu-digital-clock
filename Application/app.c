/* Display states, edit workflow, student ID scrolling and alarm decisions. */
//四种显示状态、学号滚动、闹钟音乐和修改流程

#include "app.h"
#include "board.h"
#include "config.h"
#include "keys.h"
#include "music.h"
#include "settings.h"
#include "uart1.h"

#define STATE_CLOCK      0U
#define STATE_STUDENT    1U
#define STATE_ALARM      2U
#define STATE_TEMPERATURE 3U

unsigned char code StudentId[] = STUDENT_ID_TEXT;
#define STUDENT_ID_LEN ((unsigned char)(sizeof(StudentId) - 1U))
#define STUDENT_FIXED_POS (STUDENT_ID_LEN - 4U)
#define STUDENT_FIXED_FLAG 0x80U

static unsigned char data DisplayState;
static unsigned char data EditItem;
static unsigned char data ClockPage;
static unsigned char data StudentPos;
static bit ClockTickEnabled;
static unsigned int data UiTicks;

static void App_Render(void);
static void App_HandleKeys(unsigned char events);

void App_Init(void)
{
    DisplayState = STATE_CLOCK;
    EditItem = FIELD_NONE;
    ClockPage = 0;
    StudentPos = 0;
    ClockTickEnabled = CLOCK_TICK_DEFAULT_ON;
    UiTicks = 0;
    App_Render();
}

void App_Service(void)
{
    unsigned char data hour;
    unsigned char data minute;
    unsigned char data second;
    unsigned char data alarm_index;
    unsigned char data alarm_melody;
    unsigned char data second_event;

    if (Uart1_TakeTime(&hour, &minute, &second))
    {
        if (!AlarmRinging) Music_Stop();
        Board_SetTime(hour, minute, second);
        Settings_RequestSave();
        DisplayState = STATE_CLOCK;
        EditItem = FIELD_NONE;
        UiTicks = 0;
        App_Render();
    }

    if (Uart1_TakeAlarm(&alarm_index, &hour, &minute, &alarm_melody))
    {
        if (!AlarmRinging) Music_Stop();
        Board_SetAlarm(alarm_index, hour, minute, alarm_melody);
        Settings_RequestSave();
        DisplayState = STATE_ALARM;
        EditItem = FIELD_NONE;
        UiTicks = 0;
        App_Render();
    }

    second_event = Board_TakeSecondEvent();
    if (second_event)
    {
        if ((DisplayState == STATE_CLOCK) && ClockTickEnabled)
            Board_StartClockTick();

        if ((ClockSecond == 0U) &&
            Board_IsAlarmTime(ClockHour, ClockMinute))
            Board_StartAlarm();
    }
}

void App_Tick10ms(unsigned char key_events)
{
    if (key_events != KEY_EVENT_NONE)
        App_HandleKeys(key_events);

    UiTicks++;
    if ((DisplayState == STATE_STUDENT) &&
        ((StudentPos & STUDENT_FIXED_FLAG) == 0U) &&
        (UiTicks >= STUDENT_SCROLL_TICKS))
    {
        UiTicks = 0;
        if (++StudentPos > STUDENT_FIXED_POS) StudentPos = 0;
    }
    else if ((DisplayState == STATE_CLOCK) &&
             (EditItem == FIELD_NONE) &&
             (UiTicks >= CLOCK_PAGE_TICKS))
    {
        UiTicks = 0;
        ClockPage = !ClockPage;
    }

    App_Render();
}

static void App_HandleKeys(unsigned char events)
{
    if (AlarmRinging)
    {
        Board_StopAlarm();
        return;
    }

    UiTicks = 0;

    if (events & KEY_EVENT_MODE)
    {
        if (EditItem != FIELD_NONE)
        {
            if ((DisplayState == STATE_CLOCK) && (EditItem < FIELD_SECOND))
                EditItem++;
            else if ((DisplayState == STATE_ALARM) && (EditItem < FIELD_MUSIC))
            {
                EditItem++;
                if (EditItem == FIELD_MUSIC)
                    Music_Start(AlarmMelodies[SelectedAlarm], MUSIC_PREVIEW_MS);
            }
            else
            {
                Settings_RequestSave();
                EditItem = FIELD_NONE;
                Music_Stop();
            }
        }
        else
        {
            if (++DisplayState > STATE_TEMPERATURE) DisplayState = STATE_CLOCK;
            if (DisplayState == STATE_STUDENT) StudentPos = 0;
        }
        return;
    }

    if (events & KEY_EVENT_MINUS)
    {
        if ((EditItem == FIELD_NONE) &&
            ((DisplayState == STATE_CLOCK) || (DisplayState == STATE_ALARM)))
            EditItem = FIELD_HOUR;
        else if (EditItem != FIELD_NONE)
        {
            if (DisplayState == STATE_CLOCK)
                Board_AdjustClock(EditItem, 0U);
            else
            {
                Board_AdjustAlarm(EditItem, 0U);
                if (EditItem == FIELD_MUSIC)
                    Music_Start(AlarmMelodies[SelectedAlarm], MUSIC_PREVIEW_MS);
            }
        }
    }

    if ((events & KEY_EVENT_PLUS) && (EditItem == FIELD_NONE))
    {
        if (DisplayState == STATE_CLOCK)
            ClockTickEnabled = !ClockTickEnabled;
        else if (DisplayState == STATE_STUDENT)
        {
            if (StudentPos & STUDENT_FIXED_FLAG)
                StudentPos = 0;
            else
                StudentPos = STUDENT_FIXED_FLAG | STUDENT_FIXED_POS;
        }
        else if (DisplayState == STATE_ALARM)
            Board_SelectNextAlarm();
    }
    else if ((events & (KEY_EVENT_PLUS | KEY_EVENT_PLUS_REPEAT)) &&
             (EditItem != FIELD_NONE))
    {
        if (DisplayState == STATE_CLOCK)
            Board_AdjustClock(EditItem, 1U);
        else
        {
            Board_AdjustAlarm(EditItem, 1U);
            if (EditItem == FIELD_MUSIC)
                Music_Start(AlarmMelodies[SelectedAlarm], MUSIC_PREVIEW_MS);
        }
    }
}

static void App_Render(void)
{
    unsigned char d0;
    unsigned char d1;
    unsigned char d2;
    unsigned char d3;
    unsigned char blink;
    unsigned char colon;
    unsigned char alarm_hour;
    unsigned char alarm_minute;
    unsigned char student_pos;
    unsigned char temperature;
    unsigned char temperature_decimal;

    blink = 0;
    colon = COLON_ON;

    if (DisplayState == STATE_STUDENT)
    {
        student_pos = StudentPos & 0x7FU;
        d0 = StudentId[student_pos] - '0';
        d1 = StudentId[student_pos + 1U] - '0';
        d2 = StudentId[student_pos + 2U] - '0';
        d3 = StudentId[student_pos + 3U] - '0';
        colon = COLON_OFF;
    }
    else if (DisplayState == STATE_CLOCK)
    {
        if ((EditItem != FIELD_SECOND) &&
            ((EditItem != FIELD_NONE) || (ClockPage == 0U)))
        {
            d0 = ClockHour / 10U; d1 = ClockHour % 10U;
            d2 = ClockMinute / 10U; d3 = ClockMinute % 10U;
        }
        else
        {
            d0 = ClockMinute / 10U; d1 = ClockMinute % 10U;
            d2 = ClockSecond / 10U; d3 = ClockSecond % 10U;
        }
        colon = (EditItem == FIELD_NONE) ? COLON_BLINK : COLON_ON;
        if (EditItem == FIELD_HOUR) blink = 0x03;
        else if (EditItem != FIELD_NONE) blink = 0x0C;
    }
    else if (DisplayState == STATE_ALARM)
    {
        if (EditItem == FIELD_MUSIC)
        {
            d0 = DISPLAY_A;
            d1 = SelectedAlarm + 1U;
            d2 = DISPLAY_DASH;
            d3 = AlarmMelodies[SelectedAlarm] + 1U;
            blink = 0x08;
            colon = COLON_OFF;
        }
        else
        {
            alarm_hour = AlarmHours[SelectedAlarm];
            alarm_minute = AlarmMinutes[SelectedAlarm];
            d0 = alarm_hour / 10U; d1 = alarm_hour % 10U;
            d2 = alarm_minute / 10U; d3 = alarm_minute % 10U;
            if (EditItem == FIELD_HOUR) blink = 0x03;
            else if (EditItem == FIELD_MINUTE) blink = 0x0C;
        }
    }
    else
    {
        temperature = Board_ReadTemperature(&temperature_decimal);
        d0 = temperature / 10U;
        d1 = temperature % 10U;
        d2 = temperature_decimal;
        d3 = DISPLAY_C;
        colon = COLON_DECIMAL;
    }

    Board_SetDisplay(d0, d1, d2, d3, blink, colon);
}
