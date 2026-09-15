#include "stc15w4k32s4.h"
#include "config.h"
#include "board.h"
#include "display.h"
#include "clock.h"
#include "keys.h"
#include "alarm.h"
#include "uart.h"
#include "ui.h"

#define TIMER0_RELOAD (65536UL - (FOSC / 1000UL))

static volatile bit g_task_10ms;
static u8 data g_10ms_divider;

static void timer0_init(void)
{
    TR0 = 0;
    AUXR |= 0x80;       /* Timer0 in 1T mode. */
    TMOD &= 0xF0;       /* STC mode 0: 16-bit auto reload. */
    TL0 = (u8)TIMER0_RELOAD;
    TH0 = (u8)(TIMER0_RELOAD >> 8);
    TF0 = 0;
    PT0 = 1;            /* Display/timebase has priority over UART. */
    ET0 = 1;
    TR0 = 1;
}

void timer0_isr(void) interrupt 1 using 1
{
    display_tick_1ms_isr();
    clock_tick_1ms_isr();
    alarm_tick_1ms_isr();

    g_10ms_divider++;
    if (g_10ms_divider >= 10U)
    {
        g_10ms_divider = 0;
        g_task_10ms = 1;
    }
}

void main(void)
{
    u8 key_events;

    EA = 0;
    g_task_10ms = 0;
    g_10ms_divider = 0;

    board_io_init();
    display_init();
    clock_init();
    keys_init();
    alarm_init();
    uart_init();
    ui_init();
    timer0_init();

    EA = 1;

    while (1)
    {
        uart_task();

        if (clock_take_second_event())
        {
            alarm_task_second();
        }

        if (g_task_10ms)
        {
            EA = 0;
            g_task_10ms = 0;
            EA = 1;

            keys_task_10ms();
            key_events = keys_take_events();
            ui_handle_keys(key_events);
            ui_task_10ms();
        }
    }
}
