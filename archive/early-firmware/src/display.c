#include "stc15w4k32s4.h"
#include "board.h"
#include "display.h"

/* Standard active-high abcdefg patterns; output is inverted for common anode. */
static u8 code SEGMENT_TABLE[] =
{
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F,
    0x00              /* blank */
};

static volatile u8 data g_patterns[4];
static volatile u8 data g_blink_mask;
static volatile u8 data g_colon_mode;
static u8 data g_scan_index;
static u16 data g_half_second_ms;
static bit g_blink_visible;

static u8 display_pattern(u8 code_value)
{
    if (code_value > DISP_BLANK)
    {
        return 0x00;
    }
    return SEGMENT_TABLE[code_value];
}

static void display_all_digits_off(void)
{
    DIGIT_1 = 1;
    DIGIT_2 = 1;
    DIGIT_3 = 1;
    DIGIT_4 = 1;
}

void display_init(void)
{
    g_scan_index = 0;
    g_half_second_ms = 0;
    g_blink_visible = 1;
    g_blink_mask = 0;
    g_colon_mode = COLON_OFF;
    g_patterns[0] = 0;
    g_patterns[1] = 0;
    g_patterns[2] = 0;
    g_patterns[3] = 0;
    display_all_digits_off();
    P2 = 0xFF;
    COLON_LED = 1;
}

void display_tick_1ms_isr(void)
{
    u8 pattern;

    display_all_digits_off();
    P2 = 0xFF;

    pattern = g_patterns[g_scan_index];
    if (((g_blink_mask & (1U << g_scan_index)) != 0U) && !g_blink_visible)
    {
        pattern = 0;
    }

    P2 = (u8)~pattern;
    switch (g_scan_index)
    {
        case 0: DIGIT_1 = 0; break;
        case 1: DIGIT_2 = 0; break;
        case 2: DIGIT_3 = 0; break;
        default: DIGIT_4 = 0; break;
    }

    g_scan_index++;
    if (g_scan_index >= 4U)
    {
        g_scan_index = 0;
    }

    g_half_second_ms++;
    if (g_half_second_ms >= 500U)
    {
        g_half_second_ms = 0;
        g_blink_visible = !g_blink_visible;
    }

    if (g_colon_mode == COLON_ON)
    {
        COLON_LED = 0;
    }
    else if (g_colon_mode == COLON_BLINK)
    {
        COLON_LED = g_blink_visible ? 0 : 1;
    }
    else
    {
        COLON_LED = 1;
    }
}

void display_set_codes(u8 d0, u8 d1, u8 d2, u8 d3,
                       u8 decimal_mask, u8 blink_mask)
{
    u8 p0;
    u8 p1;
    u8 p2;
    u8 p3;
    bit old_ea;

    p0 = display_pattern(d0);
    p1 = display_pattern(d1);
    p2 = display_pattern(d2);
    p3 = display_pattern(d3);

    if ((decimal_mask & 0x01U) != 0U) p0 |= 0x80;
    if ((decimal_mask & 0x02U) != 0U) p1 |= 0x80;
    if ((decimal_mask & 0x04U) != 0U) p2 |= 0x80;
    if ((decimal_mask & 0x08U) != 0U) p3 |= 0x80;

    old_ea = EA;
    EA = 0;
    g_patterns[0] = p0;
    g_patterns[1] = p1;
    g_patterns[2] = p2;
    g_patterns[3] = p3;
    g_blink_mask = blink_mask;
    EA = old_ea;
}

void display_set_colon(u8 mode)
{
    if (mode > COLON_BLINK)
    {
        mode = COLON_OFF;
    }
    g_colon_mode = mode;
}
