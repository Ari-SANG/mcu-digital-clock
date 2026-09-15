#include "stc15w4k32s4.h"
#include "config.h"
#include "clock.h"
#include "uart.h"

/* Protocol: 0x01, BCD(hour), BCD(minute), BCD(second), 0xAA. */
#define UART_DIVISOR \
    ((FOSC + (UART_BAUD * 16UL)) / (UART_BAUD * 32UL))
#define UART_RELOAD (65536UL - UART_DIVISOR)

static volatile u8 data g_rx_state;
static volatile u8 data g_rx_bcd[3];
static volatile u8 data g_pending_hour;
static volatile u8 data g_pending_minute;
static volatile u8 data g_pending_second;
static volatile bit g_time_pending;

static bit bcd_valid(u8 value)
{
    return (((value >> 4) & 0x0FU) <= 9U) && ((value & 0x0FU) <= 9U);
}

static u8 bcd_to_u8(u8 value)
{
    return (u8)(((value >> 4) & 0x0FU) * 10U + (value & 0x0FU));
}

void uart_init(void)
{
    g_rx_state = 0;
    g_time_pending = 0;

    P_SW1 &= 0x3F;      /* UART1 at RxD=P3.0, TxD=P3.1. */
    SCON = 0x50;        /* 8 data bits, variable baud, receive enabled. */

    TR1 = 0;
    AUXR |= 0x40;       /* Timer1 in 1T mode. */
    AUXR &= (u8)~0x01;  /* Timer1 is UART1 baud-rate source. */
    TMOD &= 0x0F;       /* STC mode 0: 16-bit auto reload. */
    TL1 = (u8)UART_RELOAD;
    TH1 = (u8)(UART_RELOAD >> 8);
    TF1 = 0;
    RI = 0;
    TI = 0;
    ES = 1;
    TR1 = 1;
}

void uart_task(void)
{
    u8 hour;
    u8 minute;
    u8 second;
    bit old_ea;

    if (!g_time_pending)
    {
        return;
    }

    old_ea = EA;
    EA = 0;
    hour = g_pending_hour;
    minute = g_pending_minute;
    second = g_pending_second;
    g_time_pending = 0;
    EA = old_ea;

    clock_set(hour, minute, second);
}

void uart_isr(void) interrupt 4 using 2
{
    u8 value;
    u8 hour;
    u8 minute;
    u8 second;

    if (RI)
    {
        RI = 0;
        value = SBUF;

        if (g_rx_state == 0U)
        {
            if (value == 0x01U) g_rx_state = 1U;
        }
        else if (g_rx_state <= 3U)
        {
            g_rx_bcd[g_rx_state - 1U] = value;
            g_rx_state++;
        }
        else
        {
            g_rx_state = 0;
            if ((value == 0xAAU) && bcd_valid(g_rx_bcd[0]) &&
                bcd_valid(g_rx_bcd[1]) && bcd_valid(g_rx_bcd[2]))
            {
                hour = bcd_to_u8(g_rx_bcd[0]);
                minute = bcd_to_u8(g_rx_bcd[1]);
                second = bcd_to_u8(g_rx_bcd[2]);
                if ((hour < 24U) && (minute < 60U) && (second < 60U))
                {
                    g_pending_hour = hour;
                    g_pending_minute = minute;
                    g_pending_second = second;
                    g_time_pending = 1;
                }
            }
        }
    }

    if (TI) TI = 0;
}
