#ifndef CONFIG_H
#define CONFIG_H

/* The connected MCU was measured at 11.0592 MHz through its UART baud rate. */
#define FOSC                    11059200L
#define UART1_BAUD              115200L

/* User settings. Change the student number only here. */
#define STUDENT_ID_TEXT         "202600000000"
#define CLOCK_START_HOUR        12U
#define CLOCK_START_MINUTE      0U
#define CLOCK_START_SECOND      0U
#define ALARM_START_HOUR        7U
#define ALARM_START_MINUTE      30U

/* UI timings are counted in 10 ms scheduler ticks. */
#define STUDENT_SCROLL_TICKS    70U
#define CLOCK_PAGE_TICKS        300U
#define KEY_DEBOUNCE_TICKS      3U
#define KEY_REPEAT_START_TICKS  60U
#define KEY_REPEAT_STEP_TICKS   12U
#define ALARM_RING_MS           30000U

#endif
