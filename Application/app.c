/* Display states, edit workflow, student ID scrolling and alarm decisions. */
//三种显示状态、学号滚动和修改流程

#include "app.h"
#include "board.h"
#include "config.h"
#include "keys.h"
#include "uart1.h"

#define STATE_CLOCK      0U
#define STATE_STUDENT    1U
#define STATE_ALARM      2U

unsigned char code StudentId[] = STUDENT_ID_TEXT;
#define STUDENT_ID_LEN ((unsigned char)(sizeof(StudentId) - 1U))

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
    unsigned char data second_event;

    if (Uart1_TakeTime(&hour, &minute, &second))
    {
        Board_SetTime(hour, minute, second);
        DisplayState = STATE_CLOCK;
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
            (ClockHour == AlarmHour) &&
            (ClockMinute == AlarmMinute))
            Board_StartAlarm();
    }
}

void App_Tick10ms(unsigned char key_events)
{
    if (key_events != KEY_EVENT_NONE)
        App_HandleKeys(key_events);

    UiTicks++;
    if ((DisplayState == STATE_STUDENT) &&
        (StudentId[4] != '\0') &&
        (UiTicks >= STUDENT_SCROLL_TICKS))
    {
        UiTicks = 0;
        if (++StudentPos > (STUDENT_ID_LEN - 4U)) StudentPos = 0;
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
            else if ((DisplayState == STATE_ALARM) && (EditItem < FIELD_MINUTE))
                EditItem++;
            else
                EditItem = FIELD_NONE;
        }
        else
        {
            if (++DisplayState > STATE_ALARM) DisplayState = STATE_CLOCK;
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
                Board_AdjustAlarm(EditItem, 0U);
        }
    }

    if ((events & KEY_EVENT_PLUS) &&
        (EditItem == FIELD_NONE) &&
        (DisplayState == STATE_CLOCK))
    {
        ClockTickEnabled = !ClockTickEnabled;
    }
    else if ((events & (KEY_EVENT_PLUS | KEY_EVENT_PLUS_REPEAT)) &&
             (EditItem != FIELD_NONE))
    {
        if (DisplayState == STATE_CLOCK)
            Board_AdjustClock(EditItem, 1U);
        else
            Board_AdjustAlarm(EditItem, 1U);
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

    blink = 0;
    colon = COLON_ON;

    if (DisplayState == STATE_STUDENT)
    {
        d0 = StudentId[StudentPos] - '0';
        d1 = StudentId[StudentPos + 1U] - '0';
        d2 = StudentId[StudentPos + 2U] - '0';
        d3 = StudentId[StudentPos + 3U] - '0';
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
    else
    {
        d0 = AlarmHour / 10U; d1 = AlarmHour % 10U;
        d2 = AlarmMinute / 10U; d3 = AlarmMinute % 10U;
        if (EditItem == FIELD_HOUR) blink = 0x03;
        else if (EditItem == FIELD_MINUTE) blink = 0x0C;
    }

    Board_SetDisplay(d0, d1, d2, d3, blink, colon);
}
