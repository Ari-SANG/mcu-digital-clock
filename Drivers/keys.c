/* P1.3/P1.4/P1.5 debounce and non-blocking hold-repeat generation. */
//按键消抖与长按连发

#include <STC15.H>
#include "config.h"
#include "keys.h"

sbit KEY_MODE = P1^3;
sbit KEY_MINUS = P1^4;
sbit KEY_PLUS = P1^5;

static unsigned char data KeyStable;
static unsigned char data KeyDebounce[3];
static unsigned char data KeyHold[3];

void Keys_Init(void)
{
    KEY_MODE = 1;
    KEY_MINUS = 1;
    KEY_PLUS = 1;
    P1M1 &= ~0x38;
    P1M0 &= ~0x38;
    KeyStable = 0;
}

unsigned char Keys_Scan10ms(void)
{
    unsigned char raw;
    unsigned char events;
    unsigned char i;
    unsigned char mask;

    raw = 0;
    if (!KEY_MODE) raw |= KEY_EVENT_MODE;
    if (!KEY_MINUS) raw |= KEY_EVENT_MINUS;
    if (!KEY_PLUS) raw |= KEY_EVENT_PLUS;
    events = KEY_EVENT_NONE;

    for (i = 0; i < 3U; i++)
    {
        mask = 1U << i;
        if ((raw & mask) == (KeyStable & mask))
            KeyDebounce[i] = 0;
        else if (++KeyDebounce[i] >= KEY_DEBOUNCE_TICKS)
        {
            KeyDebounce[i] = 0;
            if (raw & mask)
            {
                KeyStable |= mask;
                KeyHold[i] = 0;
                events |= mask;
            }
            else
            {
                KeyStable &= ~mask;
                KeyHold[i] = 0;
            }
        }

        if ((i != 0U) && (KeyStable & mask))
        {
            if (++KeyHold[i] >= KEY_REPEAT_START_TICKS)
            {
                events |= mask;
                KeyHold[i] = KEY_REPEAT_START_TICKS - KEY_REPEAT_STEP_TICKS;
            }
        }
    }
    return events;
}
