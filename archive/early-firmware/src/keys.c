#include "stc15w4k32s4.h"
#include "board.h"
#include "keys.h"

#define KEY_COUNT           3U
#define DEBOUNCE_TICKS      3U
#define LONG_PRESS_TICKS    80U
#define REPEAT_TICKS        15U

typedef struct
{
    u8 stable_pressed;
    u8 debounce_count;
    u16 hold_ticks;
    u8 long_sent;
    u8 repeat_ticks;
} KeyState;

static KeyState data g_keys[KEY_COUNT];
static volatile u8 data g_events;

static void key_update(u8 index, u8 pressed)
{
    KeyState data *key;

    key = &g_keys[index];
    if (pressed != key->stable_pressed)
    {
        key->debounce_count++;
        if (key->debounce_count >= DEBOUNCE_TICKS)
        {
            key->debounce_count = 0;
            key->stable_pressed = pressed;
            if (pressed)
            {
                key->hold_ticks = 0;
                key->long_sent = 0;
                key->repeat_ticks = 0;
            }
            else if (!key->long_sent)
            {
                if (index == 0U) g_events |= KEY_EVT_MODE_SHORT;
                else if (index == 1U) g_events |= KEY_EVT_PLUS_SHORT;
                else g_events |= KEY_EVT_FUNC_SHORT;
            }
        }
    }
    else
    {
        key->debounce_count = 0;
    }

    if (!key->stable_pressed)
    {
        return;
    }

    if (key->hold_ticks < 60000U)
    {
        key->hold_ticks++;
    }

    if ((index < 2U) && (!key->long_sent) &&
        (key->hold_ticks >= LONG_PRESS_TICKS))
    {
        key->long_sent = 1;
        key->repeat_ticks = 0;
        if (index == 0U) g_events |= KEY_EVT_MODE_LONG;
        else g_events |= KEY_EVT_PLUS_LONG;
    }
    else if ((index == 1U) && key->long_sent)
    {
        key->repeat_ticks++;
        if (key->repeat_ticks >= REPEAT_TICKS)
        {
            key->repeat_ticks = 0;
            g_events |= KEY_EVT_PLUS_REPEAT;
        }
    }
}

void keys_init(void)
{
    u8 i;

    g_events = 0;
    for (i = 0; i < KEY_COUNT; i++)
    {
        g_keys[i].stable_pressed = 0;
        g_keys[i].debounce_count = 0;
        g_keys[i].hold_ticks = 0;
        g_keys[i].long_sent = 0;
        g_keys[i].repeat_ticks = 0;
    }
}

void keys_task_10ms(void)
{
    key_update(0, KEY_MODE_PIN ? 0U : 1U);
    key_update(1, KEY_PLUS_PIN ? 0U : 1U);
    key_update(2, KEY_FUNC_PIN ? 0U : 1U);
}

u8 keys_take_events(void)
{
    u8 events;
    bit old_ea;

    old_ea = EA;
    EA = 0;
    events = g_events;
    g_events = 0;
    EA = old_ea;
    return events;
}
